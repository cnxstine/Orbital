#include "visualization/StochasticProbabilityCloud.hpp"
#include "core/Log.hpp"
#include <chrono>

namespace Orbital {

StochasticProbabilityCloud::StochasticProbabilityCloud(ResourceManager& resourceManager, 
                                                       std::shared_ptr<WaveFunction> waveFunction,
                                                       std::shared_ptr<SamplingPipeline> samplingPipeline)
    : m_ResourceManager(resourceManager),
      m_WaveFunction(std::move(waveFunction)),
      m_SamplingPipeline(std::move(samplingPipeline))
{
}

void StochasticProbabilityCloud::Resample(uint32_t numSamples)
{
    if (!m_WaveFunction || !m_SamplingPipeline) {
        ORB_CORE_WARN("StochasticProbabilityCloud: WaveFunction or SamplingPipeline is null. Cannot resample.");
        return;
    }

    auto startTotal = std::chrono::high_resolution_clock::now();
    
    auto sharedThis = std::dynamic_pointer_cast<ProbabilityCloud>(shared_from_this());
    auto result = m_SamplingPipeline->SampleToCloud(*m_WaveFunction, sharedThis, numSamples);
    if (!result) {
        ORB_CORE_ERROR("StochasticProbabilityCloud: Sampling failed.");
        return;
    }
    
    auto endTotal = std::chrono::high_resolution_clock::now();
    double totalMs = std::chrono::duration<double, std::milli>(endTotal - startTotal).count();
    m_SampleGenTimeMs = totalMs - m_GpuUploadTimeMs;
}

void StochasticProbabilityCloud::SetPoints(std::vector<SamplePoint> points)
{
    m_Points = std::move(points);

    auto startUpload = std::chrono::high_resolution_clock::now();
    
    GLBuffer* buffer = m_ResourceManager.Get(m_BufferHandle);
    if (!buffer) {
        // Allocate a new GLBuffer for vertex data using DynamicDraw usage
        auto newBuffer = std::make_unique<GLBuffer>(BufferTarget::Vertex, BufferUsage::DynamicDraw);
        m_BufferHandle = m_ResourceManager.Register(std::move(newBuffer));
        buffer = m_ResourceManager.Get(m_BufferHandle);
    }
    
    if (buffer) {
        buffer->Upload(m_Points.data(), m_Points.size() * sizeof(SamplePoint));
    } else {
        ORB_CORE_ERROR("StochasticProbabilityCloud: Failed to retrieve or register GLBuffer.");
    }
    
    auto endUpload = std::chrono::high_resolution_clock::now();
    m_GpuUploadTimeMs = std::chrono::duration<double, std::milli>(endUpload - startUpload).count();
}

} // namespace Orbital
