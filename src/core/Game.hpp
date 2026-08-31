#pragma once
#include "simulation/FireSimulation.hpp"
#include "simulation/Weather.hpp"
#include "world/Map.hpp"
#include "gameplay/Player.hpp"
#include "gameplay/Vehicle.hpp"
#include "gameplay/TeamAI.hpp"
#include "gameplay/Dispatch.hpp"
#include "gameplay/Mission.hpp"
#include "graphics/Camera.hpp"
#include "graphics/ParticleSystem.hpp"
#include "graphics/Credits.hpp"
#include "audio/AudioSystem.hpp"
#include "ui/HUD.hpp"
#include <memory>
#include <vector>

namespace Fireline {

enum class GameState {
    MAIN_MENU,
    DISPATCH_SCREEN,
    IN_MISSION,
    PAUSED,
    CREDITS,
    EXIT
};

class Game {
public:
    Game() {
        // Initialize core systems
        fireSim = std::make_unique<FireSimulation>(256,256,2.0f);
        weather = std::make_unique<WeatherSystem>();
        gameMap = std::make_unique<GameMap>();
        player = std::make_unique<Player>(Vec3{0,0,0});
        camera = std::make_unique<CameraSystem>();
        particles = std::make_unique<ParticleSystem>(10000);
        audio = std::make_unique<AudioSystem>();
        dispatch = std::make_unique<DispatchCenter>();
        team = std::make_unique<TeamManager>();
        hud = std::make_unique<HUD>();
        credits = std::make_unique<CreditsSystem>();

        // Add team members
        for(int i=0;i<4;i++){
            team->addMember(Vec3{randRange(-5,5),0,randRange(-5,5)});
        }

        // Vehicles
        spawnVehicles();

        state = GameState::MAIN_MENU;
        time = 0;
    }

    void spawnVehicles() {
        vehicles.clear();
        vehicles.push_back(std::make_unique<Vehicle>(VehicleType::FOREST_TRUCK, Vec3{5,0,5}));
        vehicles.push_back(std::make_unique<Vehicle>(VehicleType::TANKER, Vec3{10,0,5}));
        vehicles.push_back(std::make_unique<Vehicle>(VehicleType::LIGHT_INTERVENTION, Vec3{15,0,5}));
        vehicles.push_back(std::make_unique<Vehicle>(VehicleType::COMMAND, Vec3{0,0,10}));
        vehicles.push_back(std::make_unique<Vehicle>(VehicleType::AMBULANCE, Vec3{-5,0,5}));
        vehicles.push_back(std::make_unique<Vehicle>(VehicleType::HELICOPTER, Vec3{20,0,20}));
        for(auto& v : vehicles) v->startEngine();
    }

    void startNewMission(float difficulty=0.5f) {
        auto call = dispatch->generateCall(difficulty);
        dispatch->addCall(call);
        currentMission = std::make_unique<Mission>(call);

        // Ignite fire at mission location
        fireSim->igniteWorld(call.location, std::sqrt(call.fireSizeHa*10000)/fireSim->getCellSize());

        // Place player near fire station
        player->position = Vec3{0,0,0};

        // Reset team near player
        team->members.clear();
        for(int i=0;i<4;i++){
            team->addMember(player->position + Vec3{randRange(-3,3),0,randRange(-3,3)});
        }

        state = GameState::IN_MISSION;
        addLog("Nova missão: " + call.id + " em " + call.locationName);
    }

    void update(float dt) {
        time += dt;

        switch(state){
            case GameState::MAIN_MENU:
                // Auto-start demo after 2 sec
                if(time>1.0f) {
                    startNewMission(0.6f);
                }
                break;
            case GameState::DISPATCH_SCREEN:
                break;
            case GameState::IN_MISSION:
                updateMission(dt);
                break;
            case GameState::CREDITS:
                credits->update(dt, skipCredits);
                if(credits->isFinished()) {
                    state = GameState::EXIT;
                }
                break;
            default: break;
        }
    }

