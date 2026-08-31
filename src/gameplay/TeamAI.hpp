#pragma once
#include "core/Math.hpp"
#include "simulation/FireSimulation.hpp"
#include "Vehicle.hpp"
#include <string>
#include <vector>
#include <functional>
#include <queue>

namespace Fireline {

enum class AIState {
    IDLE,
    FOLLOW_LEADER,
    MOVE_TO,
    ATTACK_FIRE,
    PROTECT_AREA,
    CREATE_FIREBREAK,
    BRING_HOSE,
    RETREAT,
    SEARCH_AREA,
    RESCUE_CIVILIAN,
    REFILL_WATER,
    MOPUP
};

enum class RadioCommand {
    FOLLOW_ME,
    ATTACK_FIRE,
    PROTECT_AREA,
    CREATE_FIREBREAK,
    BRING_HOSE,
    RETREAT,
    SEARCH_AREA,
    HOLD_POSITION,
    REFILL
};

inline std::string radioCommandToString(RadioCommand c){
    switch(c){
        case RadioCommand::FOLLOW_ME: return "Follow me.";
        case RadioCommand::ATTACK_FIRE: return "Attack this fire.";
        case RadioCommand::PROTECT_AREA: return "Protect this area.";
        case RadioCommand::CREATE_FIREBREAK: return "Create firebreak.";
        case RadioCommand::BRING_HOSE: return "Bring the hose.";
        case RadioCommand::RETREAT: return "Retreat.";
        case RadioCommand::SEARCH_AREA: return "Search the area.";
        case RadioCommand::HOLD_POSITION: return "Hold position.";
        case RadioCommand::REFILL: return "Refill water.";
    }
    return "Unknown";
}

struct AIBlackboard {
    Vec3 targetPos;
    Vec3 leaderPos;
    Vec3 firePos;
    Vec3 protectPos;
    bool hasTarget = false;
    float dangerLevel = 0;
    float fatigue = 0;
    bool lowWater = false;
    bool civilianNearby = false;
    Vec3 civilianPos;
};

class FirefighterAI {
public:
    FirefighterAI(int id, Vec3 pos) : id(id), position(pos), homePos(pos) {
        state = AIState::IDLE;
        health = 100;
        stamina = 100;
        water = 20; // liters backpack
    }

    void update(float dt, const FireSimulation& sim, const WeatherState& weather, const std::vector<Vec3>& civilians) {
        time += dt;
        // Perception
        perceive(sim, weather, civilians);

        // Decision
        switch(state){
            case AIState::IDLE:
                if(blackboard.hasTarget) state = AIState::MOVE_TO;
                else if(blackboard.dangerLevel > 0.7f) state = AIState::RETREAT;
                else if(blackboard.lowWater) state = AIState::REFILL_WATER;
                break;
            case AIState::FOLLOW_LEADER:
                followLeader(dt);
                break;
            case AIState::MOVE_TO:
                moveTo(dt);
                if((position - blackboard.targetPos).length() < 2.0f) {
                    if(blackboard.dangerLevel > 0.5f) state = AIState::ATTACK_FIRE;
                    else state = AIState::IDLE;
                }
                break;
            case AIState::ATTACK_FIRE:
                attackFire(dt, const_cast<FireSimulation&>(sim));
                if(blackboard.dangerLevel < 0.1f) state = AIState::MOPUP;
                if(blackboard.lowWater) state = AIState::REFILL_WATER;
                if(blackboard.dangerLevel > 0.9f) state = AIState::RETREAT;
                break;
            case AIState::PROTECT_AREA:
                protectArea(dt, const_cast<FireSimulation&>(sim));
                break;
            case AIState::CREATE_FIREBREAK:
                createFirebreak(dt, const_cast<FireSimulation&>(sim));
                break;
            case AIState::BRING_HOSE:
                // Move hose to leader
                moveTo(dt);
                break;
            case AIState::RETREAT:
                retreat(dt);
                if(blackboard.dangerLevel < 0.2f) state = AIState::IDLE;
                break;
            case AIState::SEARCH_AREA:
                searchArea(dt, civilians);
                break;
            case AIState::RESCUE_CIVILIAN:
                rescueCivilian(dt);
                break;
            case AIState::REFILL_WATER:
                // Find water source
                break;
            case AIState::MOPUP:
                mopup(dt, const_cast<FireSimulation&>(sim));
                break;
        }

        // Physics
        velocity = velocity * 0.9f; // damping
        position = position + velocity*dt;
        position.y = sim.getTerrain().getHeightWorld(position.x, position.z) + 1.7f;

        // Stamina
        if(velocity.length() > 0.5f) stamina = std::max(0.0f, stamina - dt*2.0f);
        else stamina = std::min(100.0f, stamina + dt*5.0f);

        // Heat damage
        if(blackboard.dangerLevel > 0.8f) {
            health -= dt*5.0f * blackboard.dangerLevel;
        }

        // Radio cooldown
        if(radioCooldown>0) radioCooldown -= dt;
    }

