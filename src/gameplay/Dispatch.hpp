#pragma once
#include "core/Math.hpp"
#include "simulation/FireTypes.hpp"
#include "Vehicle.hpp"
#include <string>
#include <vector>
#include <random>

namespace Fireline {

struct DispatchCall {
    std::string id;
    Vec3 location;
    std::string locationName;
    float fireSizeHa = 0;
    Vec2 wind;
    float windSpeedKmh = 0;
    ThreatLevel threat = ThreatLevel::MODERATE;
    int civiliansAtRisk = 0;
    int civiliansTotal = 0;
    std::string cause; // vehicle, lightning, accident
    std::string description;
    float timeSinceReport = 0; // minutes
    std::vector<VehicleType> recommendedUnits;
    std::vector<VehicleType> availableUnits;
    bool isActive = false;
    bool isCompleted = false;
};

class DispatchCenter {
public:
    DispatchCenter() {
        rng.seed(1337);
        // Predefine locations
        locations = {
            {"Torre Vigia Norte", Vec3{-200,0,300}},
            {"Vale do Rio Seco", Vec3{150,0,-100}},
            {"Aldeia de Pedra Alta", Vec3{-80,0,-250}},
            {"Fazenda Santa Clara", Vec3{300,0,200}},
            {"Estrada Florestal KM 42", Vec3{0,0,0}},
            {"Ponte do Lobo", Vec3{-350,0,50}},
            {"Serra da Bruma", Vec3{400,0,-300}},
            {"Acampamento Base", Vec3{50,0,400}},
            {"Área Industrial Sul", Vec3{-100,0,500}},
            {"Lago Escondido", Vec3{250,0,350}}
        };
        causes = {"Veículo em chamas", "Relâmpago", "Queimada descontrolada", "Acidente industrial", "Fogueira abandonada", "Linha elétrica caída"};
    }

    DispatchCall generateCall(float difficulty=0.5f) {
        DispatchCall call;
        call.id = "INC-" + std::to_string(1000 + (rand()%9000));

        // Pick location
        int locIdx = rand()%locations.size();
        call.location = locations[locIdx].pos + Vec3{randRange(-50,50),0,randRange(-50,50)};
        call.locationName = locations[locIdx].name;

        // Fire size based on difficulty
        float sizeRoll = randFloat();
        if(sizeRoll < 0.4f) call.fireSizeHa = randRange(0.1f, 2.0f);
        else if(sizeRoll < 0.7f) call.fireSizeHa = randRange(2.0f, 10.0f);
        else if(sizeRoll < 0.9f) call.fireSizeHa = randRange(10.0f, 50.0f);
        else call.fireSizeHa = randRange(50.0f, 300.0f);

        call.windSpeedKmh = randRange(5, 45) * (0.5f + difficulty);
        float windAngle = randRange(0, 360) * DEG2RAD;
        call.wind = Vec2{std::cos(windAngle), std::sin(windAngle)};

        // Threat level
        float threatScore = call.fireSizeHa*0.1f + call.windSpeedKmh*0.2f + difficulty*2;
        if(threatScore < 2) call.threat = ThreatLevel::LOW;
        else if(threatScore < 5) call.threat = ThreatLevel::MODERATE;
        else if(threatScore < 10) call.threat = ThreatLevel::HIGH;
        else if(threatScore < 18) call.threat = ThreatLevel::VERY_HIGH;
        else call.threat = ThreatLevel::EXTREME;

        call.civiliansAtRisk = (call.threat >= ThreatLevel::HIGH) ? rand()%15 : rand()%3;
        call.civiliansTotal = call.civiliansAtRisk + rand()%5;
        call.cause = causes[rand()%causes.size()];

        call.description = generateDescription(call);
        call.timeSinceReport = randRange(2,15);

        // Recommend units
        if(call.fireSizeHa < 2) {
            call.recommendedUnits = {VehicleType::LIGHT_INTERVENTION, VehicleType::FOREST_TRUCK};
        } else if(call.fireSizeHa < 10) {
            call.recommendedUnits = {VehicleType::FOREST_TRUCK, VehicleType::TANKER, VehicleType::COMMAND};
        } else if(call.fireSizeHa < 50) {
            call.recommendedUnits = {VehicleType::FOREST_TRUCK, VehicleType::FOREST_TRUCK, VehicleType::TANKER, VehicleType::COMMAND, VehicleType::HELICOPTER};
        } else {
            call.recommendedUnits = {VehicleType::FOREST_TRUCK, VehicleType::FOREST_TRUCK, VehicleType::FIRE_ENGINE, VehicleType::TANKER, VehicleType::TANKER, VehicleType::COMMAND, VehicleType::HELICOPTER, VehicleType::AMBULANCE};
        }

        call.availableUnits = {
            VehicleType::FOREST_TRUCK, VehicleType::FIRE_ENGINE, VehicleType::LIGHT_INTERVENTION,
            VehicleType::TANKER, VehicleType::COMMAND, VehicleType::AMBULANCE, VehicleType::HELICOPTER
        };

        call.isActive = true;
        return call;
    }

    std::string generateDescription(const DispatchCall& call) {
        std::string desc;
        desc += "Incêndio florestal reportado em " + call.locationName + ". ";
        if(call.fireSizeHa < 1) desc += "Foco pequeno, ";
        else if(call.fireSizeHa < 10) desc += "Incêndio em progressão, ";
        else desc += "Grande incêndio fora de controlo, ";

        desc += "causa provável: " + call.cause + ". ";
        desc += "Vento de " + std::to_string((int)call.windSpeedKmh) + " km/h. ";
        if(call.civiliansAtRisk>0) desc += std::to_string(call.civiliansAtRisk) + " civis em risco. ";
        desc += "Nível de ameaça: " + threatToString(call.threat) + ".";
        return desc;
    }

    void addCall(const DispatchCall& call){ activeCalls.push_back(call); }
    void clearCalls(){ activeCalls.clear(); }

    std::vector<DispatchCall> activeCalls;

private:
    struct Loc { std::string name; Vec3 pos; };
    std::vector<Loc> locations;
    std::vector<std::string> causes;
    std::mt19937 rng;

    float randFloat(){ std::uniform_real_distribution<float> d(0,1); return d(rng); }
    float randRange(float lo,float hi){ return lo + (hi-lo)*randFloat(); }
    int rand(){ std::uniform_int_distribution<int> d(0,1000000); return d(rng); }
};

} // namespace Fireline
