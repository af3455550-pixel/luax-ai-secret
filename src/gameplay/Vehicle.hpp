#pragma once
#include "core/Math.hpp"
#include "Tools.hpp"
#include <string>
#include <vector>
#include <memory>

namespace Fireline {

enum class VehicleType {
    FOREST_TRUCK = 0,      // Veículo de combate a incêndios florestais
    FIRE_ENGINE = 1,       // Caminhão de bombeiros
    LIGHT_INTERVENTION = 2,// Veículo ligeiro de intervenção
    TANKER = 3,            // Caminhão-cisterna
    COMMAND = 4,           // Veículo de comando
    AMBULANCE = 5,         // Ambulância
    HELICOPTER = 6         // Helicóptero de combate
};

struct VehicleSpec {
    VehicleType type;
    std::string name;
    float massKg;
    float maxSpeedKmh;
    float enginePowerKw;
    float waterCapacityL;
    float pumpFlowLps;
    float fuelCapacityL;
    int crewCapacity;
    float traction; // 0-1 offroad
    float suspensionTravel;
    bool hasHose;
    bool hasPump;
    bool hasSiren;
    bool hasRadio;
};

inline VehicleSpec getVehicleSpec(VehicleType t){
    switch(t){
        case VehicleType::FOREST_TRUCK: return {t,"VFCI - Unimog 4x4",8500,85,220,2500,20,200,3,0.95f,0.35f,true,true,true,true};
        case VehicleType::FIRE_ENGINE: return {t,"Caminhão ABTF",14000,100,300,4000,30,300,6,0.6f,0.2f,true,true,true,true};
        case VehicleType::LIGHT_INTERVENTION: return {t,"VLCI - Pickup 4x4",3200,120,150,600,8,80,4,0.9f,0.3f,true,false,true,true};
        case VehicleType::TANKER: return {t,"Caminhão-Cisterna 12000L",18000,80,280,12000,40,400,2,0.5f,0.18f,true,true,true,true};
        case VehicleType::COMMAND: return {t,"Veículo Comando",2800,130,130,0,0,70,5,0.75f,0.25f,false,false,true,true};
        case VehicleType::AMBULANCE: return {t,"Ambulância 4x4",3500,120,110,0,0,80,4,0.7f,0.25f,false,false,true,true};
        case VehicleType::HELICOPTER: return {t,"Heli Bambi Bucket",3500,220,800,1200,50,600,3,1.0f,0,true,true,true,true};
    }
    return {t,"Desconhecido",5000,80,150,1000,10,100,2,0.5f,0.2f,true,true,true,true};
}

struct Wheel {
    Vec3 localPos;
    float radius = 0.4f;
    float suspensionCompression = 0.0f;
    float suspensionForce = 0.0f;
    float rotation = 0.0f;
    float steerAngle = 0.0f;
    bool isGrounded = false;
    float grip = 1.0f;
};

class Vehicle {
public:
    Vehicle(VehicleType type, Vec3 pos) : spec(getVehicleSpec(type)), position(pos) {
        // Initialize wheels
        int wheelCount = (type==VehicleType::HELICOPTER)?0:4;
        if(type==VehicleType::FOREST_TRUCK || type==VehicleType::FIRE_ENGINE || type==VehicleType::TANKER) wheelCount=6;
        wheels.resize(wheelCount);
        for(int i=0;i<wheelCount;i++){
            wheels[i].localPos = Vec3{(i%2==0?-1.0f:1.0f)*1.0f, -0.5f, (i/2 - wheelCount/4)*1.2f};
            wheels[i].radius = (type==VehicleType::TANKER)?0.55f:0.45f;
        }
        waterLevel = spec.waterCapacityL;
        fuelLevel = spec.fuelCapacityL;
        if(spec.hasHose) {
            hose = std::make_unique<Hose>(type==VehicleType::LIGHT_INTERVENTION);
        }
    }

    virtual ~Vehicle() = default;

