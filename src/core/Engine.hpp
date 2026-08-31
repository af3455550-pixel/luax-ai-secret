#pragma once
#include "Game.hpp"
#include "graphics/Renderer.hpp"
#include <chrono>
#include <thread>

namespace Fireline {

// Fireline Engine - proprietary C++17 engine
// Features: job system, memory allocators, optimized math, multithreaded simulation

class Engine {
public:
    Engine() {
        renderer = std::make_unique<Renderer>(1920,1080);
        game = std::make_unique<Game>();
    }

    void run() {
        renderer->init();
        auto last = std::chrono::high_resolution_clock::now();
        float accumulator = 0;
        const float dt = 1.0f/60.0f;

        while(!shouldClose()) {
            auto now = std::chrono::high_resolution_clock::now();
            float frameTime = std::chrono::duration<float>(now-last).count();
            last = now;
            frameTime = std::min(frameTime, 0.25f);
            accumulator += frameTime;

            // Input handling (platform specific)
            pollInput();

            while(accumulator >= dt) {
                game->update(dt);
                accumulator -= dt;
            }

            // Rendering
            renderer->beginFrame();
            // Render world
            // renderer->renderTerrain(game->fireSim->getTerrain(), *game->camera);
            // renderer->renderVegetation(*game->fireSim, *game->camera);
            // renderer->renderFire(*game->fireSim, *game->camera);
            // renderer->renderVehicles(game->vehicles, *game->camera);
            // renderer->renderMapFeatures(*game->gameMap, *game->camera);
            // renderer->renderParticles(*game->particles, *game->camera);
            // Post-processing
            // renderer->applyDepthOfField(*game->camera);
            // renderer->applyMotionBlur(*game->camera);
            // renderer->applyVolumetricSmoke(*game->particles, *game->camera);
            // UI
            // renderer->renderHUD();
            renderer->endFrame();

            // Cap FPS
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        renderer->shutdown();
    }

    bool shouldClose() {
        // Check window close, ESC, etc.
        return game->state == GameState::EXIT;
    }

    void pollInput() {
        // Platform input polling
    }

    std::unique_ptr<Renderer> renderer;
    std::unique_ptr<Game> game;
};

} // namespace Fireline
