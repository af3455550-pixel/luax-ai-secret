#include <iostream>
#include <vector>
#include <string>
#include "../../Runtime/Engine/EngineApp.hpp"

int main(int argc, char* argv[]) {
    std::cout << "===============================================================\n";
    std::cout << "             APEX ENGINE AAA - RUNTIME & ENGINE ARCHITECTURE   \n";
    std::cout << "             Modular C++20 Core inspired by Unreal Engine      \n";
    std::cout << "===============================================================\n";

    Apex::Engine::EngineConfig config;
    config.appName = "ApexEngine AAA Showcase";
    config.windowWidth = 1920;
    config.windowHeight = 1080;
    config.workerThreads = 4;
    config.fixedTimeStep = 1.0f / 60.0f;

    uint32_t framesToRun = 90;
    if (argc > 1) {
        framesToRun = std::stoul(argv[1]);
    }

    Apex::Engine::EngineApp engine;
    if (!engine.Initialize(config)) {
        std::cerr << "Failed to initialize Apex Engine!\n";
        return 1;
    }

    // Run Engine Simulation Loop
    engine.Run(framesToRun);

    // Shutdown
    engine.Shutdown();

    std::cout << "\n[ApexEngine] Execution successfully completed!\n";
    return 0;
}
