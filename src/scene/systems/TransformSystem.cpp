#include "scene/systems/TransformSystem.hpp"
#include "scene/Scene.hpp"
#include "scene/components/TransformComponent.hpp"

namespace Orbital {

void TransformSystem::OnUpdate(Scene& scene)
{
    scene.Each<TransformComponent>([](Entity /*e*/, TransformComponent& transform) {
        if (transform.Dirty) {
            transform.WorldMatrix = transform.GetLocalMatrix();
            transform.Dirty       = false;
        }
    });
}

} // namespace Orbital
