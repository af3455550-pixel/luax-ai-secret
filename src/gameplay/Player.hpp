#pragma once
#include "core/Math.hpp"
#include "Tools.hpp"
#include "Vehicle.hpp"
#include "simulation/FireSimulation.hpp"
#include <memory>
#include <vector>

namespace Fireline {

class Player {
public:
    Player(Vec3 pos) : position(pos) {
        health = 100;
        stamina = 100;
        hydration = 100;
        // Default tools
        tools.push_back(std::make_unique<Hose>(false));
        tools.push_back(std::make_unique<HandTool>(ToolType::PULASKI));
        tools.push_back(std::make_unique<HandTool>(ToolType::SHOVEL));
        tools.push_back(std::make_unique<Tool>(ToolType::EXTINGUISHER_WATER));
        currentTool = 0;

        ppe.helmet = true;
        ppe.mask = true;
        ppe.fireSuit = true;
        ppe.gloves = true;
        ppe.boots = true;
    }

    void update(float dt, FireSimulation& sim, const WeatherState& weather) {
        time += dt;

        // Movement
        Vec3 moveInput{inputMove.x, 0, inputMove.y};
        if(moveInput.length() > 0.1f) {
            moveInput = moveInput.normalized();
            // Rotate by yaw
            float c = std::cos(yaw), s = std::sin(yaw);
            Vec3 worldMove{moveInput.x*c - moveInput.z*s, 0, moveInput.x*s + moveInput.z*c};
            velocity = worldMove * moveSpeed * (isSprinting?1.8f:1.0f) * (stamina/100.0f*0.5f+0.5f);
            isMoving = true;
        } else {
            velocity = velocity * 0.85f;
            isMoving = false;
        }

        position = position + velocity*dt;
        // Terrain collision
        float groundH = sim.getTerrain().getHeightWorld(position.x, position.z);
        position.y = groundH + 1.8f; // eye height

        // Stamina
        if(isSprinting && isMoving) stamina = std::max(0.0f, stamina - dt*8.0f);
        else stamina = std::min(100.0f, stamina + dt*6.0f);

        if(stamina < 5) isSprinting = false;

        // Heat and smoke exposure
        Vec3 grid = sim.getTerrain().worldToGrid(position);
        int gx=(int)grid.x, gy=(int)grid.z;
        float nearbyHeat = 0;
        float nearbySmoke = 0;
        for(int dy=-3; dy<=3; ++dy){
            for(int dx=-3; dx<=3; ++dx){
                int nx=gx+dx, ny=gy+dy;
                if(nx<0||nx>=sim.width()||ny<0||ny>=sim.height()) continue;
                auto& cell = sim.getCell(nx,ny);
                if(cell.state==FireState::COMBUSTION) {
                    float dist = std::sqrt(float(dx*dx+dy*dy));
                    nearbyHeat += cell.intensity / (dist+1);
                    nearbySmoke += cell.smokeDensity / (dist+1);
                }
            }
        }

        // Apply PPE protection
        float heatProtection = ppe.getHeatProtection();
        float smokeProtection = ppe.getSmokeProtection();

        float heatDamage = nearbyHeat*0.01f * (1.0f-heatProtection);
        if(nearbyHeat>50) {
            health -= heatDamage*dt;
            // Screen shake from heat?
        }

        float smokeInhale = nearbySmoke*0.02f * (1.0f-smokeProtection);
        if(smokeInhale>0.1f) {
            stamina -= smokeInhale*dt*10;
            // Cough
        }

        // Hydration
        hydration -= dt*0.05f * (isSprinting?2.0f:1.0f) * (weather.temperature/30.0f);
        if(hydration<20) {
            stamina -= dt*2.0f;
        }

        // Tool use
        if(isUsingTool && currentTool>=0 && currentTool<(int)tools.size()) {
            auto& tool = tools[currentTool];
            tool->isActive = true;
            tool->update(dt);
            Vec3 target = position + forward*tool->spec.rangeM;
            float used = tool->use(dt, target - position);
            if(tool->spec.waterFlowLps>0) {
                // Apply water to simulation
                sim.applyWaterWorld(target, used*10.0f, 2.5f);
            } else if(tool->spec.type==ToolType::PULASKI || tool->spec.type==ToolType::MCLEOD || tool->spec.type==ToolType::SHOVEL) {
                // Create firebreak
                auto* hand = dynamic_cast<HandTool*>(tool.get());
                if(hand) {
                    float prog = hand->createFirebreakProgress(dt);
                    if(prog>0) {
                        sim.createFirebreakWorld(target, 2.0f);
                    }
                }
            }
        } else {
            if(currentTool>=0 && currentTool<(int)tools.size()) tools[currentTool]->isActive=false;
        }

        // Vehicle interaction
        if(inVehicle && currentVehicle) {
            position = currentVehicle->position + Vec3{0,1.5f,0};
            // Transfer controls to vehicle
            if(inputMove.length()>0) {
                currentVehicle->accelerate(inputMove.y);
                currentVehicle->steer(inputMove.x);
            }
        }

        // Health regen slow if safe
        if(nearbyHeat<10 && health<100) health = std::min(100.0f, health + dt*0.5f);
    }

    void setInput(Vec2 move, bool sprint, bool useTool) {
        inputMove = move;
        isSprinting = sprint;
        isUsingTool = useTool;
    }

    void look(float dyaw, float dpitch) {
        yaw += dyaw;
        pitch = clamp(pitch+dpitch, -85*DEG2RAD, 85*DEG2RAD);
        // Update forward
        forward = Vec3{std::sin(yaw)*std::cos(pitch), std::sin(pitch), std::cos(yaw)*std::cos(pitch)}.normalized();
    }

    void switchTool(int idx) {
        if(idx>=0 && idx<(int)tools.size()) currentTool = idx;
    }

    void enterVehicle(Vehicle* veh) {
        if(veh && (veh->position - position).length() < 5.0f) {
            inVehicle = true;
            currentVehicle = veh;
            veh->startEngine();
        }
    }

    void exitVehicle() {
        if(inVehicle) {
            inVehicle = false;
            if(currentVehicle) {
                position = currentVehicle->position + Vec3{2,0,0};
                currentVehicle = nullptr;
            }
        }
    }

    Vec3 position;
    Vec3 velocity{0,0,0};
    Vec3 forward{0,0,1};
    float yaw=0, pitch=0;
    float moveSpeed = 4.0f;
    Vec2 inputMove{0,0};
    bool isSprinting=false;
    bool isMoving=false;
    bool isUsingTool=false;

    float health, stamina, hydration;
    PPE ppe;
    std::vector<std::unique_ptr<Tool>> tools;
    int currentTool=0;

    bool inVehicle=false;
    Vehicle* currentVehicle=nullptr;

    float time=0;

private:
    float clamp(float v,float lo,float hi){ return std::max(lo,std::min(hi,v)); }
};

} // namespace Fireline
