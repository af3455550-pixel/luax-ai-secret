#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <memory>
#include <bitset>
#include <cassert>
#include <string>
#include "../Core/Math/MathTypes.hpp"
#include "../Core/Log/Logger.hpp"

namespace Apex::ECS {

    using Entity = uint32_t;
    constexpr Entity NULL_ENTITY = 0xFFFFFFFF;
    constexpr size_t MAX_COMPONENTS = 64;

    using ComponentTypeID = uint32_t;

    inline ComponentTypeID GetUniqueComponentTypeID() {
        static ComponentTypeID nextID = 0;
        return nextID++;
    }

    template <typename T>
    inline ComponentTypeID GetComponentTypeID() {
        static ComponentTypeID id = GetUniqueComponentTypeID();
        return id;
    }

    using ComponentMask = std::bitset<MAX_COMPONENTS>;

    class IPool {
    public:
        virtual ~IPool() = default;
        virtual void Remove(Entity e) = 0;
        virtual bool Has(Entity e) const = 0;
    };

    template <typename T>
    class ComponentPool : public IPool {
    public:
        ComponentPool() = default;

        template <typename... Args>
        T& Add(Entity e, Args&&... args) {
            if (e >= m_sparse.size()) {
                m_sparse.resize(e + 1, -1);
            }

            if (m_sparse[e] != -1) {
                // Replace existing
                size_t index = static_cast<size_t>(m_sparse[e]);
                m_dense[index] = T(std::forward<Args>(args)...);
                return m_dense[index];
            }

            size_t newIndex = m_dense.size();
            m_sparse[e] = static_cast<int32_t>(newIndex);
            m_dense.emplace_back(std::forward<Args>(args)...);
            m_entities.push_back(e);
            return m_dense.back();
        }

        void Remove(Entity e) override {
            if (e >= m_sparse.size() || m_sparse[e] == -1) return;

            size_t indexToRemove = static_cast<size_t>(m_sparse[e]);
            size_t lastIndex = m_dense.size() - 1;
            Entity lastEntity = m_entities[lastIndex];

            // Swap and pop
            m_dense[indexToRemove] = std::move(m_dense[lastIndex]);
            m_entities[indexToRemove] = lastEntity;

            m_sparse[lastEntity] = static_cast<int32_t>(indexToRemove);
            m_sparse[e] = -1;

            m_dense.pop_back();
            m_entities.pop_back();
        }

        bool Has(Entity e) const override {
            return e < m_sparse.size() && m_sparse[e] != -1;
        }

        T& Get(Entity e) {
            assert(Has(e) && "Entity does not have component!");
            return m_dense[static_cast<size_t>(m_sparse[e])];
        }

        const T& Get(Entity e) const {
            assert(Has(e) && "Entity does not have component!");
            return m_dense[static_cast<size_t>(m_sparse[e])];
        }

        std::vector<T>& GetDense() { return m_dense; }
        const std::vector<T>& GetDense() const { return m_dense; }
        const std::vector<Entity>& GetEntities() const { return m_entities; }
        size_t Size() const { return m_dense.size(); }

    private:
        std::vector<int32_t> m_sparse;
        std::vector<T> m_dense;
        std::vector<Entity> m_entities;
    };

    class Registry {
    public:
        Registry() : m_nextEntityID(0) {}

        Entity CreateEntity() {
            Entity e;
            if (!m_freeEntities.empty()) {
                e = m_freeEntities.back();
                m_freeEntities.pop_back();
            } else {
                e = m_nextEntityID++;
            }

            if (e >= m_entityMasks.size()) {
                m_entityMasks.resize(e + 1);
            }
            m_entityMasks[e].reset();
            m_activeEntities.push_back(e);
            return e;
        }

        void DestroyEntity(Entity e) {
            if (e >= m_entityMasks.size()) return;

            for (auto& pool : m_pools) {
                if (pool) {
                    pool->Remove(e);
                }
            }
            m_entityMasks[e].reset();
            m_freeEntities.push_back(e);

            auto it = std::find(m_activeEntities.begin(), m_activeEntities.end(), e);
            if (it != m_activeEntities.end()) {
                m_activeEntities.erase(it);
            }
        }

        template <typename T, typename... Args>
        T& AddComponent(Entity e, Args&&... args) {
            ComponentTypeID id = GetComponentTypeID<T>();
            EnsurePoolCapacity(id);

            if (!m_pools[id]) {
                m_pools[id] = std::make_unique<ComponentPool<T>>();
            }

            m_entityMasks[e].set(id);
            auto* pool = static_cast<ComponentPool<T>*>(m_pools[id].get());
            return pool->Add(e, std::forward<Args>(args)...);
        }

        template <typename T>
        void RemoveComponent(Entity e) {
            ComponentTypeID id = GetComponentTypeID<T>();
            if (id < m_pools.size() && m_pools[id]) {
                m_pools[id]->Remove(e);
                m_entityMasks[e].reset(id);
            }
        }

        template <typename T>
        bool HasComponent(Entity e) const {
            if (e >= m_entityMasks.size()) return false;
            ComponentTypeID id = GetComponentTypeID<T>();
            return m_entityMasks[e].test(id);
        }

