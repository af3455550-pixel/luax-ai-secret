#pragma once
#include "Dispatch.hpp"
#include "simulation/FireSimulation.hpp"
#include "simulation/Weather.hpp"
#include "TeamAI.hpp"
#include <string>
#include <vector>
#include <functional>

namespace Fireline {

enum class MissionPhase {
    DISPATCH,
    ENROUTE,
    INITIAL_ATTACK,
    ESCALATION,
    STRUCTURE_PROTECTION,
    EVACUATION,
    MOPUP,
    COMPLETED,
    FAILED
};

inline std::string phaseToString(MissionPhase p){
    switch(p){
        case MissionPhase::DISPATCH: return "DESPACHO";
        case MissionPhase::ENROUTE: return "DESLOCAMENTO";
        case MissionPhase::INITIAL_ATTACK: return "ATAQUE INICIAL";
        case MissionPhase::ESCALATION: return "INCENDIO FORA DE CONTROLO";
        case MissionPhase::STRUCTURE_PROTECTION: return "PROTECAO DE ESTRUTURAS";
        case MissionPhase::EVACUATION: return "EVACUACAO NECESSARIA";
        case MissionPhase::MOPUP: return "RESCALDO";
        case MissionPhase::COMPLETED: return "CONCLUIDA";
        case MissionPhase::FAILED: return "FALHADA";
    }
    return "UNKNOWN";
}

struct MissionObjective {
    std::string description;
    bool completed = false;
    bool optional = false;
    float progress = 0; // 0-1
    MissionObjective() = default;
    MissionObjective(const std::string& d, bool c, bool o, float p)
        : description(d), completed(c), optional(o), progress(p) {}
};

class Mission {
public:
    Mission(const DispatchCall& call) : dispatch(call) {
        phase = MissionPhase::DISPATCH;
        time = 0;
        generateObjectives();
    }

    void generateObjectives() {
        objectives.clear();
        objectives.push_back(MissionObjective{"Chegar ao local do incêndio", false, false, 0});
        objectives.push_back(MissionObjective{"Conter 80% do perímetro", false, false, 0});
        if(dispatch.civiliansAtRisk>0) {
            objectives.push_back(MissionObjective{"Evacuar " + std::to_string(dispatch.civiliansAtRisk) + " civis", false, false, 0});
        }
        objectives.push_back(MissionObjective{"Proteger estruturas ameaçadas", false, true, 0});
        objectives.push_back(MissionObjective{"Eliminar pontos quentes - rescaldo", false, false, 0});
    }

