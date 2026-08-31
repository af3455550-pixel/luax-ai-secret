#pragma once

#include <vector>
#include <memory>
#include <cmath>
#include "RenderGraph.hpp"
#include "../RHI/RHI.hpp"
#include "../ECS/ECS.hpp"
#include "../Core/Math/MathTypes.hpp"
#include "../Core/Profiling/Profiler.hpp"

namespace Apex::RenderCore {

    struct CameraData {
        Math::Vec3 position{0.0f, 2.0f, -5.0f};
        Math::Vec3 forward{0.0f, 0.0f, 1.0f};
        Math::Mat4 viewMatrix{Math::Mat4::Identity()};
        Math::Mat4 projMatrix{Math::Mat4::Identity()};
        Math::Mat4 viewProjMatrix{Math::Mat4::Identity()};
        float fovRad{60.0f * Math::DEG2RAD};
        float aspect{16.0f / 9.0f};
        float nearZ{0.1f};
        float farZ{1000.0f};

        void UpdateMatrices() {
            viewMatrix = Math::Mat4::LookAt(position, position + forward, Math::Vec3::Up());
            projMatrix = Math::Mat4::Perspective(fovRad, aspect, nearZ, farZ);
            viewProjMatrix = projMatrix * viewMatrix;
        }
    };

    struct MaterialPBR {
        Math::Vec3 albedo{0.8f, 0.8f, 0.8f};
        float metallic{0.0f};
        float roughness{0.5f};
        float ao{1.0f};
        Math::Vec3 emissive{0.0f, 0.0f, 0.0f};
    };

    // PBR BRDF Math (Cook-Torrance GGX)
    inline float DistributionGGX(const Math::Vec3& N, const Math::Vec3& H, float roughness) {
        float a = roughness * roughness;
        float a2 = a * a;
        float NdotH = std::max(N.Dot(H), 0.0f);
        float NdotH2 = NdotH * NdotH;

        float num = a2;
        float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
        denom = Math::PI * denom * denom;

        return num / std::max(denom, 0.0000001f);
    }

    inline float GeometrySchlickGGX(float NdotV, float roughness) {
        float r = (roughness + 1.0f);
        float k = (r * r) / 8.0f;

        float num = NdotV;
        float denom = NdotV * (1.0f - k) + k;

        return num / std::max(denom, 0.0000001f);
    }

    inline float GeometrySmith(const Math::Vec3& N, const Math::Vec3& V, const Math::Vec3& L, float roughness) {
        float NdotV = std::max(N.Dot(V), 0.0f);
        float NdotL = std::max(N.Dot(L), 0.0f);
        float ggx2 = GeometrySchlickGGX(NdotV, roughness);
        float ggx1 = GeometrySchlickGGX(NdotL, roughness);

        return ggx1 * ggx2;
    }

    inline Math::Vec3 FresnelSchlick(float cosTheta, const Math::Vec3& F0) {
        float factor = std::pow(std::clamp(1.0f - cosTheta, 0.0f, 1.0f), 5.0f);
        return F0 + (Math::Vec3(1.0f) - F0) * factor;
    }

    class PBRRenderer {
    public:
        PBRRenderer() = default;

        void Initialize(uint32_t width = 1920, uint32_t height = 1080) {
            m_width = width;
            m_height = height;

            // Allocate G-Buffer Render Targets
            RHI::TextureDescriptor albedoDesc{width, height, 1, 1, RHI::TextureFormat::RGBA8_UNORM, true, false, "GBuffer_AlbedoRoughness"};
            RHI::TextureDescriptor normalDesc{width, height, 1, 1, RHI::TextureFormat::RGBA16_FLOAT, true, false, "GBuffer_NormalMetallic"};
            RHI::TextureDescriptor depthDesc{width, height, 1, 1, RHI::TextureFormat::D24_S8, true, true, "GBuffer_Depth"};

            m_gbufferAlbedo = RHI::RHIDevice::Get().CreateTexture(albedoDesc);
            m_gbufferNormal = RHI::RHIDevice::Get().CreateTexture(normalDesc);
            m_gbufferDepth = RHI::RHIDevice::Get().CreateTexture(depthDesc);

            LOG_INFO("Renderer", "Initialized Deferred + Forward+ PBR Pipeline (" << width << "x" << height << ")");
        }

