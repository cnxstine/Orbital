#include "scene/systems/RenderSystem.hpp"

// RenderSystem intentionally empty for the rendering foundation milestone.
// When MeshComponent is introduced, this system will:
//
//   scene.Each<TransformComponent, MeshComponent>([&](Entity e, auto& t, auto& m) {
//       if (!m.MeshHandle.IsValid()) return;
//
//       auto* mesh   = renderer.GetResourceManager().Get(m.MeshHandle);
//       auto* shader = renderer.GetResourceManager().Get(m.MaterialHandle);
//       if (!mesh || !shader) return;
//
//       shader->Bind();
//       shader->SetUniform("u_Model",        t.WorldMatrix);
//       shader->SetUniform("u_NormalMatrix", glm::transpose(glm::inverse(t.WorldMatrix)));
//
//       renderer.Submit(DrawCommand{
//           .vaoId           = mesh->GetVAO().GetID(),
//           .shaderProgramId = shader->GetProgramID(),
//           .indexCount      = mesh->GetIndexCount(),
//           .instanceCount   = 1,
//       });
//   });

namespace Orbital {

void RenderSystem::OnUpdate(Scene& /*scene*/, Renderer& /*renderer*/, RenderContext& /*ctx*/)
{
    // Intentionally empty — foundation milestone.
    // The Renderer::BeginFrame() already clears to the configured clear color.
}

} // namespace Orbital
