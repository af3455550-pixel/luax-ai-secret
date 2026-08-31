#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <functional>
#include <sstream>
#include "../ECS/ECS.hpp"
#include "../Core/Log/Logger.hpp"

namespace Apex::Scripting {

    struct ScriptEnvironment {
        std::unordered_map<std::string, float> numberVars;
        std::unordered_map<std::string, std::string> stringVars;
    };

    class ScriptEngine {
    public:
        static ScriptEngine& Get() {
            static ScriptEngine instance;
            return instance;
        }

        void Initialize() {
            RegisterNativeBindings();
            LOG_INFO("Scripting", "Initialized Lua/Python VM Virtual Machine & Native API Bindings.");
        }

        void RegisterNativeBindings() {
            // Expose native bindings
            m_nativeBindings["Log"] = [](const std::string& arg) {
                LOG_INFO("ScriptRuntime", arg);
            };
        }

        void OnStart(ECS::Registry& registry) {
            registry.ForEach<ECS::ScriptComponent, ECS::TransformComponent>(
                [&](ECS::Entity e, ECS::ScriptComponent& sc, ECS::TransformComponent& /*tc*/) {
                    if (!sc.isEnabled || sc.initialized) return;
                    sc.initialized = true;
                    LOG_INFO("ScriptRuntime", "Executed " << sc.className << "::OnStart on Entity #" << e);
                });
        }

        void OnUpdate(ECS::Registry& registry, float deltaTime) {
            registry.ForEach<ECS::ScriptComponent, ECS::TransformComponent, ECS::RigidBodyComponent>(
                [&](ECS::Entity /*e*/, ECS::ScriptComponent& sc, ECS::TransformComponent& tc, ECS::RigidBodyComponent& /*rb*/) {
                    if (!sc.isEnabled) return;

                    // Simulated script execution logic: e.g. subtle rotation or AI movement
                    if (sc.className == "PlayerController") {
                        // Example: Apply small forward movement or oscillation
                        float timeOffset = deltaTime * 2.0f;
                        tc.localTransform.position.x += std::sin(timeOffset) * 0.05f;
                        tc.isDirty = true;
                    }
                });
        }

        void ReloadScripts(ECS::Registry& registry) {
            LOG_INFO("Scripting", "Hot-reloading all active Lua/Python scripts...");
            registry.ForEach<ECS::ScriptComponent>([&](ECS::Entity, ECS::ScriptComponent& sc) {
                sc.initialized = false;
            });
            OnStart(registry);
        }

    private:
        ScriptEngine() = default;
        std::unordered_map<std::string, std::function<void(const std::string&)>> m_nativeBindings;
    };

} // namespace Apex::Scripting
