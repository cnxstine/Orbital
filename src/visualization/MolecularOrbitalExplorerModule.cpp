#include "visualization/MolecularOrbitalExplorerModule.hpp"
#include "core/Engine.hpp"
#include "resources/ResourceManager.hpp"
#include "renderer/backend/GLShader.hpp"
#include "renderer/backend/GLBuffer.hpp"
#include "renderer/backend/GLVertexArray.hpp"
#include "renderer/backend/GLTexture.hpp"
#include "renderer/backend/GLFramebuffer.hpp"
#include "camera/CameraManager.hpp"
#include "camera/controllers/ArcballController.hpp"
#include "visualization/HydrogenicOrbital.hpp"
#include "visualization/MonteCarloSamplingPipeline.hpp"
#include "visualization/StochasticProbabilityCloud.hpp"
#include "visualization/OverlapIntegral.hpp"
#include "visualization/MolecularEnergyModel.hpp"
#include "visualization/EnergySweep.hpp"
#include "visualization/MolecularOrbitalFactory.hpp"
#include "resources/loaders/ShaderLoader.hpp"
#include "core/Log.hpp"

#include <imgui.h>
#include <glad/gl.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <vector>

namespace Orbital {

#pragma pack(push, 1)
struct BMPFileHeader {
    uint16_t fileType{0x4D42}; // "BM"
    uint32_t fileSize{0};
    uint16_t reserved1{0};
    uint16_t reserved2{0};
    uint32_t offsetData{54};
};

struct BMPInfoHeader {
    uint32_t size{40};
    int32_t width{0};
    int32_t height{0};
    uint16_t planes{1};
    uint16_t bitCount{24};
    uint32_t compression{0};
    uint32_t sizeImage{0};
    int32_t xPixelsPerMeter{0};
    int32_t yPixelsPerMeter{0};
    uint32_t colorsUsed{0};
    uint32_t colorsImportant{0};
};
#pragma pack(pop)

static bool SaveBMP(const std::string& filepath, int width, int height, const uint8_t* rgbData) {
    std::ofstream file(filepath, std::ios::binary);
    if (!file) {
        return false;
    }

    int rowSize = (width * 3 + 3) & ~3;
    int padding = rowSize - (width * 3);
    uint32_t fileSize = 54 + rowSize * height;

    BMPFileHeader fileHeader;
    fileHeader.fileSize = fileSize;

    BMPInfoHeader infoHeader;
    infoHeader.width = width;
    infoHeader.height = height;
    infoHeader.sizeImage = rowSize * height;

    file.write(reinterpret_cast<const char*>(&fileHeader), sizeof(fileHeader));
    file.write(reinterpret_cast<const char*>(&infoHeader), sizeof(infoHeader));

    std::vector<uint8_t> paddingBytes(padding, 0);
    for (int y = 0; y < height; ++y) {
        const uint8_t* row = rgbData + y * width * 3;
        for (int x = 0; x < width; ++x) {
            uint8_t r = row[x * 3 + 0];
            uint8_t g = row[x * 3 + 1];
            uint8_t b = row[x * 3 + 2];
            file.put(b);
            file.put(g);
            file.put(r);
        }
        if (padding > 0) {
            file.write(reinterpret_cast<const char*>(paddingBytes.data()), padding);
        }
    }

    return true;
}

MolecularOrbitalExplorerModule::MolecularOrbitalExplorerModule(Engine& engine)
    : m_Engine(engine)
{
}

void MolecularOrbitalExplorerModule::OnEnter()
{
    ORB_CORE_INFO("MolecularOrbitalExplorerModule::OnEnter()");

    // Load shaders
    m_PointDensityShader = m_Engine.GetResourceManager().Load<GLShader>("shaders/geometry/point_density");
    m_ResolveShader      = m_Engine.GetResourceManager().Load<GLShader>("shaders/geometry/density_resolve");

    if (!m_PointDensityShader.IsValid() || !m_ResolveShader.IsValid()) {
        ORB_CORE_ERROR("MolecularOrbitalExplorerModule: Failed to load point or resolve shaders.");
    }

    m_ActiveWaveFunction = MolecularOrbitalFactory::Create(m_MOType, m_Separation);
    m_SamplingPipeline   = std::make_shared<MonteCarloSamplingPipeline>();
    
    m_ProbabilityCloud   = std::make_shared<StochasticProbabilityCloud>(
        m_Engine.GetResourceManager(), 
        m_ActiveWaveFunction, 
        m_SamplingPipeline
    );

    ReallocateFBO();

    m_DummyVAO  = std::make_unique<GLVertexArray>();
    m_PointsVAO = std::make_unique<GLVertexArray>();

    m_ProbabilityCloud->Resample(m_ActiveSampleCount);

    // Pre-cache energy sweep results for the visual graph
    m_CachedSweepResult = EnergySweep::RunSweep(0.5f, 10.0f, 0.05f);

    // Check if auto-generation is requested via environment variable
    if (const char* env = std::getenv("ORBITAL_GENERATE_VERIFICATION")) {
        if (std::string(env) == "1") {
            m_GenerateVerificationPackage = true;
            m_AutoExitAfterGeneration = true;
        }
    }
}

void MolecularOrbitalExplorerModule::OnExit()
{
    ORB_CORE_INFO("MolecularOrbitalExplorerModule::OnExit()");
    m_DummyVAO.reset();
    m_PointsVAO.reset();
    m_Framebuffer.reset();
    m_ProbabilityCloud.reset();
    m_SamplingPipeline.reset();
    m_ActiveWaveFunction.reset();
}

void MolecularOrbitalExplorerModule::Update(float dt)
{
    m_FrameTimeMs = dt * 1000.0f;
    m_FpsTimer += dt;
    m_FpsFrameCount++;
    if (m_FpsTimer >= 0.5f) {
        m_FPS = static_cast<float>(m_FpsFrameCount) / m_FpsTimer;
        m_FpsTimer = 0.0f;
        m_FpsFrameCount = 0;
    }

    if (m_PendingResample) {
        UpdateMOState();
        m_ProbabilityCloud->SetWaveFunction(m_ActiveWaveFunction);
        m_ProbabilityCloud->Resample(m_ActiveSampleCount);
        m_PendingResample = false;
    }
}

void MolecularOrbitalExplorerModule::UpdateMOState()
{
    m_ActiveWaveFunction = MolecularOrbitalFactory::Create(m_MOType, m_Separation);
    
    if (m_ExposureMode == ExposureMode::PerOrbital) {
        switch (m_MOType) {
            case MolecularOrbitalType::Sigma1s:
                m_Exposure = 2.6f;
                m_IntensityScale = 0.5f;
                m_ParticleSize = 4.0f;
                m_Contrast = 1.0f;
                break;
            case MolecularOrbitalType::Sigma1sStar:
                m_Exposure = 3.0f;
                m_IntensityScale = 0.5f;
                m_ParticleSize = 4.0f;
                m_Contrast = 1.2f;
                break;
            case MolecularOrbitalType::Sigma2p:
                m_Exposure = 3.0f;
                m_IntensityScale = 0.7f;
                m_ParticleSize = 4.0f;
                m_Contrast = 1.2f;
                break;
            case MolecularOrbitalType::Sigma2pStar:
                m_Exposure = 3.2f;
                m_IntensityScale = 0.7f;
                m_ParticleSize = 4.0f;
                m_Contrast = 1.2f;
                break;
            case MolecularOrbitalType::Pi2pX:
            case MolecularOrbitalType::Pi2pY:
                m_Exposure = 2.8f;
                m_IntensityScale = 0.6f;
                m_ParticleSize = 4.0f;
                m_Contrast = 1.1f;
                break;
            case MolecularOrbitalType::Pi2pXStar:
            case MolecularOrbitalType::Pi2pYStar:
                m_Exposure = 3.0f;
                m_IntensityScale = 0.6f;
                m_ParticleSize = 4.0f;
                m_Contrast = 1.1f;
                break;
        }
    }
}

void MolecularOrbitalExplorerModule::RenderFrameInternal()
{
    ReallocateFBO();
    uint32_t width = m_Engine.GetWindow().GetWidth();
    uint32_t height = m_Engine.GetWindow().GetHeight();

    m_Framebuffer->Bind();
    glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);