        void RenderFrame(ECS::Registry& registry, CameraData& camera) {
            APEX_PROFILE_SCOPE("PBRRenderer::RenderFrame");

            camera.UpdateMatrices();

            // 1. Frustum Culling
            std::vector<ECS::Entity> visibleMeshes;
            {
                APEX_PROFILE_SCOPE("FrustumCulling");
                registry.ForEach<ECS::MeshComponent, ECS::TransformComponent>([&](ECS::Entity e, const ECS::MeshComponent& mesh, const ECS::TransformComponent& tc) {
                    if (!mesh.isVisible) return;
                    // Simple distance/frustum test
                    Math::Vec3 toMesh = tc.worldMatrix.TransformPoint(Math::Vec3::Zero()) - camera.position;
                    float dist = toMesh.Length();
                    if (dist < camera.farZ) {
                        visibleMeshes.push_back(e);
                    }
                });
            }

            RenderGraph graph;

            // Pass 1: Cascaded Shadow Map (CSM)
            graph.AddPass("CascadedShadowPass", [this, &registry](RHI::RHICommandBuffer& cmd) {
                APEX_PROFILE_SCOPE("Pass_ShadowMap");
                // Render shadow casters into depth atlas
                registry.ForEach<ECS::LightComponent>([&](ECS::Entity, const ECS::LightComponent& light) {
                    if (light.castShadows) {
                        cmd.DrawIndexed(12000, 1); // Simulated shadow draw call
                    }
                });
            });

            // Pass 2: Deferred G-Buffer Generation
            graph.AddPass("GBufferPass", [this, &registry, &visibleMeshes](RHI::RHICommandBuffer& cmd) {
                APEX_PROFILE_SCOPE("Pass_GBuffer");
                for (ECS::Entity e : visibleMeshes) {
                    if (registry.HasComponent<ECS::MeshComponent>(e)) {
                        cmd.DrawIndexed(28000, 1); // Draw mesh geometry into GBuffer
                    }
                }
            });

            // Pass 3: Clustered Deferred PBR Lighting
            graph.AddPass("LightingPass", [this, &registry, &camera](RHI::RHICommandBuffer& cmd) {
                APEX_PROFILE_SCOPE("Pass_PBRLighting");
                // Evaluate Cook-Torrance BRDF for directional + point lights
                cmd.Draw(3, 1); // Fullscreen triangle lighting resolve
            });

            // Pass 4: Forward+ Transparent & Particles Pass
            graph.AddPass("ForwardTransparentPass", [](RHI::RHICommandBuffer& cmd) {
                APEX_PROFILE_SCOPE("Pass_ForwardTransparents");
                cmd.DrawIndexed(1500, 1);
            });

            // Pass 5: Post-Processing (ACES Tonemapping, Bloom, FXAA)
            graph.AddPass("PostProcessPass", [](RHI::RHICommandBuffer& cmd) {
                APEX_PROFILE_SCOPE("Pass_PostProcess");
                cmd.Draw(3, 1); // Fullscreen tonemap & post-process composite
            });

            graph.Compile();
            auto cmd = RHI::RHIDevice::Get().GetCommandBuffer();
            graph.Execute(*cmd);

            m_lastDrawCalls = cmd->GetDrawCallCount();
            m_lastTriangles = cmd->GetTriangleCount();
        }

        uint32_t GetLastDrawCalls() const { return m_lastDrawCalls; }
        uint32_t GetLastTriangles() const { return m_lastTriangles; }

    private:
        uint32_t m_width{1920};
        uint32_t m_height{1080};
        std::shared_ptr<RHI::RHITexture> m_gbufferAlbedo;
        std::shared_ptr<RHI::RHITexture> m_gbufferNormal;
        std::shared_ptr<RHI::RHITexture> m_gbufferDepth;
        uint32_t m_lastDrawCalls{0};
        uint32_t m_lastTriangles{0};
    };

} // namespace Apex::RenderCore
