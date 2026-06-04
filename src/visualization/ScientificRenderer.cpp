#include "visualization/ScientificRenderer.hpp"
#include "core/Log.hpp"

namespace Orbital {

ScientificRenderer::ScientificRenderer(Renderer& baseRenderer)
    : m_BaseRenderer(baseRenderer)
{
    ORB_CORE_TRACE("ScientificRenderer initialized");
}

void ScientificRenderer::SubmitVolume(const VolumeComponent& volume, const glm::mat4& transform)
{
    // Validate resources
    if (!volume.VolumeTexture.IsValid()) {
        ORB_CORE_WARN("ScientificRenderer::SubmitVolume: Invalid VolumeTexture handle.");
        return;
    }

    ORB_CORE_TRACE("ScientificRenderer::SubmitVolume: Submitting volumetric raymarch pass");
    
    // In the future, this will submit a draw call using a screen-aligned bounding box quad
    // or a bounding cube, binding the volume texture to a texture unit (e.g., slot 0) 
    // and the transfer function to another.
    
    DrawCommand cmd;
    cmd.vaoId = 0; // Bounding volume VAO
    cmd.shaderProgramId = 0; // Volumetric raymarching shader
    cmd.vertexCount = 36; // Bounding cube vertices
    cmd.instanceCount = 1;
    cmd.primitiveType = 0x0004; // GL_TRIANGLES
    
    m_BaseRenderer.Submit(cmd);
}

void ScientificRenderer::SubmitStochasticCloud(const StochasticCloudComponent& cloud)
{
    if (!cloud.PointBuffer.IsValid() || cloud.PointCount == 0) {
        ORB_CORE_WARN("ScientificRenderer::SubmitStochasticCloud: Invalid PointBuffer or zero point count.");
        return;
    }

    ORB_CORE_TRACE("ScientificRenderer::SubmitStochasticCloud: Submitting {} points for stochastic render", cloud.PointCount);

    // Submits a point-cloud draw command.
    // In OpenGL: glDrawArrays(GL_POINTS, 0, PointCount)
    // The billboarding and phase-to-hue calculations happen in the shader.
    
    DrawCommand cmd;
    cmd.vaoId = 0; // Point VAO (could be a dummy VAO since data is bound via SSBO/VBO)
    cmd.shaderProgramId = 0; // Stochastic point shader
    cmd.vertexCount = cloud.PointCount;
    cmd.indexCount = 0; // DrawArrays
    cmd.instanceCount = 1;
    cmd.primitiveType = 0x0000; // GL_POINTS (0x0000)
    
    m_BaseRenderer.Submit(cmd);
}

void ScientificRenderer::SubmitGlyphs(const VectorFieldComponent& field)
{
    if (!field.FieldBuffer.IsValid() || field.FieldSize == 0) {
        ORB_CORE_WARN("ScientificRenderer::SubmitGlyphs: Invalid FieldBuffer or empty field.");
        return;
    }

    ORB_CORE_TRACE("ScientificRenderer::SubmitGlyphs: Submitting {} vector field glyphs", field.FieldSize);

    // Instanced draw call: draws a template arrow glyph N times where N = FieldSize.
    // The instance transformation matrix / direction is read from the FieldBuffer.
    
    DrawCommand cmd;
    cmd.vaoId = 0; // Arrow geometry VAO
    cmd.shaderProgramId = 0; // Vector glyph shader
    cmd.vertexCount = 12; // Arrow template vertices
    cmd.indexCount = 0;
    cmd.instanceCount = field.FieldSize;
    cmd.primitiveType = 0x0004; // GL_TRIANGLES
    
    m_BaseRenderer.Submit(cmd);
}

void ScientificRenderer::SubmitLatticeBox(const glm::vec3& minBounds, const glm::vec3& maxBounds, const glm::vec4& color)
{
    ORB_CORE_TRACE("ScientificRenderer::SubmitLatticeBox: Submitting wireframe box");

    // Submits a wireframe box outline.
    // Typically renders a cube template using lines.
    
    DrawCommand cmd;
    cmd.vaoId = 0; // Cube wireframe VAO
    cmd.shaderProgramId = 0; // Basic line shader
    cmd.vertexCount = 24; // 12 edges * 2 vertices
    cmd.indexCount = 0;
    cmd.instanceCount = 1;
    cmd.primitiveType = 0x0001; // GL_LINES (0x0001)
    
    m_BaseRenderer.Submit(cmd);
}

} // namespace Orbital