    auto* shader = m_Engine.GetResourceManager().Get(m_PointDensityShader);
    if (shader && m_ProbabilityCloud->GetPointCount() > 0) {
        shader->Bind();
        shader->SetUniform("u_ParticleSize", m_ParticleSize);
        shader->SetUniform("u_IntensityScale", m_IntensityScale);

        auto bufferHandle = m_ProbabilityCloud->GetGPUPointBuffer();
        GLBuffer* buffer = m_Engine.GetResourceManager().Get(bufferHandle);
        if (buffer) {
            VertexLayout layout;
            layout.stride = sizeof(SamplePoint);
            layout.attributes = {
                {0, 3, VertexAttributeType::Float, false, offsetof(SamplePoint, Position)},
                {1, 1, VertexAttributeType::Float, false, offsetof(SamplePoint, Density)},
                {2, 1, VertexAttributeType::Float, false, offsetof(SamplePoint, Phase)}
            };
            m_PointsVAO->SetVertexBuffer(*buffer, layout);
        }

        m_PointsVAO->Bind();
        glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(m_ProbabilityCloud->GetPointCount()));
        m_PointsVAO->Unbind();
        shader->Unbind();
    }

    glDisable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    m_Framebuffer->Unbind();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    auto* resolveShader = m_Engine.GetResourceManager().Get(m_ResolveShader);
    if (resolveShader) {
        resolveShader->Bind();
        resolveShader->SetUniform("u_Exposure", m_Exposure);
        resolveShader->SetUniform("u_Gamma", m_Gamma);
        resolveShader->SetUniform("u_Contrast", m_Contrast);

        const GLTexture& densityTexture = m_Framebuffer->GetColorAttachment(0);
        densityTexture.Bind(0);
        resolveShader->SetUniform("u_DensityTexture", 0);

        m_DummyVAO->Bind();
        glDrawArrays(GL_TRIANGLES, 0, 3);
        m_DummyVAO->Unbind();

        resolveShader->Unbind();
        GLTexture::Unbind(0);
    }
}

