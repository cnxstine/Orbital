#include "visualization/OrbitalViewerModule.hpp"
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
#include "resources/loaders/ShaderLoader.hpp"
#include "core/Log.hpp"
#include <filesystem>
#include <fstream>
#include <vector>

#include <imgui.h>
#include <glad/gl.h>
#include <chrono>

namespace Orbital {

OrbitalViewerModule::OrbitalViewerModule(Engine& engine)
    : m_Engine(engine)
{
}

void OrbitalViewerModule::OnEnter()
{
    ORB_CORE_INFO("OrbitalViewerModule::OnEnter()");

    // Load shaders
    m_PointDensityShader = m_Engine.GetResourceManager().Load<GLShader>("shaders/geometry/point_density");
    m_ResolveShader      = m_Engine.GetResourceManager().Load<GLShader>("shaders/geometry/density_resolve");

    if (!m_PointDensityShader.IsValid() || !m_ResolveShader.IsValid()) {
        ORB_CORE_ERROR("OrbitalViewerModule: Failed to load point or resolve shaders.");
    }

    // Setup active orbital and sampling pipeline
    m_ActiveOrbital    = std::make_shared<HydrogenicOrbital>(m_QuantumN, m_QuantumL, m_QuantumM, 0.5, m_IsRealCombo);
    m_SamplingPipeline = std::make_shared<MonteCarloSamplingPipeline>();
    
    // Create the stochastic point cloud
    m_ProbabilityCloud = std::make_shared<StochasticProbabilityCloud>(
        m_Engine.GetResourceManager(), 
        m_ActiveOrbital, 
        m_SamplingPipeline
    );

    // Initial allocate for Framebuffer
    ReallocateFBO();

    // Create VAOs
    m_DummyVAO  = std::make_unique<GLVertexArray>();
    m_PointsVAO = std::make_unique<GLVertexArray>();

    // Trigger initial sampling
    m_ProbabilityCloud->Resample(m_ActiveSampleCount);

    // Check if auto-generation is requested via environment variable
    if (const char* env = std::getenv("ORBITAL_GENERATE_VERIFICATION")) {
        if (std::string(env) == "1") {
            m_GenerateVerificationPackage = true;
            m_AutoExitAfterGeneration = true;
        }
    }
}

void OrbitalViewerModule::OnExit()
{
    ORB_CORE_INFO("OrbitalViewerModule::OnExit()");
    m_DummyVAO.reset();
    m_PointsVAO.reset();
    m_Framebuffer.reset();
    m_ProbabilityCloud.reset();
    m_SamplingPipeline.reset();
    m_ActiveOrbital.reset();
}

void OrbitalViewerModule::Update(float dt)
{
    // FPS counter calculation
    m_FrameTimeMs = dt * 1000.0f;
    m_FpsTimer += dt;
    m_FpsFrameCount++;
    if (m_FpsTimer >= 0.5f) {
        m_FPS = static_cast<float>(m_FpsFrameCount) / m_FpsTimer;
        m_FpsTimer = 0.0f;
        m_FpsFrameCount = 0;
    }

    // Rebuild/Resample if parameters changed
    if (m_PendingResample) {
        m_ActiveOrbital = std::make_shared<HydrogenicOrbital>(m_QuantumN, m_QuantumL, m_QuantumM, 0.5, m_IsRealCombo);
        m_ProbabilityCloud->SetWaveFunction(m_ActiveOrbital);
        m_ProbabilityCloud->Resample(m_ActiveSampleCount);
        m_PendingResample = false;
    }
}

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

void OrbitalViewerModule::UpdateOrbitalStateFromSelection()
{
    switch (m_SelectedState) {
        case SelectedState::H_1s:
            m_QuantumN = 1; m_QuantumL = 0; m_QuantumM = 0; m_IsRealCombo = false;
            break;
        case SelectedState::H_2s:
            m_QuantumN = 2; m_QuantumL = 0; m_QuantumM = 0; m_IsRealCombo = false;
            break;
        case SelectedState::H_2p_x:
            m_QuantumN = 2; m_QuantumL = 1; m_QuantumM = 1; m_IsRealCombo = true;
            break;
        case SelectedState::H_2p_y:
            m_QuantumN = 2; m_QuantumL = 1; m_QuantumM = -1; m_IsRealCombo = true;
            break;
        case SelectedState::H_2p_z:
            m_QuantumN = 2; m_QuantumL = 1; m_QuantumM = 0; m_IsRealCombo = true;
            break;
    }
}

void OrbitalViewerModule::RenderFrameInternal()
{
    ReallocateFBO();
    uint32_t width = m_Engine.GetWindow().GetWidth();
    uint32_t height = m_Engine.GetWindow().GetHeight();

    // ── Pass 1: Accumulate point density into floating-point framebuffer ──────
    m_Framebuffer->Bind();
    glViewport(0, 0, width, height);

    // Clear accumulated density channel to 0.0
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Additive blending, no depth writing
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glDisable(GL_DEPTH_TEST);
    
    // Enable shader point size
    glEnable(GL_PROGRAM_POINT_SIZE);

    auto* shader = m_Engine.GetResourceManager().Get(m_PointDensityShader);
    if (shader && m_ProbabilityCloud->GetPointCount() > 0) {
        shader->Bind();
        shader->SetUniform("u_ParticleSize", m_ParticleSize);
        shader->SetUniform("u_IntensityScale", m_IntensityScale);

        // Bind GPU buffer containing SamplePoints to the points VAO
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
        glDrawArrays(GL_POINTS, 0, m_ProbabilityCloud->GetPointCount());
        m_PointsVAO->Unbind();
        shader->Unbind();
    }

    // Restore GL states
    glDisable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    m_Framebuffer->Unbind();

    // ── Pass 2: Resolve accumulated density texture to tone-mapped color ──────
    // Clear backbuffer to solid black
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    auto* resolveShader = m_Engine.GetResourceManager().Get(m_ResolveShader);
    if (resolveShader) {
        resolveShader->Bind();
        resolveShader->SetUniform("u_Exposure", m_Exposure);
        resolveShader->SetUniform("u_Gamma", m_Gamma);

        // Bind the density texture
        const GLTexture& densityTexture = m_Framebuffer->GetColorAttachment(0);
        densityTexture.Bind(0);
        resolveShader->SetUniform("u_DensityTexture", 0);

        // Draw fullscreen quad using zero-VBO technique
        m_DummyVAO->Bind();
        glDrawArrays(GL_TRIANGLES, 0, 3);
        m_DummyVAO->Unbind();

        resolveShader->Unbind();
        GLTexture::Unbind(0);
    }
}

void OrbitalViewerModule::Render()
{
    if (m_GenerateVerificationPackage) {
        m_GenerateVerificationPackage = false;

        // Save original states
        SelectedState originalState = m_SelectedState;
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

            // Set standardized viewpoint for captures: pivot=0, radius=8, theta=45 deg, phi=60 deg
            controller->SetViewPoint({0.0f, 0.0f, 0.0f}, 8.0f, glm::radians(45.0f), glm::radians(60.0f));
        }

        std::filesystem::create_directories("screenshots");

        SelectedState statesToCapture[] = {
            SelectedState::H_1s,
            SelectedState::H_2s,
            SelectedState::H_2p_x,
            SelectedState::H_2p_y,
            SelectedState::H_2p_z
        };
        const char* filenames[] = {
            "screenshots/hydrogen_1s.bmp",
            "screenshots/hydrogen_2s.bmp",
            "screenshots/hydrogen_2px.bmp",
            "screenshots/hydrogen_2py.bmp",
            "screenshots/hydrogen_2pz.bmp"
        };

        for (int i = 0; i < 5; ++i) {
            m_SelectedState = statesToCapture[i];
            UpdateOrbitalStateFromSelection();

            m_ActiveOrbital = std::make_shared<HydrogenicOrbital>(m_QuantumN, m_QuantumL, m_QuantumM, 0.5, m_IsRealCombo);
            m_ProbabilityCloud->SetWaveFunction(m_ActiveOrbital);
            m_ProbabilityCloud->Resample(m_ActiveSampleCount);

            RenderFrameInternal();

            // Read the backbuffer
            uint32_t width = m_Engine.GetWindow().GetWidth();
            uint32_t height = m_Engine.GetWindow().GetHeight();
            std::vector<uint8_t> pixels(width * height * 3);
            glPixelStorei(GL_PACK_ALIGNMENT, 1);
            glFinish(); // Ensure all GPU rendering commands have completed
            glReadPixels(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height), GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

            if (SaveBMP(filenames[i], static_cast<int>(width), static_cast<int>(height), pixels.data())) {
                ORB_CORE_INFO("Saved orbital screenshot: {}", filenames[i]);
            } else {
                ORB_CORE_ERROR("Failed to save orbital screenshot: {}", filenames[i]);
            }
        }

        // Restore original states
        m_SelectedState = originalState;
        m_ActiveSampleCount = originalSampleCount;
        UpdateOrbitalStateFromSelection();

        m_ActiveOrbital = std::make_shared<HydrogenicOrbital>(m_QuantumN, m_QuantumL, m_QuantumM, 0.5, m_IsRealCombo);
        m_ProbabilityCloud->SetWaveFunction(m_ActiveOrbital);
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

void OrbitalViewerModule::OnParameterPanel()
{
    ImGui::Begin("Quantum Chemistry & Performance Controls");

    // Quantum Chemistry Selection
    ImGui::Text("Quantum State Selector");
    bool qnChanged = false;
    
    const char* orbitalNames[] = { "Hydrogen 1s", "Hydrogen 2s", "Hydrogen 2p_x", "Hydrogen 2p_y", "Hydrogen 2p_z" };
    int currentIdx = static_cast<int>(m_SelectedState);
    if (ImGui::Combo("Orbital State", &currentIdx, orbitalNames, 5)) {
        m_SelectedState = static_cast<SelectedState>(currentIdx);
        UpdateOrbitalStateFromSelection();
        qnChanged = true;
    }

    // Swappable sample count selector
    const char* sampleCountLabels[] = { "100k", "250k", "500k", "1M" };
    uint32_t sampleCountValues[] = { 100000, 250000, 500000, 1000000 };
    if (ImGui::Combo("Sample Count", &m_SelectedSampleCountIdx, sampleCountLabels, 4)) {
        m_ActiveSampleCount = sampleCountValues[m_SelectedSampleCountIdx];
        qnChanged = true;
    }

    if (qnChanged) {
        m_PendingResample = true;
    }

    ImGui::Separator();

    // Educational Metadata
    ImGui::Text("Educational Metadata");
    
    int totalNodes = m_QuantumN - 1;
    int radialNodes = m_QuantumN - m_QuantumL - 1;
    int angularNodes = m_QuantumL;
    const char* orbitalType = (m_QuantumL == 0) ? "s-type" : (m_QuantumL == 1) ? "p-type" : "other";

    ImGui::BulletText("Name: %s", (m_SelectedState == SelectedState::H_1s) ? "1s" :
                                 (m_SelectedState == SelectedState::H_2s) ? "2s" :
                                 (m_SelectedState == SelectedState::H_2p_x) ? "2p_x" :
                                 (m_SelectedState == SelectedState::H_2p_y) ? "2p_y" : "2p_z");
    ImGui::BulletText("Orbital Type: %s", orbitalType);
    ImGui::BulletText("Quantum Numbers: (n=%d, l=%d, m=%d)", m_QuantumN, m_QuantumL, m_QuantumM);
    ImGui::BulletText("Nodes: Total=%d (Radial=%d, Angular=%d)", totalNodes, radialNodes, angularNodes);
    
    ImGui::Spacing();
    ImGui::TextWrapped("Description: %s", 
        (m_SelectedState == SelectedState::H_1s) ? "The ground state of the hydrogen atom. It is spherically symmetric, with no nodes, and the probability density decays exponentially with distance from the nucleus." :
        (m_SelectedState == SelectedState::H_2s) ? "An excited state with spherical symmetry. It features a single radial node at r = 2 Bohr radii, resulting in a visible low-density gap between the inner core and outer shell." :
        (m_SelectedState == SelectedState::H_2p_x) ? "A real linear combination of the 2p states. It forms a dumbbell shape aligned along the X-axis, with a planar angular node at x = 0 (the YZ-plane)." :
        (m_SelectedState == SelectedState::H_2p_y) ? "A real linear combination of the 2p states. It forms a dumbbell shape aligned along the Y-axis, with a planar angular node at y = 0 (the XZ-plane)." :
        "The standard angular momentum state aligned along the Z-axis. It forms a dumbbell shape with a planar angular node at z = 0 (the XY-plane).");

    ImGui::Separator();

    // Visual configurations
    ImGui::Text("Visualization Options");
    ImGui::SliderFloat("Particle Size", &m_ParticleSize, 1.0f, 16.0f);
    ImGui::SliderFloat("Intensity Scale", &m_IntensityScale, 0.01f, 2.0f);
    ImGui::SliderFloat("Exposure", &m_Exposure, 0.1f, 5.0f);
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
    if (m_QuantumN <= 2) {
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "Strategy: Direct Analytical Sampling");
        ImGui::TextWrapped("%s", "Using Gamma/Rejection distribution. 0% Markov autocorrelation overhead.");
    } else {
        ImGui::TextColored(ImVec4(0.9f, 0.5f, 0.1f, 1.0f), "Strategy: Metropolis-Hastings MCMC");
        ImGui::TextWrapped("%s", "Running random walk proposals with target probability density.");
    }

    ImGui::Separator();
    if (ImGui::Button("Generate Verification Package")) {
        m_GenerateVerificationPackage = true;
    }

    ImGui::End();
}

void OrbitalViewerModule::ReallocateFBO()
{
    uint32_t width = m_Engine.GetWindow().GetWidth();
    uint32_t height = m_Engine.GetWindow().GetHeight();

    if (!m_Framebuffer || m_Framebuffer->GetWidth() != width || m_Framebuffer->GetHeight() != height) {
        m_Framebuffer = std::make_unique<GLFramebuffer>(width, height);
        m_Framebuffer->AddColorAttachment(TextureFormat::R32F);
        
        bool ok = m_Framebuffer->Build();
        if (!ok) {
            ORB_CORE_ERROR("OrbitalViewerModule: Failed to build framebuffer ({}x{})", width, height);
        } else {
            ORB_CORE_TRACE("OrbitalViewerModule: Resized framebuffer to {}x{}", width, height);
        }
    }
}

} // namespace Orbital