    void updateMission(float dt) {
        if(!currentMission) return;

        // Update weather
        weather->update(dt);
        if(weather->pendingLightningStrike) {
            fireSim->igniteWorld(Vec3{weather->lightningPos.x,0,weather->lightningPos.y}, 3.0f);
            weather->pendingLightningStrike = false;
            audio->playSound(SoundType::EXPLOSION, Vec3{weather->lightningPos.x,0,weather->lightningPos.y});
        }

        // Update fire
        fireSim->update(dt, weather->current);

        // Update player
        player->update(dt, *fireSim, weather->current);

        // Update vehicles
        for(auto& v : vehicles) {
            v->update(dt, &fireSim->getTerrain());
        }

        // Update team AI
        std::vector<Vec3> civs; // would come from civilians system
        // Simulate civilians near mission location
        if(currentMission->dispatch.civiliansAtRisk>0) {
            for(int i=0;i<currentMission->dispatch.civiliansAtRisk;i++){
                civs.push_back(currentMission->dispatch.location + Vec3{randRange(-30,30),0,randRange(-30,30)});
            }
        }
        team->update(dt, *fireSim, weather->current, civs);

        // Update mission logic
        currentMission->update(dt, *fireSim, *weather, *team, player->position);

        // Update camera
        Vec3 playerForward = player->forward;
        Vec3 vehiclePos = player->inVehicle && player->currentVehicle ? player->currentVehicle->position : Vec3{0,0,0};
        camera->update(dt, player->position, playerForward, vehiclePos, player->inVehicle);

        // Update particles based on fire
        for(auto& cell : fireSim->getCells()) {
            if(cell.state==FireState::COMBUSTION && randFloat()<0.02f) {
                particles->emitSmoke(cell.worldPos + Vec3{0,2,0}, Vec3{weather->current.windDirection.x, 2, weather->current.windDirection.y}, 1, cell.intensity/200.0f);
                if(randFloat()<0.01f*cell.vegProfile.emberProduction) {
                    particles->emitEmbers(cell.worldPos + Vec3{0,3,0}, 1);
                }
            } else if(cell.state==FireState::MOPUP && cell.isHotspot) {
                if(randFloat()<0.01f) particles->emitSteam(cell.worldPos, 1);
            }
        }
        particles->emitAsh(Vec3{0,10,0}, weather->current.getWindVector3(), 1);
        particles->update(dt, weather->current.getWindVector3());

        // Update audio
        audio->update(dt, player->position, playerForward, fireSim->stats.totalIntensity);
        // Fire crackle
        if(fireSim->stats.combustion>0 && (int)(time*2)%2==0) {
            // Find nearest fire
            Vec3 grid = fireSim->getTerrain().worldToGrid(player->position);
            int gx=(int)grid.x, gy=(int)grid.z;
            for(int dy=-5;dy<=5;dy++) for(int dx=-5;dx<=5;dx++){
                int nx=gx+dx, ny=gy+dy;
                if(nx<0||nx>=fireSim->width()||ny<0||ny>=fireSim->height()) continue;
                if(fireSim->getCell(nx,ny).state==FireState::COMBUSTION) {
                    audio->playSound(SoundType::FIRE_CRACKLE, fireSim->getCell(nx,ny).worldPos, 0.5f, true);
                    break;
                }
            }
        }

        // Check mission completion
        if(currentMission->phase==MissionPhase::COMPLETED) {
            addLog("Missão concluída com sucesso!");
            // After short delay go to credits
            missionCompleteTimer += dt;
            if(missionCompleteTimer>5.0f) {
                state = GameState::CREDITS;
                credits->reset();
            }
        } else if(currentMission->phase==MissionPhase::FAILED) {
            addLog("Missão falhada!");
            missionCompleteTimer += dt;
            if(missionCompleteTimer>5.0f) state = GameState::MAIN_MENU;
        }
    }

    void handleInput(char input) {
        switch(state){
            case GameState::IN_MISSION:
                handleMissionInput(input);
                break;
            case GameState::CREDITS:
                if(input=='q' || input=='Q' || input==27) skipCredits=true;
                break;
            case GameState::MAIN_MENU:
                if(input=='\n' || input==' ') startNewMission();
                break;
            default: break;
        }
    }

    void handleMissionInput(char c) {
        // Simple WASD for console
        Vec2 move{0,0};
        switch(c){
            case 'w': case 'W': move.y=1; break;
            case 's': case 'S': move.y=-1; break;
            case 'a': case 'A': move.x=-1; break;
            case 'd': case 'D': move.x=1; break;
            case ' ': player->setInput(player->inputMove, player->isSprinting, true); return;
            case 'c': case 'C': cycleCamera(); return;
            case 'v': case 'V': 
                if(player->inVehicle) player->exitVehicle();
                else {
                    // Find nearest vehicle
                    Vehicle* nearest=nullptr;
                    float best=1e9f;
                    for(auto& v: vehicles){
                        float d=(v->position - player->position).length();
                        if(d<best){best=d; nearest=v.get();}
                    }
                    if(nearest && best<10) player->enterVehicle(nearest);
                }
                return;
            case '1': player->switchTool(0); return;
            case '2': player->switchTool(1); return;
            case '3': player->switchTool(2); return;
            case '4': player->switchTool(3); return;
            case 'r': case 'R':
                // Radio command
                team->broadcastCommand(RadioCommand::ATTACK_FIRE, player->position + player->forward*10.0f);
                audio->playRadio("Attack this fire!", player->position);
                return;
            case 'f': case 'F':
                team->broadcastCommand(RadioCommand::FOLLOW_ME, player->position);
                return;
            case 'g': case 'G':
                team->broadcastCommand(RadioCommand::CREATE_FIREBREAK, player->position + player->forward*5.0f);
                return;
            case 'q': case 'Q':
                state = GameState::CREDITS;
                credits->reset();
                return;
            default: break;
        }
        if(move.length()>0) {
            player->setInput(move, false, false);
        } else {
            player->setInput(Vec2{0,0}, false, false);
        }
    }

