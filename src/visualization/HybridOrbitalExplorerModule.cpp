#include "visualization/HybridOrbitalExplorerModule.hpp"
#include "core/Engine.hpp"
#include "resources/ResourceManager.hpp"
#include "resources/loaders/ShaderLoader.hpp"
#include "renderer/backend/GLShader.hpp"
#include "renderer/backend/GLBuffer.hpp"
#include "renderer/backend/GLVertexArray.hpp"
#include "renderer/backend/GLTexture.hpp"
#include "renderer/backend/GLFramebuffer.hpp"
#include "camera/CameraManager.hpp"
#include "camera/controllers/ArcballController.hpp"
#include "visualization/ModuleLayer.hpp"
#include "visualization/OrbitalViewerModule.hpp"
#include "visualization/MolecularOrbitalExplorerModule.hpp"
#include "utils/BMPWriter.hpp"
#include "core/Log.hpp"

#include <glad/gl.h>
#include <imgui.h>
#include <filesystem>
#include <fstream>
#include <vector>
#include <cmath>
#include <complex>

namespace Orbital {

HybridOrbitalExplorerModule::HybridOrbitalExplorerModule(Engine& engine)
    : m_Engine(engine)
{
}

void HybridOrbitalExplorerModule::OnEnter()
{
    ORB_CORE_INFO("HybridOrbitalExplorerModule::OnEnter()");

    // Load shaders
    m_PointDensityShader = m_Engine.GetResourceManager().Load<GLShader>("shaders/geometry/point_density");
    m_ResolveShader      = m_Engine.GetResourceManager().Load<GLShader>("shaders/geometry/density_resolve");
    m_LineShader         = m_Engine.GetResourceManager().Load<GLShader>("shaders/geometry/line");

    if (!m_PointDensityShader.IsValid() || !m_ResolveShader.IsValid() || !m_LineShader.IsValid()) {
        ORB_CORE_ERROR("HybridOrbitalExplorerModule: Failed to load shaders.");
    }

    // Allocate framebuffers and VAOs
    ReallocateFBO();
    m_DummyVAO  = std::make_unique<GLVertexArray>();
    m_PointsVAO = std::make_unique<GLVertexArray>();

    // Setup guide line structures
    m_GuideVAO  = std::make_unique<GLVertexArray>();
    m_GuideVBO  = std::make_unique<GLBuffer>(BufferTarget::Vertex, BufferUsage::StaticDraw);

    // Initial setup of hybrid state and guides
    UpdateOrbitalState();
    UpdateGuideBuffers();

    // Check if auto-generation is requested
    if (const char* env = std::getenv("ORBITAL_GENERATE_VERIFICATION")) {
        if (std::string(env) == "1") {
            m_GenerateVerificationPackage = true;
            m_AutoExitAfterGeneration = true;
        }
    }
}

void HybridOrbitalExplorerModule::OnExit()
{
    ORB_CORE_INFO("HybridOrbitalExplorerModule::OnExit()");
    m_DummyVAO.reset();
    m_PointsVAO.reset();
    m_GuideVAO.reset();
    m_GuideVBO.reset();
    m_Framebuffer.reset();
    m_ProbabilityCloud.reset();
    m_SamplingPipeline.reset();
    m_ActiveWaveFunction.reset();
}

void HybridOrbitalExplorerModule::Update(float dt)
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
        m_ActiveWaveFunction = std::make_shared<HybridOrbital>(m_SelectedType, m_ActiveOrbitalIdx);
        m_ProbabilityCloud->SetWaveFunction(m_ActiveWaveFunction);
        m_ProbabilityCloud->Resample(m_ActiveSampleCount);
        m_PendingResample = false;
    }
}

void HybridOrbitalExplorerModule::Render()
{
    if (m_GenerateVerificationPackage) {
        m_GenerateVerificationPackage = false;
        GenerateScreenshots();
    } else {
        RenderFrameInternal();
    }
}

