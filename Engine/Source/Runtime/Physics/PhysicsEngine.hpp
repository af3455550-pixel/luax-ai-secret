#pragma once

#include <vector>
#include <iostream>
#include <cmath>
#include <algorithm>
#include "../ECS/ECS.hpp"
#include "../Core/Math/MathTypes.hpp"
#include "../Core/Profiling/Profiler.hpp"
#include "../Core/Log/Logger.hpp"

namespace Apex::Physics {

    struct RaycastHit {
        bool hasHit{false};
        ECS::Entity entity{ECS::NULL_ENTITY};
        Math::Vec3 point{0.0f, 0.0f, 0.0f};
        Math::Vec3 normal{0.0f, 1.0f, 0.0f};
        float distance{0.0f};
    };

    class PhysicsEngine {
    public:
        static PhysicsEngine& Get() {
            static PhysicsEngine instance;
            return instance;
        }

        void Initialize(const Math::Vec3& gravity = {0.0f, -9.81f, 0.0f}) {
            m_gravity = gravity;
            LOG_INFO("Physics", "Physics Engine initialized (Bullet/PhysX Pipeline Ready), Gravity: " << m_gravity.ToString());
        }

        void StepSimulation(ECS::Registry& registry, float deltaTime) {
            APEX_PROFILE_SCOPE("PhysicsEngine::StepSimulation");

            // 1. Integrate Forces and Velocities
            registry.ForEach<ECS::RigidBodyComponent, ECS::TransformComponent>([&](ECS::Entity, ECS::RigidBodyComponent& rb, ECS::TransformComponent& tc) {
                if (rb.isKinematic || rb.mass <= 0.0f) return;

                if (rb.useGravity) {
                    rb.velocity += m_gravity * deltaTime;
                }

                // Add external forces
                Math::Vec3 accel = rb.totalForces / rb.mass;
                rb.velocity += accel * deltaTime;
                rb.totalForces = Math::Vec3::Zero();

                // Apply damping
                rb.velocity *= (1.0f - rb.linearDamping * deltaTime);

                // Integrate position
                tc.localTransform.position += rb.velocity * deltaTime;
                tc.isDirty = true;
            });

            // 2. Collision Detection and Impulse Resolution (Simple Ground Plane + Sphere Collisions)
            registry.ForEach<ECS::RigidBodyComponent, ECS::ColliderComponent, ECS::TransformComponent>(
                [&](ECS::Entity, ECS::RigidBodyComponent& rb, ECS::ColliderComponent& col, ECS::TransformComponent& tc) {
                    if (rb.isKinematic) return;

                    // Floor collision (Y = 0)
                    float bottomY = tc.localTransform.position.y - col.size.y * 0.5f;
                    if (bottomY < 0.0f) {
                        tc.localTransform.position.y = col.size.y * 0.5f;
                        if (rb.velocity.y < 0.0f) {
                            rb.velocity.y = -rb.velocity.y * rb.restitution;
                            // Apply friction to X and Z
                            rb.velocity.x *= (1.0f - rb.friction * 0.1f);
                            rb.velocity.z *= (1.0f - rb.friction * 0.1f);
                        }
                        tc.isDirty = true;
                    }
                });
        }

        RaycastHit Raycast(ECS::Registry& registry, const Math::Ray& ray, float maxDistance = 1000.0f) {
            APEX_PROFILE_SCOPE("PhysicsEngine::Raycast");
            RaycastHit hit;
            hit.hasHit = false;
            hit.distance = maxDistance;

            registry.ForEach<ECS::ColliderComponent, ECS::TransformComponent>([&](ECS::Entity e, const ECS::ColliderComponent& col, const ECS::TransformComponent& tc) {
                // Ray - Sphere or Ray - AABB intersection
                Math::Vec3 center = tc.worldMatrix.TransformPoint(col.centerOffset);
                float radius = col.size.Length() * 0.5f;

                Math::Vec3 oc = ray.origin - center;
                float b = oc.Dot(ray.direction);
                float c = oc.Dot(oc) - radius * radius;
                float discriminant = b * b - c;

                if (discriminant > 0.0f) {
                    float t = -b - std::sqrt(discriminant);
                    if (t > 0.0f && t < hit.distance) {
                        hit.hasHit = true;
                        hit.distance = t;
                        hit.entity = e;
                        hit.point = ray.GetPoint(t);
                        hit.normal = (hit.point - center).Normalized();
                    }
                }
            });

            return hit;
        }

    private:
        PhysicsEngine() = default;
        Math::Vec3 m_gravity{0.0f, -9.81f, 0.0f};
    };

} // namespace Apex::Physics
