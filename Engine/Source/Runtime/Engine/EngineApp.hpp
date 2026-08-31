#pragma once

#include <memory>
#include <chrono>
#include <thread>
#include "../Core/Memory/Memory.hpp"
#include "../Core/Threading/TaskGraph.hpp"
#include "../Core/Events/EventBus.hpp"
#include "../Core/Log/Logger.hpp"
#include "../Core/Profiling/Profiler.hpp"
#include "../Core/Reflection/Reflection.hpp"
#include "../RHI/RHI.hpp"
#include "../RenderCore/PBRRenderer.hpp"
#include "../Physics/PhysicsEngine.hpp"
#include "../Audio/AudioEngine.hpp"
#include "../Scripting/ScriptEngine.hpp"
#include "../Networking/NetworkEngine.hpp"
#include "../AssetPipeline/AssetManager.hpp"
#include "World/World.hpp"

namespace Apex::Engine {

    struct EngineConfig {
        std::string appName{"Apex Engine AAA Runtime"};
        uint32_t windowWidth{1920};
        uint32_t windowHeight{1080};
        bool isHeadlessServer{false};
        size_t workerThreads{4};
        float fixedTimeStep{1.0f / 60.0f}; // 60 Hz physics
    };

    class EngineApp {
    public:
        EngineApp() = default;
        ~EngineApp() { Shutdown(); }

        bool Initialize(const EngineConfig& config = {}) {
            m_config = config;

            LOG_INFO("Engine", "==================================================");
            LOG_INFO("Engine", " Starting " << m_config.appName);
            LOG_INFO("Engine", " Architecture: Modular C++20 ECS / Job System / PBR");
            LOG_INFO("Engine", "==================================================");

            // 1. Threading / Job Task Graph
            Threading::TaskGraph::Get().Initialize(m_config.workerThreads);

            // 2. Reflection System
            RegisterReflection();

            // 3. Asset Pipeline
            Assets::AssetManager::Get().Initialize(1024 * 1024 * 1024); // 1GB Cache

            // 4. RHI & Modern Graphics Device
            RHI::RHIDevice::Get().Initialize(RHI::GraphicsBackend::Vulkan);

            // 5. Render Core & PBR Pipeline
            m_renderer = std::make_unique<RenderCore::PBRRenderer>();
            m_renderer->Initialize(m_config.windowWidth, m_config.windowHeight);

            // 6. Physics Simulation Subsystem
            Physics::PhysicsEngine::Get().Initialize({0.0f, -9.81f, 0.0f});

            // 7. Spatial Audio Subsystem
            Audio::AudioEngine::Get().Initialize();

            // 8. Scripting Subsystem
            ScriptEngineInit();

            // 9. Networking Subsystem
            Networking::NetworkEngine::Get().StartServer(7777);

            // 10. Default World Creation
            m_world = std::make_unique<World>("Sanctuary_Level_01");
            BuildDemoScene();

            m_isRunning = true;
            LOG_INFO("Engine", "Engine Initialization Sequence Completed Successfully!");
            return true;
        }

        void Run(uint32_t simulatedFrames = 120) {
            LOG_INFO("Engine", "Entering Main Loop execution (" << simulatedFrames << " frames)...");

            auto lastTime = std::chrono::high_resolution_clock::now();
            float accumulator = 0.0f;
            float totalEngineTime = 0.0f;

            for (uint32_t frame = 0; frame < simulatedFrames && m_isRunning; ++frame) {
                Profiling::Profiler::Get().BeginFrame();

                auto currentTime = std::chrono::high_resolution_clock::now();
                float frameDelta = std::chrono::duration<float>(currentTime - lastTime).count();
                lastTime = currentTime;

                // Frame clamping to prevent spiral of death
                if (frameDelta > 0.25f) frameDelta = 0.25f;
                if (frameDelta <= 0.0f) frameDelta = 0.0166f;

                accumulator += frameDelta;
                totalEngineTime += frameDelta;

                // 1. Fixed Timestep Physics Simulation
                while (accumulator >= m_config.fixedTimeStep) {
                    Physics::PhysicsEngine::Get().StepSimulation(m_world->GetRegistry(), m_config.fixedTimeStep);
                    accumulator -= m_config.fixedTimeStep;
                }

                // 2. Multithreaded TaskGraph execution for logic, scripting & audio
                auto scriptTask = Threading::TaskGraph::Get().Enqueue([this, frameDelta]() {
                    APEX_PROFILE_SCOPE("ScriptingSystem::Update");
                    Scripting::ScriptEngine::Get().OnUpdate(m_world->GetRegistry(), frameDelta);
                });

                auto audioTask = Threading::TaskGraph::Get().Enqueue([this]() {
                    APEX_PROFILE_SCOPE("AudioSystem::Update");
                    Audio::ListenerData listener;
                    listener.position = m_camera.position;
                    listener.forward = m_camera.forward;
                    Audio::AudioEngine::Get().Update(m_world->GetRegistry(), listener);
                });

                auto netTask = Threading::TaskGraph::Get().Enqueue([this, frameDelta, totalEngineTime]() {
                    APEX_PROFILE_SCOPE("NetworkingSystem::Update");
                    Networking::NetworkEngine::Get().ServerTick(m_world->GetRegistry(), frameDelta, totalEngineTime);
                });

                // Wait for parallel logic tasks
                scriptTask.wait();
                audioTask.wait();
                netTask.wait();

                // 3. Scene Graph hierarchy transform propagation
                {
                    APEX_PROFILE_SCOPE("SceneGraph::Update");
                    m_world->Update(frameDelta);
                }

                // 4. Render Pipeline (Shadows -> GBuffer -> Lighting -> PostProcess)
                m_renderer->RenderFrame(m_world->GetRegistry(), m_camera);

                Profiling::Profiler::Get().EndFrame();

                // Periodic status log every 30 frames
                if (frame % 30 == 0 || frame == simulatedFrames - 1) {
                    PrintFrameStatus(frame);
                }

                // Small yield to simulate standard 60fps pacing
                std::this_thread::sleep_for(std::chrono::milliseconds(4));
            }

            LOG_INFO("Engine", "Simulation loop finished cleanly.");
        }

