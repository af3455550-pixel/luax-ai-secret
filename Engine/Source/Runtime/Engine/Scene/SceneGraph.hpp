#pragma once

#include <vector>
#include <memory>
#include <string>
#include "../../Core/Math/MathTypes.hpp"
#include "../../ECS/ECS.hpp"

namespace Apex::Engine {

    class SceneNode {
    public:
        SceneNode(ECS::Entity entity, const std::string& name = "SceneNode")
            : m_entity(entity), m_name(name), m_parent(nullptr), m_isDirty(true) {}

        ECS::Entity GetEntity() const { return m_entity; }
        const std::string& GetName() const { return m_name; }
        void SetName(const std::string& name) { m_name = name; }

        SceneNode* GetParent() const { return m_parent; }
        const std::vector<std::unique_ptr<SceneNode>>& GetChildren() const { return m_children; }

        void AddChild(std::unique_ptr<SceneNode> child) {
            if (child) {
                child->m_parent = this;
                child->MarkDirty();
                m_children.push_back(std::move(child));
            }
        }

        void MarkDirty() {
            m_isDirty = true;
            for (auto& child : m_children) {
                child->MarkDirty();
            }
        }

        void UpdateWorldTransform(ECS::Registry& registry, const Math::Mat4& parentMatrix) {
            if (registry.HasComponent<ECS::TransformComponent>(m_entity)) {
                auto& tc = registry.GetComponent<ECS::TransformComponent>(m_entity);
                if (m_isDirty || tc.isDirty) {
                    Math::Mat4 localMat = tc.localTransform.ToMatrix();
                    tc.worldMatrix = parentMatrix * localMat;
                    tc.isDirty = false;
                    m_isDirty = false;
                }

                for (auto& child : m_children) {
                    child->UpdateWorldTransform(registry, tc.worldMatrix);
                }
            } else {
                for (auto& child : m_children) {
                    child->UpdateWorldTransform(registry, parentMatrix);
                }
            }
        }

    private:
        ECS::Entity m_entity{ECS::NULL_ENTITY};
        std::string m_name;
        SceneNode* m_parent{nullptr};
        std::vector<std::unique_ptr<SceneNode>> m_children;
        bool m_isDirty{true};
    };

    class SceneGraph {
    public:
        SceneGraph() {
            m_root = std::make_unique<SceneNode>(ECS::NULL_ENTITY, "Root");
        }

        SceneNode* GetRoot() { return m_root.get(); }

        SceneNode* CreateNode(ECS::Entity entity, const std::string& name, SceneNode* parent = nullptr) {
            auto node = std::make_unique<SceneNode>(entity, name);
            SceneNode* raw = node.get();
            if (parent) {
                parent->AddChild(std::move(node));
            } else {
                m_root->AddChild(std::move(node));
            }
            return raw;
        }

        void Update(ECS::Registry& registry) {
            if (m_root) {
                m_root->UpdateWorldTransform(registry, Math::Mat4::Identity());
            }
        }

    private:
        std::unique_ptr<SceneNode> m_root;
    };

} // namespace Apex::Engine