void HybridOrbitalExplorerModule::RenderFrameInternal()
{
    ReallocateFBO();
    uint32_t width = m_Engine.GetWindow().GetWidth();
    uint32_t height = m_Engine.GetWindow().GetHeight();

    // Clear backbuffer to solid black
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (m_ShowCloud && m_ProbabilityCloud && m_ProbabilityCloud->GetPointCount() > 0) {
        // Pass 1: Accumulate point density in floating-point framebuffer
        m_Framebuffer->Bind();
        glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_PROGRAM_POINT_SIZE);

        auto* shader = m_Engine.GetResourceManager().Get(m_PointDensityShader);
        if (shader) {
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

        // Pass 2: Resolve accumulated density texture with tone-mapping
        glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
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

    // Pass 3: Geometry Overlay
    if (m_ShowGuides && m_LineShader.IsValid()) {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);

        auto* lineShader = m_Engine.GetResourceManager().Get(m_LineShader);
        if (lineShader) {
            lineShader->Bind();

            glm::mat4 model = glm::mat4(1.0f);
            lineShader->SetUniform("u_Model", model);

            m_GuideVAO->Bind();

            // Render bond lines (solid green)
            lineShader->SetUniform("u_Color", glm::vec4(0.2f, 0.9f, 0.3f, 1.0f));
            glLineWidth(3.0f);
            glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(m_BondVertexCount));

            // Render outlines (semi-transparent light blue)
            lineShader->SetUniform("u_Color", glm::vec4(0.4f, 0.7f, 1.0f, 0.5f));
            glLineWidth(1.5f);
            glDrawArrays(GL_LINES, static_cast<GLsizei>(m_BondVertexCount), static_cast<GLsizei>(m_OutlineVertexCount));

            m_GuideVAO->Unbind();
            lineShader->Unbind();
        }
    }
}

