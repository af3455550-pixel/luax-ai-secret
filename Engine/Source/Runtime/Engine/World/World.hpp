#pragma once

#include <string>
#include <memory>
#include <vector>
#include "../../ECS/ECS.hpp"
#include "../Scene/SceneGraph.hpp"
#include "../../Core/Math/MathTypes.hpp"
#include "../../Core/Log/Logger.hpp"

namespace Apex::Engine {

    class World {
    public:
        World(std::string name = "MainWorld")
            : m_name(std::move(name)) {
            m_sceneGraph = std::make_unique<SceneGraph>();
        }

        ECS::Registry& GetRegistry() { return m_registry; }
        const ECS::Registry& GetRegistry() const { return m_registry; }
        SceneGraph& GetSceneGraph() { return *m_sceneGraph; }

        ECS::Entity SpawnActor(const std::string& name, const Math::Vec3& position = {0,0,0}, SceneNode* parent = nullptr) {
            ECS::Entity e = m_registry.CreateEntity();

            auto& tag = m_registry.AddComponent<ECS::TagComponent>(e);
            tag.name = name;

            auto& tc = m_registry.AddComponent<ECS::TransformComponent>(e);
            tc.localTransform.position = position;

            m_sceneGraph->CreateNode(e, name, parent);
            return e;
        }

        ECS::Entity SpawnStaticMesh(const std::string& name, const std::string& meshAsset, const Math::Vec3& position, const Math::Vec3& scale = {1,1,1}) {
            ECS::Entity e = SpawnActor(name, position);
            auto& tc = m_registry.GetComponent<ECS::TransformComponent>(e);
            tc.localTransform.scale = scale;

            auto& mc = m_registry.AddComponent<ECS::MeshComponent>(e);
            mc.meshAssetID = meshAsset;
            mc.castShadows = true;
            return e;
        }

        ECS::Entity SpawnPhysicsBox(const std::string& name, const Math::Vec3& position, float mass = 10.0f) {
            ECS::Entity e = SpawnStaticMesh(name, "Engine/Meshes/Box.obj", position);

            auto& col = m_registry.AddComponent<ECS::ColliderComponent>(e);
            col.shape = ECS::ColliderShape::Box;
            col.size = Math::Vec3(1.0f, 1.0f, 1.0f);

            auto& rb = m_registry.AddComponent<ECS::RigidBodyComponent>(e);
            rb.mass = mass;
            rb.useGravity = true;

            auto& net = m_registry.AddComponent<ECS::NetworkReplicationComponent>(e);
            net.netId = static_cast<uint32_t>(e);

            return e;
        }

        ECS::Entity SpawnLight(const std::string& name, ECS::LightType type, const Math::Vec3& position, const Math::Vec3& color, float intensity) {
            ECS::Entity e = SpawnActor(name, position);
            auto& lc = m_registry.AddComponent<ECS::LightComponent>(e);
            lc.type = type;
            lc.color = color;
            lc.intensity = intensity;
            return e;
        }

        void Update(float deltaTime) {
            (void)deltaTime;
            m_sceneGraph->Update(m_registry);
        }

        const std::string& GetName() const { return m_name; }

    private:
        std::string m_name;
        ECS::Registry m_registry;
        std::unique_ptr<SceneGraph> m_sceneGraph;
    };

} // namespace Apex::Engine