        template <typename T>
        T& GetComponent(Entity e) {
            ComponentTypeID id = GetComponentTypeID<T>();
            assert(HasComponent<T>(e) && "Entity lacks requested component!");
            auto* pool = static_cast<ComponentPool<T>*>(m_pools[id].get());
            return pool->Get(e);
        }

        template <typename T>
        const T& GetComponent(Entity e) const {
            ComponentTypeID id = GetComponentTypeID<T>();
            assert(HasComponent<T>(e) && "Entity lacks requested component!");
            auto* pool = static_cast<const ComponentPool<T>*>(m_pools[id].get());
            return pool->Get(e);
        }

        template <typename... Components>
        std::vector<Entity> View() const {
            std::vector<Entity> result;
            ComponentMask queryMask;
            (queryMask.set(GetComponentTypeID<Components>()), ...);

            for (Entity e : m_activeEntities) {
                if ((m_entityMasks[e] & queryMask) == queryMask) {
                    result.push_back(e);
                }
            }
            return result;
        }

        template <typename... Components, typename Func>
        void ForEach(Func&& func) {
            ComponentMask queryMask;
            (queryMask.set(GetComponentTypeID<Components>()), ...);

            for (Entity e : m_activeEntities) {
                if ((m_entityMasks[e] & queryMask) == queryMask) {
                    func(e, GetComponent<Components>(e)...);
                }
            }
        }

        size_t GetActiveEntityCount() const { return m_activeEntities.size(); }
        const std::vector<Entity>& GetAllEntities() const { return m_activeEntities; }

    private:
        void EnsurePoolCapacity(ComponentTypeID id) {
            if (id >= m_pools.size()) {
                m_pools.resize(id + 1);
            }
        }

        Entity m_nextEntityID;
        std::vector<Entity> m_freeEntities;
        std::vector<Entity> m_activeEntities;
        std::vector<ComponentMask> m_entityMasks;
        std::vector<std::unique_ptr<IPool>> m_pools;
    };

    // ==========================================
    // Core Engine Components
    // ==========================================

    struct TagComponent {
        std::string name{"GameObject"};
        std::string tag{"Default"};
        bool isStatic{false};
    };

    struct TransformComponent {
        Math::Transform localTransform;
        Math::Mat4 worldMatrix{Math::Mat4::Identity()};
        bool isDirty{true};

        void SetPosition(const Math::Vec3& pos) {
            localTransform.position = pos;
            isDirty = true;
        }

        void SetRotation(const Math::Quat& rot) {
            localTransform.rotation = rot;
            isDirty = true;
        }

        void SetScale(const Math::Vec3& scale) {
            localTransform.scale = scale;
            isDirty = true;
        }
    };

    enum class LightType {
        Directional,
        Point,
        Spot
    };

    struct LightComponent {
        LightType type{LightType::Point};
        Math::Vec3 color{1.0f, 1.0f, 1.0f};
        float intensity{10.0f};
        float radius{15.0f};
        float innerSpotAngle{20.0f};
        float outerSpotAngle{45.0f};
        bool castShadows{true};
    };

    struct MeshComponent {
        std::string meshAssetID{"Engine/Meshes/Cube.fbx"};
        std::string materialAssetID{"Engine/Materials/PBR_Default.mat"};
        bool castShadows{true};
        bool receiveShadows{true};
        bool isVisible{true};
    };

    enum class ColliderShape {
        Box,
        Sphere,
        Capsule,
        Mesh
    };

    struct ColliderComponent {
        ColliderShape shape{ColliderShape::Box};
        Math::Vec3 size{1.0f, 1.0f, 1.0f}; // box half-extents or radius/height
        Math::Vec3 centerOffset{0.0f, 0.0f, 0.0f};
        bool isTrigger{false};
    };

    struct RigidBodyComponent {
        float mass{1.0f}; // 0 = static
        Math::Vec3 velocity{0.0f, 0.0f, 0.0f};
        Math::Vec3 angularVelocity{0.0f, 0.0f, 0.0f};
        Math::Vec3 totalForces{0.0f, 0.0f, 0.0f};
        bool useGravity{true};
        bool isKinematic{false};
        float linearDamping{0.05f};
        float angularDamping{0.05f};
        float friction{0.6f};
        float restitution{0.3f}; // bounciness

        void ApplyForce(const Math::Vec3& f) {
            totalForces += f;
        }

        void ApplyImpulse(const Math::Vec3& impulse) {
            if (mass > 0.0f && !isKinematic) {
                velocity += impulse / mass;
            }
        }
    };

    struct AudioSourceComponent {
        std::string soundAssetID{"Engine/Audio/Ambience.ogg"};
        float volume{1.0f};
        float pitch{1.0f};
        bool loop{false};
        bool is3D{true};
        float minDistance{1.0f};
        float maxDistance{50.0f};
        bool isPlaying{false};
    };

    struct ScriptComponent {
        std::string scriptPath{"Scripts/PlayerController.lua"};
        std::string className{"PlayerController"};
        bool initialized{false};
        bool isEnabled{true};
    };

    struct NetworkReplicationComponent {
        uint32_t netId{0};
        uint32_t ownerClientId{0};
        bool isServer{false};
        bool isReplicated{true};
        uint32_t dirtyBits{0xFFFFFFFF};
        float replicationFrequency{20.0f}; // 20 Hz
        float lastReplicationTime{0.0f};
    };

} // namespace Apex::ECS