void HybridOrbitalExplorerModule::OnParameterPanel()
{
    ImGui::Begin("Quantum Chemistry & Performance Controls");

    ImGui::Text("Current Module:");
    ImGui::BulletText("Hydrogen Orbital Explorer");
    ImGui::BulletText("Molecular Orbital Explorer");
    ImGui::BulletText("Hybrid Orbital Explorer (ACTIVE)");
    ImGui::Separator();

    // Module Navigator Combo
    ModuleLayer* moduleLayer = nullptr;
    for (auto& layer : m_Engine.GetLayerStack()) {
        moduleLayer = dynamic_cast<ModuleLayer*>(layer.get());
        if (moduleLayer) break;
    }

    if (moduleLayer) {
        const char* moduleNames[] = { "Hydrogen Orbital Explorer", "Molecular Orbital Explorer", "Hybrid Orbital Explorer" };
        int activeIdx = 2;
        if (ImGui::Combo("Active Module", &activeIdx, moduleNames, 3)) {
            if (activeIdx == 0) {
                moduleLayer->SetActiveModule(std::make_unique<OrbitalViewerModule>(m_Engine));
                ImGui::End();
                return;
            } else if (activeIdx == 1) {
                moduleLayer->SetActiveModule(std::make_unique<MolecularOrbitalExplorerModule>(m_Engine));
                ImGui::End();
                return;
            }
        }
    }
    ImGui::Separator();

    // Hybrid Selector Controls
    ImGui::Text("Hybridization Selector");
    bool stateChanged = false;

    const char* hybridTypeNames[] = { "sp Hybridization", "sp2 Hybridization", "sp3 Hybridization" };
    int currentTypeIdx = static_cast<int>(m_SelectedType);
    if (ImGui::Combo("Hybridization Type", &currentTypeIdx, hybridTypeNames, 3)) {
        m_SelectedType = static_cast<HybridOrbitalType>(currentTypeIdx);
        m_ActiveOrbitalIdx = 0;
        stateChanged = true;
    }

    int maxOrbitals = (m_SelectedType == HybridOrbitalType::sp) ? 2 : (m_SelectedType == HybridOrbitalType::sp2) ? 3 : 4;
    std::vector<std::string> orbitalLabels;
    std::vector<const char*> orbitalLabelsCStr;
    for (int i = 0; i < maxOrbitals; ++i) {
        orbitalLabels.push_back("Hybrid Orbital " + std::to_string(i + 1));
        orbitalLabelsCStr.push_back(orbitalLabels.back().c_str());
    }

    if (ImGui::Combo("Active Orbital", &m_ActiveOrbitalIdx, orbitalLabelsCStr.data(), maxOrbitals)) {
        stateChanged = true;
    }

    const char* sampleCountLabels[] = { "100k", "250k", "500k", "1M" };
    uint32_t sampleCountValues[] = { 100000, 250000, 500000, 1000000 };
    if (ImGui::Combo("Sample Count", &m_SelectedSampleCountIdx, sampleCountLabels, 4)) {
        m_ActiveSampleCount = sampleCountValues[m_SelectedSampleCountIdx];
        stateChanged = true;
    }

    if (stateChanged) {
        m_PendingResample = true;
        UpdateGuideBuffers();
    }

    ImGui::Separator();

    // Toggles for Cloud and Guides
    ImGui::Text("Display Controls");
    ImGui::Checkbox("Show Probability Cloud", &m_ShowCloud);
    ImGui::Checkbox("Show Geometry Guides", &m_ShowGuides);
    ImGui::Separator();

    // Hybrid Orbital Metadata Panel
    ImGui::Text("Hybrid Orbital Metadata");
    const char* typeName = (m_SelectedType == HybridOrbitalType::sp) ? "sp" : (m_SelectedType == HybridOrbitalType::sp2) ? "sp²" : "sp³";
    const char* geomName = (m_SelectedType == HybridOrbitalType::sp) ? "Linear" : (m_SelectedType == HybridOrbitalType::sp2) ? "Trigonal Planar" : "Tetrahedral";
    const char* angleName = (m_SelectedType == HybridOrbitalType::sp) ? "180°" : (m_SelectedType == HybridOrbitalType::sp2) ? "120°" : "109.47°";

    ImGui::BulletText("Hybridization Type: %s", typeName);
    ImGui::BulletText("Orbital Index: %d (of %d)", m_ActiveOrbitalIdx + 1, maxOrbitals);
    ImGui::BulletText("Total Hybrid Orbitals: %d", maxOrbitals);
    ImGui::BulletText("Ideal Bond Angle: %s", angleName);
    ImGui::BulletText("Geometry: %s", geomName);

    // Constituent Atomic Wavefunction formulas
    ImGui::Spacing();
    ImGui::Text("Linear Combination Formula:");
    if (m_ActiveWaveFunction) {
        double c2s = m_ActiveWaveFunction->GetCoeff2s();
        double c2px = m_ActiveWaveFunction->GetCoeff2px();
        double c2py = m_ActiveWaveFunction->GetCoeff2py();
        double c2pz = m_ActiveWaveFunction->GetCoeff2pz();

        std::string formula = "ψ = ";
        bool first = true;
        auto addTerm = [&](double val, const std::string& label) {
            if (std::abs(val) > 1e-4) {
                if (!first) {
                    formula += (val > 0) ? " + " : " - ";
                } else {
                    if (val < 0) formula += "-";
                    first = false;
                }
                char buf[32];
                std::snprintf(buf, sizeof(buf), "%.3f", std::abs(val));
                formula += std::string(buf) + " * |" + label + "⟩";
            }
        };

        addTerm(c2s, "2s");
        addTerm(c2px, "2px");
        addTerm(c2py, "2py");
        addTerm(c2pz, "2pz");

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));
        ImGui::TextWrapped("%s", formula.c_str());
        ImGui::PopStyleColor();
    }

    ImGui::Spacing();
    ImGui::TextWrapped("Description: %s", m_ActiveWaveFunction ? m_ActiveWaveFunction->GetMetadata().Description.c_str() : "");
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

    // Visual Parameters
    ImGui::Text("Visualization Options");
    ImGui::SliderFloat("Particle Size", &m_ParticleSize, 1.0f, 16.0f);
    ImGui::SliderFloat("Intensity Scale", &m_IntensityScale, 0.01f, 2.0f);
    ImGui::SliderFloat("Exposure", &m_Exposure, 0.1f, 5.0f);
    ImGui::SliderFloat("Contrast", &m_Contrast, 0.1f, 3.0f);
    ImGui::SliderFloat("Gamma Correction", &m_Gamma, 1.0f, 3.0f);
    ImGui::Separator();

    // Instrumentation metrics
    ImGui::Text("Performance Instrumentation");
    ImGui::Text("FPS: %.1f", m_FPS);
    ImGui::Text("Frame Time: %.2f ms", m_FrameTimeMs);
    ImGui::Text("Active Samples: %u", m_ProbabilityCloud ? m_ProbabilityCloud->GetPointCount() : 0);
    ImGui::Text("Sample Gen Time: %.2f ms", m_ProbabilityCloud ? m_ProbabilityCloud->GetSampleGenTimeMs() : 0.0);
    ImGui::Text("GPU Upload Time: %.2f ms", m_ProbabilityCloud ? m_ProbabilityCloud->GetGpuUploadTimeMs() : 0.0);

    ImGui::Separator();
    if (ImGui::Button("Generate Verification Package")) {
        m_GenerateVerificationPackage = true;
    }

    ImGui::End();
}