    void cycleCamera() {
        int next = ((int)camera->mode + 1) % 7;
        camera->setMode((CameraMode)next);
    }

    std::string renderConsole() {
        switch(state){
            case GameState::MAIN_MENU:
                return renderMainMenu();
            case GameState::IN_MISSION:
                return hud->renderConsole(player->position, player->health, player->stamina, player->hydration,
                                          *fireSim, weather->current, *currentMission, player->currentTool,
                                          player->inVehicle && player->currentVehicle ? player->currentVehicle->waterLevel : 0)
                       + "\nParticulas: " + std::to_string(particles->particles.size()) + "  Camera: " + std::to_string((int)camera->mode) + "  Veiculos: " + std::to_string(vehicles.size()) + "\n";
            case GameState::CREDITS:
                return credits->renderConsoleFrame(80,24);
            default:
                return "Estado desconhecido\n";
        }
    }

    std::string renderMainMenu() {
        std::string s;
        s += "\033[2J\033[H";
        s += "\n";
        s += "  ______ _____ _____  ______ _      _____ _   _ ______ \n";
        s += " |  ____|_   _|  __ |  ____| |    |_   _|  | |  ____|\n";
        s += " | |__    | | | |__) | |__  | |      | | |   | | |__   \n";
        s += " |  __|   | | |  _  /|  __| | |      | | | . | |  __|  \n";
        s += " | |     _| |_| |  || |____| |____ _| |_| |  | |____ \n";
        s += " |_|    |_____|_|  |_|______|______|_____|_|  |______|\n";
        s += "\n";
        s += "  WILDFIRE COMMAND - SIMULADOR PROFISSIONAL\n";
        s += "\n";
        s += "\033[38;5;202mFIRELINE: WILDFIRE COMMAND - Simulador Profissional de Bombeiros Florestais\033[0m\n";
        s += "Versão 1.0.0 | C++17 | Motor proprietário Fireline Engine\n\n";
        s += "Pressione ESPAÇO para iniciar missão, C para créditos, Q para sair\n";
        s += "Veículos: VFCI, Caminhão ABTF, VLCI, Cisterna, Comando, Ambulância, Helicóptero\n";
        s += "Ferramentas: Mangueiras, Extintores, Bombas, Machado, Pulaski, McLeod, Motosserra\n";
        s += "Simulação: Vento, Inclinação, Vegetação, Humidade, Temperatura, Rescaldo, Reacendimento\n\n";
        s += "Últimas chamadas de despacho:\n";
        for(int i=0;i<3;i++){
            auto call = dispatch->generateCall();
            s += "  " + call.id + " | " + call.locationName + " | " + std::to_string((int)call.fireSizeHa) + "ha | Vento " + std::to_string((int)call.windSpeedKmh) + "km/h | " + threatToString(call.threat) + "\n";
        }
        s += "\nEquipa: " + std::to_string(team->members.size()) + " bombeiros prontos\n";
        s += "Mapa: 512x512 células (2m cada) = 1.04km² de floresta densa, montanhas, vales, rios, aldeias\n";
        return s;
    }

    void addLog(const std::string& msg){
        if(currentMission) currentMission->addLog(msg);
        logs.push_back(msg);
    }

    // Systems
    std::unique_ptr<FireSimulation> fireSim;
    std::unique_ptr<WeatherSystem> weather;
    std::unique_ptr<GameMap> gameMap;
    std::unique_ptr<Player> player;
    std::unique_ptr<CameraSystem> camera;
    std::unique_ptr<ParticleSystem> particles;
    std::unique_ptr<AudioSystem> audio;
    std::unique_ptr<DispatchCenter> dispatch;
    std::unique_ptr<TeamManager> team;
    std::unique_ptr<HUD> hud;
    std::unique_ptr<CreditsSystem> credits;
    std::unique_ptr<Mission> currentMission;
    std::vector<std::unique_ptr<Vehicle>> vehicles;

    GameState state;
    float time=0;
    float missionCompleteTimer=0;
    bool skipCredits=false;
    std::vector<std::string> logs;

private:
    float randFloat(){ return static_cast<float>(rand())/RAND_MAX; }
    float randRange(float lo,float hi){ return lo + (hi-lo)*randFloat(); }
};

} // namespace Fireline
