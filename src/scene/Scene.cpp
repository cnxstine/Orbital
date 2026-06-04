#include "scene/Scene.hpp"
#include "core/Log.hpp"

namespace Orbital {

Scene::Scene(std::string_view name)
    : m_Name(name)
{
    ORB_CORE_TRACE("Scene '{}' created", m_Name);
}

Scene::~Scene()
{
    Clear();
}

Entity Scene::CreateEntity(std::string_view name)
{
    const EntityID id = m_NextId++;
    m_Components[id]; // Insert empty component map
    m_DebugNames[id] = std::string(name);
    m_EntityNames.push_back(std::string(name));

    ORB_CORE_TRACE("Scene '{}': created entity '{}' (id={})", m_Name, name, id);
    return Entity{id};
}

void Scene::DestroyEntity(Entity e)
{
    if (!e.IsValid()) return;

    auto nameIt = m_DebugNames.find(e.id);
    const std::string dname = (nameIt != m_DebugNames.end()) ? nameIt->second : "?";

    m_Components.erase(e.id);
    m_DebugNames.erase(e.id);

    // Remove from ordered name list
    auto it = std::find(m_EntityNames.begin(), m_EntityNames.end(), dname);
    if (it != m_EntityNames.end()) m_EntityNames.erase(it);

    ORB_CORE_TRACE("Scene '{}': destroyed entity '{}' (id={})", m_Name, dname, e.id);
}

void Scene::OnUpdate(float /*dt*/)
{
    // Systems are invoked externally (by SceneLayer or modules).
    // This hook exists for future built-in scene logic (hierarchy updates, etc.)
}

void Scene::Clear()
{
    m_Components.clear();
    m_DebugNames.clear();
    m_EntityNames.clear();
    m_NextId = 1;
    ORB_CORE_TRACE("Scene '{}' cleared", m_Name);
}

} // namespace Orbital