void HybridOrbitalExplorerModule::SetupProbabilityCloud()
{
    m_ActiveWaveFunction = std::make_shared<HybridOrbital>(m_SelectedType, m_ActiveOrbitalIdx);
    m_SamplingPipeline   = std::make_shared<MonteCarloSamplingPipeline>();
    m_ProbabilityCloud   = std::make_shared<StochasticProbabilityCloud>(
        m_Engine.GetResourceManager(),
        m_ActiveWaveFunction,
        m_SamplingPipeline
    );
    m_ProbabilityCloud->Resample(m_ActiveSampleCount);
}

void HybridOrbitalExplorerModule::ReallocateFBO()
{
    uint32_t width = m_Engine.GetWindow().GetWidth();
    uint32_t height = m_Engine.GetWindow().GetHeight();

    if (!m_Framebuffer || m_Framebuffer->GetWidth() != width || m_Framebuffer->GetHeight() != height) {
        m_Framebuffer = std::make_unique<GLFramebuffer>(width, height);
        m_Framebuffer->AddColorAttachment(TextureFormat::R32F);
        
        bool ok = m_Framebuffer->Build();
        if (!ok) {
            ORB_CORE_ERROR("HybridOrbitalExplorerModule: Failed to build framebuffer ({}x{})", width, height);
        } else {
            ORB_CORE_TRACE("HybridOrbitalExplorerModule: Resized framebuffer to {}x{}", width, height);
        }
    }
}

void HybridOrbitalExplorerModule::UpdateOrbitalState()
{
    SetupProbabilityCloud();
}

void HybridOrbitalExplorerModule::UpdateGuideBuffers()
{
    std::vector<glm::vec3> vertices;

    if (m_SelectedType == HybridOrbitalType::sp) {
        // Bonds along Z-axis (Linear)
        vertices.push_back({0.0f, 0.0f, 0.0f});
        vertices.push_back({0.0f, 0.0f, 2.0f});

        vertices.push_back({0.0f, 0.0f, 0.0f});
        vertices.push_back({0.0f, 0.0f, -2.0f});

        m_BondVertexCount = 4;
        m_OutlineVertexCount = 0;
    } else if (m_SelectedType == HybridOrbitalType::sp2) {
        // Trigonal planar bonds in XY plane
        glm::vec3 p0(2.0f, 0.0f, 0.0f);
        glm::vec3 p1(-1.0f, std::sqrt(3.0f), 0.0f);
        glm::vec3 p2(-1.0f, -std::sqrt(3.0f), 0.0f);

        vertices.push_back({0.0f, 0.0f, 0.0f});
        vertices.push_back(p0);

        vertices.push_back({0.0f, 0.0f, 0.0f});
        vertices.push_back(p1);

        vertices.push_back({0.0f, 0.0f, 0.0f});
        vertices.push_back(p2);

        // Outlines
        vertices.push_back(p0);
        vertices.push_back(p1);

        vertices.push_back(p1);
        vertices.push_back(p2);

        vertices.push_back(p2);
        vertices.push_back(p0);

        m_BondVertexCount = 6;
        m_OutlineVertexCount = 6;
    } else if (m_SelectedType == HybridOrbitalType::sp3) {
        // Tetrahedral bonds
        float A = 2.0f / std::sqrt(3.0f);
        glm::vec3 p0(A, A, A);
        glm::vec3 p1(A, -A, -A);
        glm::vec3 p2(-A, A, -A);
        glm::vec3 p3(-A, -A, A);

        vertices.push_back({0.0f, 0.0f, 0.0f});
        vertices.push_back(p0);

        vertices.push_back({0.0f, 0.0f, 0.0f});
        vertices.push_back(p1);

        vertices.push_back({0.0f, 0.0f, 0.0f});
        vertices.push_back(p2);

        vertices.push_back({0.0f, 0.0f, 0.0f});
        vertices.push_back(p3);

        // Outlines
        vertices.push_back(p0); vertices.push_back(p1);
        vertices.push_back(p0); vertices.push_back(p2);
        vertices.push_back(p0); vertices.push_back(p3);
        vertices.push_back(p1); vertices.push_back(p2);
        vertices.push_back(p1); vertices.push_back(p3);
        vertices.push_back(p2); vertices.push_back(p3);

        m_BondVertexCount = 8;
        m_OutlineVertexCount = 12;
    }

    if (!vertices.empty()) {
        m_GuideVBO->Upload(vertices.data(), vertices.size() * sizeof(glm::vec3));

        VertexLayout layout;
        layout.stride = sizeof(glm::vec3);
        layout.attributes = {
            {0, 3, VertexAttributeType::Float, false, 0}
        };
        m_GuideVAO->SetVertexBuffer(*m_GuideVBO, layout);
    }
}