    void update(float dt, const class Terrain* terrain) {
        // Physics update
        // Simple suspension and traction
        velocity = velocity + acceleration*dt;
        velocity = velocity * (1.0f - drag*dt);
        position = position + velocity*dt;

        // Terrain interaction
        if(terrain) {
            float groundHeight = terrain->getHeightWorld(position.x, position.z);
            if(position.y < groundHeight + 1.0f) {
                position.y = groundHeight + 1.0f;
                velocity.y = std::max(0.0f, velocity.y);
                isGrounded = true;
            } else {
                isGrounded = false;
                if(spec.type != VehicleType::HELICOPTER) {
                    velocity.y -= 9.81f*dt;
                }
            }
            // Traction based on slope
            // float slope = terrain->getSlope... but simplified
        }

        // Engine and fuel
        if(isEngineOn) {
            fuelLevel = std::max(0.0f, fuelLevel - spec.enginePowerKw*0.0001f*dt);
            if(fuelLevel<=0) isEngineOn=false;
        }

        // Pump
        if(isPumpOn && spec.hasPump) {
            if(waterLevel>0) {
                waterLevel = std::max(0.0f, waterLevel - spec.pumpFlowLps*dt);
            } else {
                isPumpOn=false;
            }
        }

        // Siren
        if(sirenOn) sirenTimer += dt;

        // Hose
        if(hose) hose->update(dt);

        // Wheels
        for(auto& w : wheels) {
            w.rotation += velocity.length()*dt / w.radius;
            w.suspensionCompression = clamp(w.suspensionCompression + (randFloat()-0.5f)*dt, -spec.suspensionTravel, spec.suspensionTravel);
        }

        // Helicopter special
        if(spec.type==VehicleType::HELICOPTER) {
            rotorAngle += dt*20.0f;
            if(isEngineOn) {
                // Hover logic
                float targetAlt = 30.0f;
                float altError = targetAlt - (position.y - (terrain?terrain->getHeightWorld(position.x,position.z):0));
                velocity.y += altError*dt*0.5f;
            }
        }

        time += dt;
    }

    void startEngine(){ if(fuelLevel>0) isEngineOn=true; }
    void stopEngine(){ isEngineOn=false; }
    void setSiren(bool on){ sirenOn=on; }
    void setPump(bool on){ if(spec.hasPump) isPumpOn=on; }
    void setLights(bool on){ lightsOn=on; }

    // Driving
    void accelerate(float amount){ // -1 to 1
        if(!isEngineOn) return;
        float tractionFactor = spec.traction * (isGrounded?1.0f:0.1f);
        Vec3 forwardDir{std::sin(yaw),0,std::cos(yaw)};
        acceleration = forwardDir * amount * spec.enginePowerKw * 0.01f * tractionFactor;
        // Limit speed
        float maxSpeedMs = spec.maxSpeedKmh/3.6f;
        if(velocity.length() > maxSpeedMs) {
            velocity = velocity.normalized()*maxSpeedMs;
        }
    }

    void steer(float amount){ // -1 to 1
        yaw += amount * dtSteer * (velocity.length()/5.0f + 0.5f);
        for(auto& w : wheels) {
            if(w.localPos.z > 0) w.steerAngle = amount * 0.5f; // front wheels steer
        }
    }

    void brake(float amount){
        velocity = velocity * (1.0f - amount*0.1f);
    }

    // Water operations
    float useWater(float liters) {
        float used = std::min(liters, waterLevel);
        waterLevel -= used;
        return used;
    }

    void refillWater(float liters){
        waterLevel = std::min(spec.waterCapacityL, waterLevel+liters);
    }

    bool canRefillFromHydrant(Vec3 hydrantPos) const {
        return (position - hydrantPos).length() < 10.0f;
    }

    // Radio
    void radioMessage(const std::string& msg){
        lastRadioMessage = msg;
        radioTimer = 5.0f;
    }

    VehicleSpec spec;
    Vec3 position;
    Vec3 velocity{0,0,0};
    Vec3 acceleration{0,0,0};
    float yaw = 0; // rotation Y
    float pitch = 0, roll = 0;
    float drag = 0.2f;
    float dtSteer = 1.5f;
    bool isGrounded = true;
    bool isEngineOn = false;
    bool sirenOn = false;
    bool lightsOn = false;
    bool isPumpOn = false;
    float waterLevel = 0;
    float fuelLevel = 0;
    float sirenTimer = 0;
    float rotorAngle = 0;
    float time = 0;
    std::vector<Wheel> wheels;
    std::unique_ptr<Hose> hose;
    std::string lastRadioMessage;
    float radioTimer = 0;

private:
    float randFloat(){ return static_cast<float>(rand())/RAND_MAX; }
};

} // namespace Fireline