    void giveCommand(RadioCommand cmd, Vec3 pos) {
        lastCommand = cmd;
        commandPos = pos;
        commandTime = time;
        hasCommand = true;

        switch(cmd){
            case RadioCommand::FOLLOW_ME: state = AIState::FOLLOW_LEADER; blackboard.leaderPos = pos; break;
            case RadioCommand::ATTACK_FIRE: state = AIState::ATTACK_FIRE; blackboard.firePos = pos; blackboard.targetPos = pos; blackboard.hasTarget=true; break;
            case RadioCommand::PROTECT_AREA: state = AIState::PROTECT_AREA; blackboard.protectPos = pos; blackboard.targetPos = pos; break;
            case RadioCommand::CREATE_FIREBREAK: state = AIState::CREATE_FIREBREAK; blackboard.targetPos = pos; blackboard.hasTarget=true; break;
            case RadioCommand::BRING_HOSE: state = AIState::BRING_HOSE; blackboard.targetPos = pos; break;
            case RadioCommand::RETREAT: state = AIState::RETREAT; break;
            case RadioCommand::SEARCH_AREA: state = AIState::SEARCH_AREA; blackboard.targetPos = pos; break;
            case RadioCommand::HOLD_POSITION: state = AIState::IDLE; blackboard.hasTarget=false; break;
            case RadioCommand::REFILL: state = AIState::REFILL_WATER; break;
        }
        // Voice line
        radioMessage = radioCommandToString(cmd);
        radioCooldown = 2.0f;
    }

    void perceive(const FireSimulation& sim, const WeatherState& weather, const std::vector<Vec3>& civilians) {
        // Check nearby fire
        Vec3 grid = sim.getTerrain().worldToGrid(position);
        int gx = (int)grid.x, gy = (int)grid.z;
        float maxIntensity = 0;
        Vec3 closestFire{0,0,0};
        bool foundFire = false;
        for(int dy=-10; dy<=10; ++dy){
            for(int dx=-10; dx<=10; ++dx){
                int nx=gx+dx, ny=gy+dy;
                if(nx<0||nx>=sim.width()||ny<0||ny>=sim.height()) continue;
                const auto& cell = sim.getCell(nx,ny);
                if(cell.state==FireState::COMBUSTION) {
                    float dist = std::sqrt(float(dx*dx+dy*dy));
                    float influence = cell.intensity / (dist+1);
                    if(influence > maxIntensity) {
                        maxIntensity = influence;
                        closestFire = cell.worldPos;
                        foundFire = true;
                    }
                }
            }
        }
        blackboard.dangerLevel = clamp(maxIntensity/500.0f, 0.0f, 1.0f);
        if(foundFire) {
            blackboard.firePos = closestFire;
            // Smoke reduces visibility
            blackboard.dangerLevel += weather.fogDensity*0.2f;
        }

        // Water
        blackboard.lowWater = water < 3.0f;

        // Civilians
        blackboard.civilianNearby = false;
        for(auto& civ : civilians) {
            if((civ - position).length() < 15.0f) {
                blackboard.civilianNearby = true;
                blackboard.civilianPos = civ;
                break;
            }
        }
    }

    void followLeader(float dt){
        Vec3 dir = (blackboard.leaderPos - position);
        float dist = dir.length();
        if(dist>1.0f) {
            dir = dir.normalized();
            velocity = dir * 3.0f * (stamina/100.0f);
        } else {
            velocity = Vec3{0,0,0};
        }
    }

    void moveTo(float dt){
        Vec3 dir = (blackboard.targetPos - position);
        float dist = dir.length();
        if(dist>0.5f){
            dir = dir.normalized();
            velocity = dir * 2.5f;
        } else {
            velocity = Vec3{0,0,0};
            blackboard.hasTarget = false;
        }
    }

    void attackFire(float dt, FireSimulation& sim){
        // Use hose/tool
        if(water>0) {
            Vec3 grid = sim.getTerrain().worldToGrid(blackboard.firePos);
            sim.applyWater((int)grid.x,(int)grid.z, 2.0f*dt, 3.0f);
            water -= 2.0f*dt;
        }
        // Move slightly to get better angle
        Vec3 dir = (blackboard.firePos - position);
        if(dir.length() > 8.0f) {
            velocity = dir.normalized()*1.5f;
        } else if(dir.length() < 3.0f) {
            velocity = dir.normalized()*-1.0f;
        }
    }

