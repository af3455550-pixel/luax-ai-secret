#pragma once
#include "core/Math.hpp"
#include "gameplay/Mission.hpp"
#include "simulation/FireSimulation.hpp"
#include "simulation/Weather.hpp"
#include <string>
#include <vector>

namespace Fireline {

class HUD {
public:
    HUD() {}

    std::string renderConsole(const Vec3& playerPos, float health, float stamina, float hydration,
                              const FireSimulation& sim, const WeatherState& weather,
                              const Mission& mission, int toolIdx, float waterLevel) {
        std::string out;
        out += "\033[1m=== FIRELINE: WILDFIRE COMMAND ===\033[0m\n";
        out += "Pos: " + std::to_string((int)playerPos.x) + "," + std::to_string((int)playerPos.z) + "  Health: " + std::to_string((int)health) + "%  Stamina: " + std::to_string((int)stamina) + "%  Hydra: " + std::to_string((int)hydration) + "%\n";
        out += "Fogo: " + std::to_string(sim.stats.combustion) + " celulas ativas  Intensidade: " + std::to_string((int)sim.stats.totalIntensity) + " kW  Area queimada: " + std::to_string(sim.stats.burnedAreaHa) + " ha\n";
        out += "Clima: Vento " + std::to_string((int)weather.windSpeed) + " km/h dir (" + std::to_string((int)(weather.windDirection.x*100)) + "," + std::to_string((int)(weather.windDirection.y*100)) + ")  Temp: " + std::to_string((int)weather.temperature) + "C  Hum: " + std::to_string((int)weather.humidity) + "%";
        if(weather.isStorm) out += "  [TEMPESTADE]";
        if(weather.isHeatWave) out += "  [ONDA DE CALOR]";
        out += "\n";

        out += "Missão: " + mission.dispatch.id + " | " + mission.dispatch.locationName + " | Fase: " + phaseToString(mission.phase) + " | Ameaça: " + threatToString(mission.dispatch.threat) + "\n";
        out += "Objetivos:\n";
        for(auto& obj : mission.objectives) {
            out += (obj.completed?" [X] ":" [ ] ") + obj.description + " (" + std::to_string((int)(obj.progress*100)) + "%)\n";
        }

        out += "\nFerramenta atual: " + std::to_string(toolIdx) + "  Agua: " + std::to_string((int)waterLevel) + "L\n";
        out += "Comandos: WASD mover, SHIFT correr, ESPACO usar ferramenta, 1-4 trocar ferramenta, C trocar camara, M mapa, R radio, V entrar/sair veiculo, Q sair, H ajuda\n";

        if(!mission.logs.empty()) {
            out += "\n--- LOGS ---\n";
            for(auto& log : mission.logs) out += log + "\n";
        }

        // Mini map ASCII of fire
        out += "\n--- MINI MAPA DE FOGO (25x25) ---\n";
        Vec3 grid = sim.getTerrain().worldToGrid(playerPos);
        int gx=(int)grid.x, gy=(int)grid.z;
        int rad=12;
        for(int y=gy+rad; y>=gy-rad; --y){
            for(int x=gx-rad; x<=gx+rad; ++x){
                if(x==gx && y==gy) { out += "@"; continue; }
                if(x<0||x>=sim.width()||y<0||y>=sim.height()) { out += " "; continue; }
                const auto& cell = sim.getCell(x,y);
                char c=' ';
                switch(cell.state){
                    case FireState::NORMAL: 
                        if(cell.vegProfile.type==VegetationType::PINE_MATURE) c='T';
                        else if(cell.vegProfile.type==VegetationType::GRASS_DRY) c='.';
                        else c=':';
                        break;
                    case FireState::HEATING: c='~'; break;
                    case FireState::SMOKE: c='s'; break;
                    case FireState::COMBUSTION: c='#'; break;
                    case FireState::BURNED: c='x'; break;
                    case FireState::MOPUP: c='*'; break;
                }
                if(cell.hasFirebreak) c='=';
                out += c;
            }
            out += "\n";
        }

        return out;
    }
};

} // namespace Fireline