void HybridOrbitalExplorerModule::GenerateScreenshots()
{
    ORB_CORE_INFO("Generating hybrid orbital screenshots...");

    // Save original parameters
    HybridOrbitalType originalType = m_SelectedType;
    int originalOrbIdx = m_ActiveOrbitalIdx;
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

    std::vector<HybridOrbitalType> typesToCapture = { HybridOrbitalType::sp, HybridOrbitalType::sp2, HybridOrbitalType::sp3 };
    const char* typeNames[] = { "sp", "sp2", "sp3" };

    for (size_t typeIdx = 0; typeIdx < typesToCapture.size(); ++typeIdx) {
        m_SelectedType = typesToCapture[typeIdx];
        int maxOrbs = (m_SelectedType == HybridOrbitalType::sp) ? 2 : (m_SelectedType == HybridOrbitalType::sp2) ? 3 : 4;

        for (int orbIdx = 0; orbIdx < maxOrbs; ++orbIdx) {
            m_ActiveOrbitalIdx = orbIdx;
            UpdateGuideBuffers();

            m_ActiveWaveFunction = std::make_shared<HybridOrbital>(m_SelectedType, m_ActiveOrbitalIdx);
            m_ProbabilityCloud->SetWaveFunction(m_ActiveWaveFunction);
            m_ProbabilityCloud->Resample(m_ActiveSampleCount);

            for (const auto& preset : cameraPresets) {
                if (controller) {
                    controller->SetViewPoint({0.0f, 0.0f, 0.0f}, 8.0f, preset.theta, preset.phi);
                }

                RenderFrameInternal();

                // Read screen pixels
                uint32_t width = m_Engine.GetWindow().GetWidth();
                uint32_t height = m_Engine.GetWindow().GetHeight();
                std::vector<uint8_t> pixels(width * height * 3);
                glPixelStorei(GL_PACK_ALIGNMENT, 1);
                glFinish();
                glReadPixels(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height), GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

                std::string filename = "screenshots/hybrid_" + std::string(typeNames[typeIdx]) + "_orb" + std::to_string(orbIdx) + "_" + std::string(preset.name) + ".bmp";
                if (BMPWriter::Save(filename, static_cast<int>(width), static_cast<int>(height), pixels.data())) {
                    ORB_CORE_INFO("Saved hybrid screenshot: {}", filename);
                } else {
                    ORB_CORE_ERROR("Failed to save hybrid screenshot: {}", filename);
                }
            }
        }
    }

    // Restore original parameters
    m_SelectedType = originalType;
    m_ActiveOrbitalIdx = originalOrbIdx;
    m_ActiveSampleCount = originalSampleCount;
    UpdateGuideBuffers();

    m_ActiveWaveFunction = std::make_shared<HybridOrbital>(m_SelectedType, m_ActiveOrbitalIdx);
    m_ProbabilityCloud->SetWaveFunction(m_ActiveWaveFunction);
    m_ProbabilityCloud->Resample(m_ActiveSampleCount);

    if (controller) {
        controller->SetViewPoint(originalPivot, originalRadius, originalTheta, originalPhi);
    }

    if (m_AutoExitAfterGeneration) {
        ORB_CORE_INFO("Auto-exit requested after generating hybrid verification package. Requesting shutdown...");
        m_Engine.RequestShutdown();
    }
}

} // namespace Orbital