    void protectArea(float dt, FireSimulation& sim){
        // Wet area around protectPos
        Vec3 grid = sim.getTerrain().worldToGrid(blackboard.protectPos);
        sim.applyWater((int)grid.x,(int)grid.z, 1.0f*dt, 5.0f);
        // Stay near protectPos
        Vec3 dir = (blackboard.protectPos - position);
        if(dir.length()>10.0f) velocity = dir.normalized()*2.0f;
    }

    void createFirebreak(float dt, FireSimulation& sim){
        Vec3 grid = sim.getTerrain().worldToGrid(blackboard.targetPos);
        // Simulate digging
        firebreakProgress += dt*0.3f;
        if(firebreakProgress>1.0f){
            sim.createFirebreak((int)grid.x,(int)grid.z,2);
            firebreakProgress=0;
            // Move to next segment perpendicular to wind?
            blackboard.targetPos.x += randRange(-2,2);
            blackboard.targetPos.z += randRange(-2,2);
        }
    }

    void retreat(float dt){
        // Move away from fire
        Vec3 dir = (position - blackboard.firePos);
        if(dir.length()<0.1f) dir = Vec3{randRange(-1,1),0,randRange(-1,1)};
        dir = dir.normalized();
        velocity = dir * 4.0f; // run
    }

    void searchArea(float dt, const std::vector<Vec3>& civilians){
        // Wander
        wanderTimer += dt;
        if(wanderTimer>3.0f){
            wanderTimer=0;
            Vec3 offset{randRange(-10,10),0,randRange(-10,10)};
            blackboard.targetPos = blackboard.targetPos + offset;
        }
        moveTo(dt);
        if(blackboard.civilianNearby) {
            state = AIState::RESCUE_CIVILIAN;
        }
    }

    void rescueCivilian(float dt){
        Vec3 dir = (blackboard.civilianPos - position);
        if(dir.length()>1.5f) {
            velocity = dir.normalized()*3.0f;
        } else {
            // Escort
            // Simulate carrying
            rescueProgress += dt;
            if(rescueProgress>5.0f){
                state = AIState::IDLE;
                rescueProgress=0;
            }
        }
    }

    void mopup(float dt, FireSimulation& sim){
        // Look for hotspots
        Vec3 grid = sim.getTerrain().worldToGrid(position);
        int gx=(int)grid.x, gy=(int)grid.z;
        bool foundHotspot=false;
        for(int dy=-5;dy<=5;dy++){
            for(int dx=-5;dx<=5;dx++){
                int nx=gx+dx, ny=gy+dy;
                if(nx<0||nx>=sim.width()||ny<0||ny>=sim.height()) continue;
                auto& cell = sim.getCells()[ny*sim.width()+nx];
                if(cell.state==FireState::MOPUP && cell.isHotspot){
                    sim.applyWater(nx,ny,1.5f*dt,2.0f);
                    foundHotspot=true;
                    blackboard.targetPos = cell.worldPos;
                }
            }
        }
        if(!foundHotspot){
            state = AIState::IDLE;
        }
    }

    int id;
    Vec3 position;
    Vec3 velocity{0,0,0};
    Vec3 homePos;
    AIState state;
    AIBlackboard blackboard;
    float health;
    float stamina;
    float water;
    float time=0;
    float firebreakProgress=0;
    float wanderTimer=0;
    float rescueProgress=0;
    RadioCommand lastCommand = RadioCommand::HOLD_POSITION;
    Vec3 commandPos;
    float commandTime=0;
    bool hasCommand=false;
    std::string radioMessage;
    float radioCooldown=0;

private:
    float randFloat(){ return static_cast<float>(rand())/RAND_MAX; }
    float randRange(float lo,float hi){ return lo+(hi-lo)*randFloat(); }
};

class TeamManager {
public:
    TeamManager() {}

    void addMember(Vec3 pos){
        members.emplace_back((int)members.size(), pos);
    }

    void update(float dt, const FireSimulation& sim, const WeatherState& weather, const std::vector<Vec3>& civilians){
        for(auto& m : members) m.update(dt, sim, weather, civilians);
    }

    void broadcastCommand(RadioCommand cmd, Vec3 pos){
        for(auto& m : members) m.giveCommand(cmd, pos);
        lastBroadcast = radioCommandToString(cmd);
        broadcastTime = 0;
    }

    void commandMember(int id, RadioCommand cmd, Vec3 pos){
        if(id>=0 && id<(int)members.size()) members[id].giveCommand(cmd,pos);
    }

    std::vector<FirefighterAI> members;
    std::string lastBroadcast;
    float broadcastTime=0;
};

} // namespace Fireline