void MolecularOrbitalExplorerModule::Render()
{
    if (m_GenerateVerificationPackage) {
        m_GenerateVerificationPackage = false;

        float originalSeparation = m_Separation;
        MolecularOrbitalType originalType = m_MOType;
        uint32_t originalSampleCount = m_ActiveSampleCount;

        auto* controller = dynamic_cast<ArcballController*>(m_Engine.GetCameraManager().GetController());
        glm::vec3 originalPivot = {0.0f, 0.0f, 0.0f};
        float originalRadius = 5.0f;
        float originalTheta = 0.0f;
        float originalPhi = 0.52359f;
        if (controller) {
            originalPivot = controller->GetPivot();
            originalRadius = controller->GetRadius();
            originalTheta = controller->GetTheta();
            originalPhi = controller->GetPhi();
        }

        std::filesystem::create_directories("screenshots");

        struct CameraPreset {
            const char* name;
            float theta;
            float phi;
        };

        CameraPreset cameraPresets[] = {
            { "standard", glm::radians(45.0f), glm::radians(60.0f) },
            { "front",    glm::radians(90.0f), glm::radians(90.0f) },
            { "side",     glm::radians(0.0f),  glm::radians(90.0f) },
            { "top",      glm::radians(0.0f),  glm::radians(0.001f) }
        };

        MolecularOrbitalType statesToCapture[] = {
            MolecularOrbitalType::Sigma1s,
            MolecularOrbitalType::Sigma1sStar,
            MolecularOrbitalType::Sigma2p,
            MolecularOrbitalType::Sigma2pStar,
            MolecularOrbitalType::Pi2pX,
            MolecularOrbitalType::Pi2pXStar,
            MolecularOrbitalType::Pi2pY,
            MolecularOrbitalType::Pi2pYStar
        };

        const char* stateNames[] = {
            "sigma_1s",
            "sigma_1s_star",
            "sigma_2p",
            "sigma_2p_star",
            "pi_2px",
            "pi_2px_star",
            "pi_2py",
            "pi_2py_star"
        };

        double separationsToCapture[] = { 10.0, 6.0, 3.0, 1.5 };

        for (int i = 0; i < 8; ++i) {
            m_MOType = statesToCapture[i];
            
            for (double sep : separationsToCapture) {
                m_Separation = static_cast<float>(sep);
                UpdateMOState();

                m_ProbabilityCloud->SetWaveFunction(m_ActiveWaveFunction);
                m_ProbabilityCloud->Resample(m_ActiveSampleCount);

                for (const auto& preset : cameraPresets) {
                    if (controller) {
                        controller->SetViewPoint({0.0f, 0.0f, 0.0f}, 8.0f, preset.theta, preset.phi);
                    }

                    RenderFrameInternal();

                    // Read the backbuffer
                    uint32_t width = m_Engine.GetWindow().GetWidth();
                    uint32_t height = m_Engine.GetWindow().GetHeight();
                    std::vector<uint8_t> pixels(width * height * 3);
                    glPixelStorei(GL_PACK_ALIGNMENT, 1);
                    glFinish();
                    glReadPixels(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height), GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

                    char sepStr[16];
                    sprintf(sepStr, "%.1f", sep);
                    std::string filename = "screenshots/hydrogen_" + std::string(stateNames[i]) + "_" + sepStr + "bohr_" + std::string(preset.name) + ".bmp";
                    if (SaveBMP(filename, static_cast<int>(width), static_cast<int>(height), pixels.data())) {
                        ORB_CORE_INFO("Saved orbital screenshot: {}", filename);
                    } else {
                        ORB_CORE_ERROR("Failed to save orbital screenshot: {}", filename);
                    }
                }
            }
        }

        m_Separation = originalSeparation;
        m_MOType = originalType;
        m_ActiveSampleCount = originalSampleCount;
        UpdateMOState();

        m_ProbabilityCloud->SetWaveFunction(m_ActiveWaveFunction);
        m_ProbabilityCloud->Resample(m_ActiveSampleCount);

        if (controller) {
            controller->SetViewPoint(originalPivot, originalRadius, originalTheta, originalPhi);
        }

        if (m_AutoExitAfterGeneration) {
            ORB_CORE_INFO("Auto-exit requested after generating verification package. Requesting shutdown...");
            m_Engine.RequestShutdown();
        }
    } else {
        RenderFrameInternal();
    }
}

