#pragma once
#include "simulation/FireTypes.hpp"
#include "core/Math.hpp"
#include <string>

namespace Fireline {

struct ToolSpec {
    ToolType type;
    std::string name;
    float weightKg;
    float effectiveness; // 0-1
    float waterFlowLps;  // liters per second
    float rangeM;
    float durability;
};

inline ToolSpec getToolSpec(ToolType t){
    switch(t){
        case ToolType::HOSE_MAIN: return {t,"Mangueira Principal",12,0.9f,8.0f,25,1.0f};
        case ToolType::HOSE_FOREST: return {t,"Mangueira Florestal",6,0.7f,4.5f,18,1.0f};
        case ToolType::EXTINGUISHER_WATER: return {t,"Extintor Agua",9,0.4f,1.2f,6,0.3f};
        case ToolType::EXTINGUISHER_FOAM: return {t,"Extintor Espuma",10,0.6f,0.8f,5,0.3f};
        case ToolType::PUMP_PORTABLE: return {t,"Bomba Portatil",25,0.85f,6.0f,20,1.0f};
        case ToolType::TANK: return {t,"Tanque Dorsal",18,0.5f,0.6f,8,0.8f};
        case ToolType::AXE: return {t,"Machado",2.5f,0.3f,0,0,1.0f};
        case ToolType::PULASKI: return {t,"Pulaski",3.0f,0.8f,0,0,1.0f};
        case ToolType::MCLEOD: return {t,"McLeod",3.2f,0.85f,0,0,1.0f};
        case ToolType::SHOVEL: return {t,"Pa",2.0f,0.6f,0,0,1.0f};
        case ToolType::CHAINSAW: return {t,"Motosserra",7.5f,0.9f,0,0,0.9f};
        case ToolType::DRIP_TORCH: return {t,"Pinga-Fogo",3.5f,0.2f,0,0,0.7f};
    }
    return {t,"Desconhecido",1,0.5f,0,0,1};
}

class Tool {
public:
    Tool(ToolType type) : spec(getToolSpec(type)) {}
    virtual ~Tool() = default;

    virtual void update(float dt) {
        if(isActive) activeTime += dt;
    }

    virtual float use(float dt, Vec3 target) {
        if(!isActive) return 0;
        // Returns water or effectiveness
        return spec.waterFlowLps * dt * spec.effectiveness;
    }

    ToolSpec spec;
    bool isActive = false;
    float activeTime = 0;
    float condition = 1.0f;
};

class Hose : public Tool {
public:
    Hose(bool forest=false) : Tool(forest?ToolType::HOSE_FOREST:ToolType::HOSE_MAIN) {
        pressure = 8.0f; // bar
        nozzleType = 0; // 0 jet, 1 fog, 2 wide
    }

    float use(float dt, Vec3 target) override {
        if(!isActive) return 0;
        float flow = spec.waterFlowLps;
        if(nozzleType==1) flow *= 0.7f; // fog uses less water but covers more
        if(nozzleType==2) flow *= 0.5f;
        waterUsed += flow*dt;
        // Pressure drop over distance
        float dist = target.length();
        float pressureFactor = clamp(1.0f - dist*0.01f, 0.3f, 1.0f);
        return flow * dt * pressureFactor;
    }

    void setNozzle(int type){ nozzleType = clamp(type,0,2); }

    float pressure;
    int nozzleType;
    float waterUsed = 0;
    Vec3 sprayOrigin;
    Vec3 sprayDirection;
};

class HandTool : public Tool {
public:
    HandTool(ToolType t) : Tool(t) {
        swingTimer = 0;
    }

    // For creating firebreaks
    float createFirebreakProgress(float dt) {
        if(!isActive) return 0;
        swingTimer += dt;
        if(swingTimer > 0.8f) {
            swingTimer = 0;
            swings++;
            return spec.effectiveness;
        }
        return 0;
    }

    int swings = 0;
    float swingTimer;
};

// PPE
struct PPE {
    bool helmet = true;
    bool mask = true;
    bool goggles = true;
    bool gloves = true;
    bool boots = true;
    bool fireSuit = true;
    float maskFilter = 1.0f; // 0-1
    float suitProtection = 0.85f;

    float getHeatProtection() const { return fireSuit ? 0.7f : 0.2f; }
    float getSmokeProtection() const { return mask ? maskFilter*0.9f : 0.1f; }
};

} // namespace Fireline