    void update(float dt, FireSimulation& sim, WeatherSystem& weather, TeamManager& team, const Vec3& playerPos) {
        time += dt;
        // Phase transitions based on simulation
        float burningCells = sim.stats.combustion;
        float totalIntensity = sim.stats.totalIntensity;

        switch(phase){
            case MissionPhase::DISPATCH:
                if(time>5.0f) phase = MissionPhase::ENROUTE;
                break;
            case MissionPhase::ENROUTE:
                {
                    float distToFire = (playerPos - dispatch.location).length();
                    if(distToFire < 50.0f) {
                        phase = MissionPhase::INITIAL_ATTACK;
                        addLog("Chegada ao local - iniciando ataque inicial");
                    }
                }
                break;
            case MissionPhase::INITIAL_ATTACK:
                // Check if fire escalates due to wind
                if(weather.current.windSpeed > 25.0f && burningCells > 100) {
                    phase = MissionPhase::ESCALATION;
                    addLog("ALERTA: Vento aumentou para " + std::to_string((int)weather.current.windSpeed) + " km/h - INCÊNDIO FORA DE CONTROLO");
                    // Add new objective
                    objectives.push_back(MissionObjective{"Estabelecer linha de contenção alternativa", false, false, 0});
                } else if(sim.stats.burnedAreaHa > dispatch.fireSizeHa*0.5f && dispatch.civiliansAtRisk>0) {
                    phase = MissionPhase::EVACUATION;
                    addLog("EVACUATION REQUIRED - Fogo aproxima-se de " + dispatch.locationName);
                } else if(burningCells < 20) {
                    phase = MissionPhase::MOPUP;
                    addLog("Fogo controlado - iniciando rescaldo");
                }
                break;
            case MissionPhase::ESCALATION:
                if(dispatch.civiliansAtRisk>0) {
                    phase = MissionPhase::EVACUATION;
                    addLog("Fogo ameaça aldeia - EVACUAÇÃO IMEDIATA");
                } else if(sim.stats.combustion > 0 && sim.stats.burnedAreaHa > dispatch.fireSizeHa*2) {
                    // Road blocked scenario
                    addLog("Estrada principal bloqueada - ROTA ALTERNATIVA NECESSÁRIA");
                    objectives.push_back(MissionObjective{"Encontrar rota alternativa", false, false, 0});
                    phase = MissionPhase::STRUCTURE_PROTECTION;
                }
                if(burningCells < 30) phase = MissionPhase::MOPUP;
                break;
            case MissionPhase::EVACUATION:
                // Check evacuation progress
                if(evacuatedCivilians >= dispatch.civiliansAtRisk) {
                    addLog("Evacuação concluída");
                    phase = MissionPhase::STRUCTURE_PROTECTION;
                }
                // If fire gets too close to structures
                if(totalIntensity > 1000) {
                    phase = MissionPhase::STRUCTURE_PROTECTION;
                    addLog("PROTECT STRUCTURES - Casas ameaçadas");
                }
                break;
            case MissionPhase::STRUCTURE_PROTECTION:
                if(structuresSaved >= structuresAtRisk) {
                    phase = MissionPhase::MOPUP;
                    addLog("Estruturas protegidas - iniciando rescaldo");
                }
                if(burningCells==0) phase = MissionPhase::MOPUP;
                break;
            case MissionPhase::MOPUP:
                {
                    int hotspots = sim.stats.mopup;
                    if(hotspots==0 && burningCells==0) {
                        phase = MissionPhase::COMPLETED;
                        addLog("Missão concluída - todos os pontos quentes eliminados");
                    }
                    // Check reignition
                    if(burningCells>10) {
                        addLog("REACENDIMENTO DETETADO - retornar ao ataque");
                        phase = MissionPhase::INITIAL_ATTACK;
                    }
                }
                break;
            default: break;
        }

        // Update objectives progress
        updateObjectives(sim);
    }

    void updateObjectives(FireSimulation& sim) {
        // Objective 0: arrive
        if(phase != MissionPhase::DISPATCH && phase != MissionPhase::ENROUTE) {
            objectives[0].completed = true;
            objectives[0].progress = 1.0f;
        }
        // Containment - based on firebreaks vs burning
        float containment = 1.0f - (float)sim.stats.combustion / (sim.stats.combustion + sim.stats.burned + 1);
        if(objectives.size()>1) {
            objectives[1].progress = clamp(containment,0.0f,1.0f);
            if(containment>0.8f) objectives[1].completed = true;
        }
    }

    void addLog(const std::string& msg){
        logs.push_back("[" + std::to_string((int)time/60) + ":" + std::to_string((int)time%60) + "] " + msg);
        if(logs.size()>20) logs.erase(logs.begin());
    }

    void evacuateCivilian(){ evacuatedCivilians++; }
    void saveStructure(){ structuresSaved++; }

    DispatchCall dispatch;
    MissionPhase phase = MissionPhase::DISPATCH;
    float time = 0;
    std::vector<MissionObjective> objectives;
    std::vector<std::string> logs;
    int evacuatedCivilians = 0;
    int structuresAtRisk = 3;
    int structuresSaved = 0;
    bool alternativeRouteFound = false;

private:
    float clamp(float v,float lo,float hi){ return std::max(lo,std::min(hi,v)); }
};

} // namespace Fireline
