#pragma once
#include "FireTypes.hpp"
#include "core/Math.hpp"

namespace Fireline {

struct FireCell {
    // Position
    int x=0,y=0;
    Vec3 worldPos;

    // Vegetation
    VegetationType vegType = VegetationType::SHRUB_LOW;
    VegetationProfile vegProfile = getVegetationProfile(VegetationType::SHRUB_LOW);

    // Fire state
    FireState state = FireState::NORMAL;
    float heat = 0.0f;              // 0-1000 kJ
    float temperature = 20.0f;      // Celsius
    float moisture = 30.0f;         // %
    float fuelRemaining = 1.0f;     // 0-1
    float burnProgress = 0.0f;      // 0-1
    float smokeDensity = 0.0f;
    float waterContent = 0.0f;      // liters applied
    float timeInState = 0.0f;

    // For reignition
    float hotspotTemp = 0.0f;       // remains hot after BURNED
    bool isHotspot = false;
    bool hasFirebreak = false;
    bool isProtected = false;       // by fire retardant

    // Derived
    float flammability = 0.5f;
    float intensity = 0.0f;         // kW/m

    FireCell() = default;
    FireCell(int x_,int y_, VegetationType vt, Vec3 wp) : x(x_), y(y_), worldPos(wp), vegType(vt) {
        vegProfile = getVegetationProfile(vt);
        fuelRemaining = 1.0f;
        moisture = 30.0f;
        updateFlammability();
    }

    void updateFlammability() {
        // Base on moisture, vegetation, temperature
        float moistureFactor = 1.0f - (moisture / 100.0f);
        float tempFactor = clamp((temperature - 10.0f)/40.0f, 0.1f, 2.0f);
        float fuelFactor = fuelRemaining;
        float vegFactor = vegProfile.combustionSpeed / 2.0f;
        flammability = clamp(moistureFactor * tempFactor * fuelFactor * vegFactor, 0.0f, 1.0f);
        if(hasFirebreak) flammability *= 0.05f;
        if(isProtected) flammability *= 0.2f;
        if(waterContent > 0) {
            flammability *= clamp(1.0f - waterContent*0.1f, 0.05f, 1.0f);
        }
    }

    bool canIgnite() const {
        if(state != FireState::NORMAL) return false;
        if(hasFirebreak) return false;
        if(fuelRemaining <= 0.05f) return false;
        return flammability > 0.15f;
    }

    void applyHeat(float amount) {
        heat += amount;
        temperature += amount * 0.05f;
        updateFlammability();
        if(state == FireState::NORMAL && heat > 150.0f * (1.5f - flammability)) {
            state = FireState::HEATING;
            timeInState = 0;
        }
    }

    void applyWater(float liters) {
        waterContent += liters;
        temperature -= liters * 8.0f; // cooling
        heat = std::max(0.0f, heat - liters * 30.0f);
        smokeDensity += liters * 0.05f; // steam

        if(state == FireState::COMBUSTION) {
            intensity -= liters * 15.0f;
            if(intensity < 20.0f) {
                state = FireState::MOPUP;
                isHotspot = true;
                hotspotTemp = temperature;
            }
        } else if(state == FireState::MOPUP) {
            hotspotTemp -= liters * 10.0f;
            if(hotspotTemp < 60.0f) {
                isHotspot = false;
                state = FireState::BURNED;
            }
        }
        // Evaporation
        if(temperature > 100) {
            waterContent = std::max(0.0f, waterContent - 0.1f);
        }
        updateFlammability();
    }

    void update(float dt, float ambientTemp, float ambientHumidity) {
        timeInState += dt;

        // Environmental influence
        temperature = lerp(temperature, ambientTemp, dt*0.01f);
        moisture = lerp(moisture, ambientHumidity, dt*0.005f);

        // Water evaporation
        if(waterContent > 0) {
            waterContent = std::max(0.0f, waterContent - dt*0.05f);
        }

        switch(state) {
            case FireState::NORMAL:
                heat = std::max(0.0f, heat - dt*5.0f);
                smokeDensity = std::max(0.0f, smokeDensity - dt*0.1f);
                break;
            case FireState::HEATING:
                if(heat > 300.0f || timeInState > 5.0f) {
                    state = FireState::SMOKE;
                    timeInState = 0;
                }
                smokeDensity += dt*0.2f;
                break;
            case FireState::SMOKE:
                smokeDensity += dt*0.5f;
                if(timeInState > 2.0f + (1.0f-flammability)*3.0f) {
                    state = FireState::COMBUSTION;
                    timeInState = 0;
                    intensity = vegProfile.heatContent * 0.01f * fuelRemaining;
                }
                break;
            case FireState::COMBUSTION:
                {
                    float burnRate = vegProfile.combustionSpeed * 0.02f * (1.0f + intensity*0.001f);
                    burnProgress += dt * burnRate;
                    fuelRemaining = std::max(0.0f, 1.0f - burnProgress);
                    intensity = vegProfile.heatContent * 0.015f * fuelRemaining * (1.0f + heat*0.001f);
                    smokeDensity = 0.8f + randFloat()*0.4f;
                    temperature = 600.0f + intensity*0.5f;

                    if(fuelRemaining <= 0.05f || burnProgress >= 1.0f) {
                        state = FireState::MOPUP;
                        isHotspot = true;
                        hotspotTemp = 400.0f + randRange(0,200);
                        timeInState = 0;
                    }
                }
                break;
            case FireState::MOPUP:
                // Hotspot cooling slowly, can reignite
                hotspotTemp = std::max(ambientTemp, hotspotTemp - dt*0.5f);
                smokeDensity = std::max(0.0f, smokeDensity - dt*0.05f);
                intensity = hotspotTemp * 0.1f;
                temperature = hotspotTemp;

                if(hotspotTemp < 80.0f && waterContent > 1.0f) {
                    isHotspot = false;
                    state = FireState::BURNED;
                    timeInState = 0;
                } else if(hotspotTemp > 200.0f && moisture < 20.0f && fuelRemaining > 0.1f) {
                    // Reignition chance
                    if(randFloat() < 0.001f * dt) {
                        state = FireState::COMBUSTION;
                        burnProgress = 0.7f; // partially burned but reignited
                        fuelRemaining = 0.3f;
                    }
                }
                if(timeInState > 120.0f && hotspotTemp < 100.0f) {
                    isHotspot = false;
                    state = FireState::BURNED;
                }
                break;
            case FireState::BURNED:
                smokeDensity = std::max(0.0f, smokeDensity - dt*0.1f);
                temperature = lerp(temperature, ambientTemp, dt*0.02f);
                intensity = 0;
                break;
        }
        updateFlammability();
    }
};

} // namespace Fireline