void MolecularOrbitalExplorerModule::OnParameterPanel()
{
    ImGui::Begin("Molecular Orbital Explorer");

    ImGui::Text("Current Module:");
    ImGui::BulletText("Hydrogen Orbital Explorer");
    ImGui::BulletText("Molecular Orbital Explorer (ACTIVE)");
    ImGui::Separator();

    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Model: LCAO Approximation (H₂⁺-style Educational Model)");
    ImGui::Separator();

    // Compute real-time overlap and energies
    HydrogenicOrbital orbitalA(1, 0, 0, 0.5, false);
    HydrogenicOrbital orbitalB(1, 0, 0, 0.5, false);
    double S_analytical = OverlapIntegral::ComputeAnalytical1s(m_Separation);
    double S_numerical = OverlapIntegral::ComputeNumerical(orbitalA, orbitalB, m_Separation, 60);
    double absError = std::abs(S_analytical - S_numerical);

    EnergyResult energyResults = MolecularEnergyModel::ComputeEnergies(m_Separation, S_analytical);

    // LCAO Selection
    ImGui::Text("Molecular Orbital (H2)");
    bool stateChanged = false;
    
    const char* moNames[] = {
        "σ(1s) - Bonding",
        "σ*(1s) - Antibonding",
        "σ(2p) - Bonding",
        "σ*(2p) - Antibonding",
        "π(2p_x) - Bonding",
        "π*(2p_x) - Antibonding",
        "π(2p_y) - Bonding",
        "π*(2p_y) - Antibonding"
    };
    int currentMoIdx = static_cast<int>(m_MOType);
    if (ImGui::Combo("Orbital Select", &currentMoIdx, moNames, 8)) {
        m_MOType = static_cast<MolecularOrbitalType>(currentMoIdx);
        UpdateMOState();
        stateChanged = true;
    }

    if (ImGui::SliderFloat("Separation (Bohr)", &m_Separation, 0.2f, 10.0f, "%.2f r_0")) {
        stateChanged = true;
    }

    // Overlap Display
    ImGui::Text("Overlap Integrals:");
    ImGui::BulletText("Analytical overlap S(R): %.6f", S_analytical);
    ImGui::BulletText("Numerical overlap S(R):  %.6f (60^3 grid)", S_numerical);
    ImGui::BulletText("Absolute Error |S_a - S_n|: %.6e", absError);

    ImGui::Spacing();
    ImGui::Text("Molecular Orbital Energies:");
    ImGui::BulletText("Bonding Energy (E+):      %.4f eV", energyResults.bondingEnergy);
    ImGui::BulletText("Antibonding Energy (E-):  %.4f eV", energyResults.antibondingEnergy);
    ImGui::BulletText("HAA (Coulomb Integral):    %.4f eV", energyResults.HAA);
    ImGui::BulletText("HAB (Resonance Integral):  %.4f eV", energyResults.HAB);
    
    // Estimated bond order
    bool isBonding = (m_MOType == MolecularOrbitalType::Sigma1s || 
                      m_MOType == MolecularOrbitalType::Sigma2p ||
                      m_MOType == MolecularOrbitalType::Pi2pX ||
                      m_MOType == MolecularOrbitalType::Pi2pY);
    double orbitalBondOrder = isBonding ? 0.5 : -0.5;
    ImGui::BulletText("Orbital Bond Order Contrib: %+.1f", orbitalBondOrder);
    if (m_MOType == MolecularOrbitalType::Sigma1s || m_MOType == MolecularOrbitalType::Sigma1sStar) {
        ImGui::BulletText("Estimated H2 Bond Order:    %.1f (if σ(1s) is filled with 2e-)", 
                          (m_MOType == MolecularOrbitalType::Sigma1s) ? 1.0 : 0.0);
    }

    // Sample count selector
    const char* sampleCountLabels[] = { "100k", "250k", "500k", "1M" };
    uint32_t sampleCountValues[] = { 100000, 250000, 500000, 1000000 };
    if (ImGui::Combo("Sample Count", &m_SelectedSampleCountIdx, sampleCountLabels, 4)) {
        m_ActiveSampleCount = sampleCountValues[m_SelectedSampleCountIdx];
        stateChanged = true;
    }

    // Swappable exposure mode selector
    const char* exposureModeLabels[] = { "Manual", "PerOrbital (Auto)", "HistogramBased" };
    int expModeIdx = static_cast<int>(m_ExposureMode);
    if (ImGui::Combo("Exposure Mode", &expModeIdx, exposureModeLabels, 3)) {
        m_ExposureMode = static_cast<ExposureMode>(expModeIdx);
        if (m_ExposureMode == ExposureMode::PerOrbital) {
            UpdateMOState();
        }
    }

    if (stateChanged) {
        m_PendingResample = true;
    }

    ImGui::Separator();

    // Educational Metadata
    ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "Educational Metadata");
    ImGui::Spacing();
    
    std::string label = m_ActiveWaveFunction->GetLabel();
    std::string desc = m_ActiveWaveFunction->GetDescription();
    int energyOrder = m_ActiveWaveFunction->GetEnergyOrdering();
    double midpointDensity = m_ActiveWaveFunction->ProbabilityDensity(glm::vec3(0.0f));

    const char* orbitalTypeStr = "";
    const char* classificationStr = isBonding ? "Bonding" : "Antibonding";
    const char* nodalStructureStr = "";
    int nodalPlanesCount = 0;

    switch (m_MOType) {
        case MolecularOrbitalType::Sigma1s:
            orbitalTypeStr = "σ (Sigma)";
            nodalStructureStr = "No nodal planes";
            nodalPlanesCount = 0;
            break;
        case MolecularOrbitalType::Sigma1sStar:
            orbitalTypeStr = "σ* (Sigma Antibonding)";
            nodalStructureStr = "1 vertical nodal plane (x=0)";
            nodalPlanesCount = 1;
            break;
        case MolecularOrbitalType::Sigma2p:
            orbitalTypeStr = "σ (Sigma)";
            nodalStructureStr = "2 nodal planes (at atomic nuclei: x = ±R/2)";
            nodalPlanesCount = 2;
            break;
        case MolecularOrbitalType::Sigma2pStar:
            orbitalTypeStr = "σ* (Sigma Antibonding)";
            nodalStructureStr = "3 nodal planes (x=0 and atomic nuclei: x = ±R/2)";
            nodalPlanesCount = 3;
            break;
        case MolecularOrbitalType::Pi2pX:
            orbitalTypeStr = "π (Pi)";
            nodalStructureStr = "1 nodal plane containing bond axis (z=0)";
            nodalPlanesCount = 1;
            break;
        case MolecularOrbitalType::Pi2pXStar:
            orbitalTypeStr = "π* (Pi Antibonding)";
            nodalStructureStr = "2 nodal planes (z=0 containing bond axis, and x=0)";
            nodalPlanesCount = 2;
            break;
        case MolecularOrbitalType::Pi2pY:
            orbitalTypeStr = "π (Pi)";
            nodalStructureStr = "1 nodal plane containing bond axis (y=0)";
            nodalPlanesCount = 1;
            break;
        case MolecularOrbitalType::Pi2pYStar:
            orbitalTypeStr = "π* (Pi Antibonding)";
            nodalStructureStr = "2 nodal planes (y=0 containing bond axis, and x=0)";
            nodalPlanesCount = 2;
            break;
    }
    
    ImGui::Text("Orbital Name: %s", label.c_str());
    ImGui::Text("Orbital Type: %s", orbitalTypeStr);
    ImGui::Text("Classification: %s", classificationStr);
    ImGui::Text("Expected Nodal Structure: %s (planes count: %d)", nodalStructureStr, nodalPlanesCount);
    ImGui::Text("Midpoint Density (x=0): %.6e", midpointDensity);
    ImGui::Text("Relative Energy Level: %d (lower is more stable)", energyOrder);
    ImGui::Text("Bond Order Contribution: %s%.1f", (orbitalBondOrder > 0.0) ? "+" : "", orbitalBondOrder);
    
    ImGui::Spacing();
    ImGui::TextWrapped("Description: %s", desc.c_str());

    ImGui::Separator();

    // ─────────────────────────────────────────────────────────────────────────
    // Energy Curve Explorer Section
    // ─────────────────────────────────────────────────────────────────────────
    if (ImGui::CollapsingHeader("Energy Curve Explorer", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::TextWrapped("Potential energy curves showing bonding and antibonding states. Click/drag on the graph to adjust separation.");

        // Custom canvas draw code
        ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
        ImVec2 canvas_size = ImVec2(ImGui::GetContentRegionAvail().x, 220.0f);
        if (canvas_size.x < 50.0f) canvas_size.x = 50.0f;

        ImGui::InvisibleButton("energy_canvas", canvas_size);
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        // Draw canvas background and border
        draw_list->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), IM_COL32(20, 20, 20, 255), 4.0f);
        draw_list->AddRect(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), IM_COL32(60, 60, 60, 255), 4.0f);

        // Padding/margins for axes
        float pad_left = 40.0f, pad_right = 15.0f, pad_top = 20.0f, pad_bottom = 25.0f;
        float plot_x0 = canvas_pos.x + pad_left;
        float plot_y0 = canvas_pos.y + pad_top;
        float plot_x1 = canvas_pos.x + canvas_size.x - pad_right;
        float plot_y1 = canvas_pos.y + canvas_size.y - pad_bottom;
        float plot_w = plot_x1 - plot_x0;
        float plot_h = plot_y1 - plot_y0;

        float y_min = -25.0f;
        float y_max = 25.0f;

        auto MapCoords = [&](float R, float E) -> ImVec2 {
            float t_x = (R - 0.5f) / (10.0f - 0.5f);
            float E_clamped = std::clamp(E, y_min, y_max);
            float t_y = (E_clamped - y_min) / (y_max - y_min);
            return ImVec2(plot_x0 + t_x * plot_w, plot_y1 - t_y * plot_h);
        };

        // Handle user interaction (dragging/clicking on canvas)
        if (ImGui::IsItemActive() && plot_w > 0.0f) {
            ImVec2 mouse_pos = ImGui::GetMousePos();
            float relative_x = (mouse_pos.x - plot_x0) / plot_w;
            float new_sep = 0.5f + relative_x * (10.0f - 0.5f);
            m_Separation = std::clamp(new_sep, 0.5f, 10.0f);
            stateChanged = true;
        }

        // Draw horizontal grid lines and labels
        for (float e_val = -20.0f; e_val <= 20.0f; e_val += 10.0f) {
            ImVec2 pt_left = MapCoords(0.5f, e_val);
            ImVec2 pt_right = MapCoords(10.0f, e_val);
            draw_list->AddLine(pt_left, pt_right, IM_COL32(50, 50, 50, 255), 1.0f);
            
            char buf[32];
            sprintf(buf, "%d", static_cast<int>(e_val));
            draw_list->AddText(ImVec2(canvas_pos.x + 5.0f, pt_left.y - 7.0f), IM_COL32(180, 180, 180, 255), buf);
        }

        // Draw separation grid lines and labels
        for (float sep_val = 1.0f; sep_val <= 10.0f; sep_val += 1.0f) {
            ImVec2 pt_top = MapCoords(sep_val, y_max);
            ImVec2 pt_bottom = MapCoords(sep_val, y_min);
            draw_list->AddLine(pt_top, pt_bottom, IM_COL32(50, 50, 50, 255), 1.0f);
            
            char buf[32];
            sprintf(buf, "%.0f", sep_val);
            draw_list->AddText(ImVec2(pt_bottom.x - 4.0f, plot_y1 + 5.0f), IM_COL32(180, 180, 180, 255), buf);
        }

        // Draw Separated H(1s) Limit (-13.6 eV) dashed line
        ImVec2 pt_limit_left = MapCoords(0.5f, -13.6057f);
        ImVec2 pt_limit_right = MapCoords(10.0f, -13.6057f);
        float dash_step = 6.0f;
        for (float dx = pt_limit_left.x; dx < pt_limit_right.x; dx += dash_step * 2.0f) {
            float next_dx = std::min(dx + dash_step, pt_limit_right.x);
            draw_list->AddLine(ImVec2(dx, pt_limit_left.y), ImVec2(next_dx, pt_limit_left.y), IM_COL32(120, 120, 120, 255), 1.5f);
        }
        draw_list->AddText(ImVec2(pt_limit_left.x + 10.0f, pt_limit_left.y - 15.0f), IM_COL32(150, 150, 150, 255), "H(1s) Limit (-13.6 eV)");

        // Draw axes labels
        draw_list->AddText(ImVec2(canvas_pos.x + 5.0f, canvas_pos.y + 2.0f), IM_COL32(220, 220, 220, 255), "Energy (eV)");
        draw_list->AddText(ImVec2(plot_x1 - 65.0f, plot_y1 - 18.0f), IM_COL32(220, 220, 220, 255), "R (Bohr)");

        // Draw curves with clipping enabled
        draw_list->PushClipRect(ImVec2(plot_x0, plot_y0), ImVec2(plot_x1, plot_y1), true);

        for (size_t i = 0; i < m_CachedSweepResult.points.size() - 1; ++i) {
            const auto& pt0 = m_CachedSweepResult.points[i];
            const auto& pt1 = m_CachedSweepResult.points[i+1];

            // Cyan for bonding curve
            draw_list->AddLine(MapCoords(pt0.separation, static_cast<float>(pt0.bondingEnergy)),
                               MapCoords(pt1.separation, static_cast<float>(pt1.bondingEnergy)),
                               IM_COL32(0, 220, 220, 255), 2.5f);

            // Crimson/Orange for antibonding curve
            draw_list->AddLine(MapCoords(pt0.separation, static_cast<float>(pt0.antibondingEnergy)),
                               MapCoords(pt1.separation, static_cast<float>(pt1.antibondingEnergy)),
                               IM_COL32(255, 80, 80, 255), 2.5f);
        }

        draw_list->PopClipRect();

        // Draw live separation marker
        float marker_x = plot_x0 + (m_Separation - 0.5f) / (10.0f - 0.5f) * plot_w;
        draw_list->AddLine(ImVec2(marker_x, plot_y0), ImVec2(marker_x, plot_y1), IM_COL32(200, 200, 200, 150), 1.0f);

        ImVec2 p_bond = MapCoords(m_Separation, static_cast<float>(energyResults.bondingEnergy));
        ImVec2 p_anti = MapCoords(m_Separation, static_cast<float>(energyResults.antibondingEnergy));

        draw_list->AddCircleFilled(p_bond, 5.0f, IM_COL32(0, 255, 255, 255));
        draw_list->AddCircle(p_bond, 5.0f, IM_COL32(255, 255, 255, 255), 0, 1.5f);

        draw_list->AddCircleFilled(p_anti, 5.0f, IM_COL32(255, 80, 80, 255));
        draw_list->AddCircle(p_anti, 5.0f, IM_COL32(255, 255, 255, 255), 0, 1.5f);

        // Display current information text
        ImGui::Spacing();
        ImGui::Columns(2, "sweep_columns", false);
        ImGui::Text("Current Separation: %.2f Bohr", m_Separation);
        ImGui::TextColored(ImVec4(0.0f, 0.9f, 0.9f, 1.0f), "Bonding (E+):      %.4f eV", energyResults.bondingEnergy);
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Antibonding (E-):  %.4f eV", energyResults.antibondingEnergy);

        ImGui::NextColumn();
        ImGui::Text("Equilibrium Sep:    %.2f Bohr", m_CachedSweepResult.equilibriumSeparation);
        ImGui::Text("Min Bonding Energy: %.4f eV", m_CachedSweepResult.minimumBondingEnergy);
        ImGui::Columns(1);

        // Educational Content
        ImGui::Spacing();
        if (ImGui::CollapsingHeader("Potential Energy Curve Explanation", ImGuiTreeNodeFlags_None)) {
            ImGui::TextWrapped("**Why bonding energy reaches a minimum (equilibrium):**\n"
                               "As nuclear separation R decreases, constructive interference between atomic wavefunctions "
                               "increases electron density in the internuclear region. This shared charge density shields "
                               "the positive nuclei, reducing their mutual repulsion and stabilizing the molecule (E+ drops). "
                               "At very short distances (R < 2.0 Bohr), the positive-positive proton Coulombic repulsion (proportional to 1/R) "
                               "grows faster than electronic stabilization. The minimum energy point represents the stable equilibrium "
                               "separation (bond length) where these forces balance.");
            ImGui::Spacing();
            ImGui::TextWrapped("**Why antibonding energy remains higher:**\n"
                               "The antisymmetric LCAO combination has a vertical nodal plane at the midpoint x = 0 where "
                               "electron density is strictly zero. Because negative charge density is suppressed between the nuclei, "
                               "there is no electrostatic shielding to counteract the proton-proton repulsion. Consequently, the "
                               "potential energy is strictly repulsive and remains higher than the separated atom limit (-13.6 eV) "
                               "at all separations.");
            ImGui::Spacing();
            ImGui::TextWrapped("**Relationship between overlap and stabilization:**\n"
                               "A larger overlap integral S represents greater spatial sharing of electrons between the nuclei. "
                               "As overlap increases, the resonance coupling term (HAB) becomes more negative, which shifts "
                               "the bonding energy lower (stabilization) and pushes the antibonding energy higher (destabilization).");
        }
    }

    ImGui::Separator();

    // Energy Curve Sweep Section
    ImGui::Text("Energy Curve Sweep");
    if (ImGui::Button("Generate Energy Curve")) {
        SweepResult sweep = EnergySweep::RunSweep(0.5f, 10.0f, 0.05f);
        m_HasSweepResults = EnergySweep::ExportToCSV(sweep, "exports/h2_energy_curve.csv");
        if (m_HasSweepResults) {
            m_SweepEquilibriumSep = sweep.equilibriumSeparation;
            m_SweepMinBondingEnergy = sweep.minimumBondingEnergy;
            m_SweepExportPath = "exports/h2_energy_curve.csv";
        }
    }
    if (m_HasSweepResults) {
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Sweep Completed!");
        ImGui::Text("Equilibrium Separation:  %.2f Bohr", m_SweepEquilibriumSep);
        ImGui::Text("Minimum Bonding Energy:  %.4f eV", m_SweepMinBondingEnergy);
        ImGui::Text("Exported CSV: %s", m_SweepExportPath.c_str());
    }

    ImGui::Separator();

    // Camera Presets
    ImGui::Text("Camera Presets");
    auto* controller = dynamic_cast<ArcballController*>(m_Engine.GetCameraManager().GetController());
    if (controller) {
        if (ImGui::Button("Standard View")) {
            controller->SetViewPoint({0.0f, 0.0f, 0.0f}, 8.0f, glm::radians(45.0f), glm::radians(60.0f));
        }
        ImGui::SameLine();
        if (ImGui::Button("Top (XZ)")) {
            controller->SetViewPoint({0.0f, 0.0f, 0.0f}, 8.0f, glm::radians(0.0f), glm::radians(0.001f));
        }
        ImGui::SameLine();
        if (ImGui::Button("Front (XY)")) {
            controller->SetViewPoint({0.0f, 0.0f, 0.0f}, 8.0f, glm::radians(90.0f), glm::radians(90.0f));
        }
        ImGui::SameLine();
        if (ImGui::Button("Side (YZ)")) {
            controller->SetViewPoint({0.0f, 0.0f, 0.0f}, 8.0f, glm::radians(0.0f), glm::radians(90.0f));
        }
    }

    ImGui::Separator();

    // Visual configurations
    ImGui::Text("Visualization Options");
    ImGui::SliderFloat("Particle Size", &m_ParticleSize, 1.0f, 16.0f);
    ImGui::SliderFloat("Intensity Scale", &m_IntensityScale, 0.01f, 2.0f);
    ImGui::SliderFloat("Exposure", &m_Exposure, 0.1f, 5.0f);
    ImGui::SliderFloat("Contrast", &m_Contrast, 0.1f, 3.0f);
    ImGui::SliderFloat("Gamma Correction", &m_Gamma, 1.0f, 3.0f);

    ImGui::Separator();

    // Instrumentation metrics
    ImGui::Text("Lightweight Instrumentation System");
    ImGui::Text("FPS: %.1f", m_FPS);
    ImGui::Text("Frame Time: %.2f ms", m_FrameTimeMs);
    ImGui::Text("Active Samples: %u", m_ProbabilityCloud ? m_ProbabilityCloud->GetPointCount() : 0);
    ImGui::Text("Sample Gen Time: %.2f ms", m_ProbabilityCloud ? m_ProbabilityCloud->GetSampleGenTimeMs() : 0.0);
    ImGui::Text("GPU Upload Time: %.2f ms", m_ProbabilityCloud ? m_ProbabilityCloud->GetGpuUploadTimeMs() : 0.0);

    // Sampler Strategy Information
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.9f, 0.5f, 0.1f, 1.0f), "Strategy: Metropolis-Hastings MCMC");
    ImGui::TextWrapped("%s", "Running stochastic Markov random walk proposals to sample molecular LCAO shapes.");

    ImGui::Separator();
    if (ImGui::Button("Generate Bonding Screenshots")) {
        m_GenerateVerificationPackage = true;
    }

    ImGui::Spacing();
    if (ImGui::CollapsingHeader("LCAO Molecular Orbital Theory Info", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::TextWrapped("**Why Overlap Creates Bonding:**\n"
                           "In-phase overlap (S > 0) of the atomic 1s orbitals leads to constructive interference. "
                           "This increases electron probability density between the two nuclei, which shields the positive nuclei from one another "
                           "and acts as an electrostatic glue holding the molecule together, lowering the energy (E+ < -13.6 eV).");
        ImGui::Spacing();
        ImGui::TextWrapped("**Why Antibonding Raises Energy:**\n"
                           "Out-of-phase overlap (destructive interference) results in a nodal plane between the nuclei where "
                           "the electron density is strictly zero. The lack of electron shielding exposes the nuclei to direct, "
                           "unmitigated Coulombic proton-proton repulsion, raising the energy (E- > -13.6 eV).");
        ImGui::Spacing();
        ImGui::TextWrapped("**How Equilibrium Bond Length Emerges:**\n"
                           "At large separations, the interaction is near zero. As the nuclei approach, bonding energy drops "
                           "due to electron sharing. However, at very short distances, the nuclear-nuclear Coulomb repulsion (1/R) "
                           "grows faster than the bonding stabilization, creating a sharp energy barrier. The equilibrium bond length "
                           "is the precise point (around 2.5 Bohr for H₂⁺) where these opposing forces balance, minimizing the total energy.");
    }

    ImGui::End();
}

void MolecularOrbitalExplorerModule::ReallocateFBO()
{
    uint32_t width = m_Engine.GetWindow().GetWidth();
    uint32_t height = m_Engine.GetWindow().GetHeight();

    if (!m_Framebuffer || m_Framebuffer->GetWidth() != width || m_Framebuffer->GetHeight() != height) {
        m_Framebuffer = std::make_unique<GLFramebuffer>(width, height);
        m_Framebuffer->AddColorAttachment(TextureFormat::R32F);
        
        bool ok = m_Framebuffer->Build();
        if (!ok) {
            ORB_CORE_ERROR("MolecularOrbitalExplorerModule: Failed to build framebuffer ({}x{})", width, height);
        } else {
            ORB_CORE_TRACE("MolecularOrbitalExplorerModule: Resized framebuffer to {}x{}", width, height);
        }
    }
}

} // namespace Orbital
