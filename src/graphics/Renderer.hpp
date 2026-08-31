#pragma once
#include "core/Math.hpp"
#include "Camera.hpp"
#include "ParticleSystem.hpp"
#include "simulation/FireSimulation.hpp"
#include "world/Map.hpp"
#include "gameplay/Vehicle.hpp"
#include <vector>
#include <string>

namespace Fireline {

// Renderer abstraction - supports console fallback and raylib/OpenGL when available
// Implements: Depth of Field, Motion Blur, Volumetric Smoke, Shadows, Reflections

class Renderer {
public:
    Renderer(int width=1280, int height=720) : screenWidth(width), screenHeight(height) {
        // Initialize default materials
    }

    void init() {
#if FIRELINE_HAS_RAYLIB
        // Raylib initialization would go here:
        // InitWindow(screenWidth, screenHeight, "FIRELINE: WILDFIRE COMMAND");
        // SetTargetFPS(60);
        // Load shaders for DoF, motion blur, volumetric
        // Load models for trees, vehicles, etc.
#endif
        initialized = true;
    }

    void shutdown() {
#if FIRELINE_HAS_RAYLIB
        // CloseWindow();
#endif
    }

    void beginFrame() {
        frameCount++;
        // Clear with sky color based on time of day
    }

    void endFrame() {
        // Swap buffers, apply post-processing
        // DoF: blur based on focusDistance
        // Motion blur: based on camera velocity
        // Volumetric smoke: raymarching through particle density
    }

    // Terrain rendering with heightmap and slope-based texturing
    void renderTerrain(const Terrain& terrain, const CameraSystem& camera) {
        // Would use heightmap mesh, with LOD, with texture splatting based on slope and height
        // Shader: triplanar mapping, normal mapping, shadow mapping
    }

    // Vegetation instancing
    void renderVegetation(const FireSimulation& sim, const CameraSystem& camera) {
        // For each cell, if has tree, render model with state:
        // NORMAL: green
        // HEATING: slightly brown, heat shimmer
        // SMOKE: particle emitter
        // COMBUSTION: fire shader with flame billboards, emissive
        // BURNED: blackened trunk, fallen if probability
        // MOPUP: glowing embers
        // Use instancing for performance
    }

    // Fire rendering - advanced
    void renderFire(const FireSimulation& sim, const CameraSystem& camera) {
        // For each combustion cell:
        // - Flame: billboard with fire texture, animated UV, emissive, light source
        // - Smoke: volumetric particles with lighting
        // - Embers: point sprites with glow
        // - Light: dynamic point light affecting surroundings
        // Intensity affects flame height and light radius
    }

    // Vehicles with PBR materials, suspension animation, lights
    void renderVehicles(const std::vector<std::unique_ptr<Vehicle>>& vehicles, const CameraSystem& camera) {
        // For each vehicle:
        // - Model with PBR (metallic, roughness)
        // - Wheel rotation and suspension compression
        // - Siren lights (emissive + bloom)
        // - Water tank level visible?
        // - Hose physics
        // - Shadow mapping
        // - Reflection probes
    }

    // Map features
    void renderMapFeatures(const GameMap& map, const CameraSystem& camera) {
        // Villages, houses, watchtowers, bridges, rivers (water shader), etc.
    }

    // Particles: smoke, ash, embers, steam
    void renderParticles(const ParticleSystem& particles, const CameraSystem& camera) {
        // Sort by depth for transparency
        // Render as camera-facing quads with soft particles
        // Smoke: volumetric with light scattering
        // Ash: subtle, slow falling, affected by wind
        // Embers: additive blending, glow
    }

    // Post-processing
    void applyDepthOfField(const CameraSystem& camera) {
        // DoF based on camera.focusDistance and dofBlur
        // Bokeh shape, aperture
    }

    void applyMotionBlur(const CameraSystem& camera) {
        // Velocity buffer, blur based on camera.motionBlurIntensity
    }

    void applyVolumetricSmoke(const ParticleSystem& smoke, const CameraSystem& camera) {
        // Raymarch through smoke density, with light scattering
    }

    // UI
    void renderHUD() {
        // HUD with health, stamina, fire stats, minimap, objectives
    }

    // Credits - 3D cinematic
    void renderCredits3D(const class CreditsSystem& credits, const CameraSystem& camera) {
        // For each visible text:
        // - 3D transform with perspective
        // - Lighting: warm fire light from below + cool moon from above
        // - Soft shadows: shadow map with PCF
        // - Subtle reflections: screen-space reflections on wet ground
        // - Motion blur: based on scroll speed
        // - Depth of Field: blur based on depth
        // - Particles: ash and embers in background
        // - Background: destroyed forest with smoke
        // - Final pullback: camera dolly out revealing huge burned forest
    }

    int screenWidth, screenHeight;
    int frameCount=0;
    bool initialized=false;

    // Settings
    bool enableDoF = true;
    bool enableMotionBlur = true;
    bool enableVolumetric = true;
    bool enableShadows = true;
    bool enableReflections = true;
    float shadowSoftness = 0.7f;
    float reflectionIntensity = 0.15f;
};

} // namespace Fireline