        void Shutdown() {
            if (!m_isRunning) return;
            LOG_INFO("Engine", "Shutting down Engine subsystems...");

            m_world.reset();
            m_renderer.reset();
            Threading::TaskGraph::Get().Shutdown();

            m_isRunning = false;
            LOG_INFO("Engine", "Engine shutdown complete.");
        }

    private:
        void RegisterReflection() {
            using namespace Reflection;
            Registry::Get().RegisterClass<Math::Transform>("Transform")
                APEX_PROPERTY(Math::Transform, position.x, Float);
        }

        void ScriptEngineInit() {
            Scripting::ScriptEngine::Get().Initialize();
        }

        void BuildDemoScene() {
            LOG_INFO("Engine", "Building AAA Sanctuary Demo Level...");

            // Directional Sun Light
            m_world->SpawnLight("Sun_Directional", ECS::LightType::Directional, {0.0f, 50.0f, 0.0f}, {1.0f, 0.95f, 0.8f}, 50.0f);

            // Point Light in temple center
            m_world->SpawnLight("Temple_Chandelier", ECS::LightType::Point, {0.0f, 5.0f, 5.0f}, {0.2f, 0.6f, 1.0f}, 25.0f);

            // Ground Floor
            m_world->SpawnStaticMesh("Ground_Terrain", "Engine/Meshes/Terrain.gltf", {0.0f, 0.0f, 0.0f}, {50.0f, 1.0f, 50.0f});

            // Dynamic Physics Rigidbodies
            for (int i = 0; i < 5; ++i) {
                float x = (i - 2) * 3.0f;
                float y = 10.0f + i * 2.5f;
                ECS::Entity box = m_world->SpawnPhysicsBox("DynamicCrate_" + std::to_string(i), {x, y, 10.0f}, 15.0f);

                // Add Script Component to first box
                if (i == 0) {
                    auto& sc = m_world->GetRegistry().AddComponent<ECS::ScriptComponent>(box);
                    sc.className = "PlayerController";
                }

                // Add Audio Source
                auto& audio = m_world->GetRegistry().AddComponent<ECS::AudioSourceComponent>(box);
                audio.isPlaying = true;
                audio.volume = 0.75f;
            }

            Scripting::ScriptEngine::Get().OnStart(m_world->GetRegistry());

            // Set up camera
            m_camera.position = {0.0f, 5.0f, -12.0f};
            m_camera.forward = {0.0f, -0.2f, 1.0f};
        }

        void PrintFrameStatus(uint32_t frame) {
            auto& profiler = Profiling::Profiler::Get();
            auto& memStats = Memory::GetGlobalMemoryStats();
            auto& net = Networking::NetworkEngine::Get();

            std::cout << "\n------------------------------------------------------------\n";
            std::cout << std::dec << std::setfill(' ');
            std::cout << ">>> FRAME " << std::setw(3) << frame
                      << " | FPS: " << std::fixed << std::setprecision(1) << profiler.GetFPS()
                      << " | FrameTime: " << std::setprecision(2) << profiler.GetLastFrameTimeMs() << " ms"
                      << " | Entities: " << m_world->GetRegistry().GetActiveEntityCount() << "\n";
            std::cout << ">>> DrawCalls: " << m_renderer->GetLastDrawCalls()
                      << " | Triangles: " << m_renderer->GetLastTriangles()
                      << " | Net Bytes Sent: " << net.GetBytesSent() << " bytes\n";
            std::cout << ">>> Memory Usage: " << (memStats.currentUsage.load() / (1024 * 1024)) << " MB"
                      << " | Active Threads: " << Threading::TaskGraph::Get().GetWorkerCount() << "\n";

            auto samples = profiler.GetSamples();
            std::cout << ">>> Profile Timers:\n";
            for (const auto& s : samples) {
                std::cout << "    - " << std::setw(28) << std::left << s.name << ": "
                          << std::fixed << std::setprecision(3) << s.durationMs << " ms\n";
            }
            std::cout << "------------------------------------------------------------\n";
        }

        EngineConfig m_config;
        bool m_isRunning{false};
        std::unique_ptr<World> m_world;
        std::unique_ptr<RenderCore::PBRRenderer> m_renderer;
        RenderCore::CameraData m_camera;
    };

} // namespace Apex::Engine
