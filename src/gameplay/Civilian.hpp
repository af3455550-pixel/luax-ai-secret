#pragma once
#include "core/Math.hpp"
#include "simulation/FireSimulation.hpp"
#include <vector>
#include <string>

namespace Fireline {

enum class CivilianType {
    ADULT,
    CHILD,
    ELDERLY,
    FIREFIGHTER,
    FARMER,
    DOG,
    CAT,
    DEER,
    HORSE
};

struct Civilian {
    int id;
    CivilianType type;
    Vec3 position;
    Vec3 velocity;
    float health = 100;
    float panic = 0; // 0-1
    bool isEvacuated = false;
    bool isInjured = false;
    bool needsRescue = false;
    Vec3 homePos;
    Vec3 evacuationPoint;
    std::string name;

    void update(float dt, const FireSimulation& sim, const Vec3& threatPos) {
        // Panic increases near fire
        Vec3 grid = sim.getTerrain().worldToGrid(position);
        int gx=(int)grid.x, gy=(int)grid.z;
        float nearbyFire = 0;
        for(int dy=-5; dy<=5; ++dy){
            for(int dx=-5; dx<=5; ++dx){
                int nx=gx+dx, ny=gy+dy;
                if(nx<0||nx>=sim.width()||ny<0||ny>=sim.height()) continue;
                auto& cell = sim.getCell(nx,ny);
                if(cell.state==FireState::COMBUSTION) {
                    float dist = std::sqrt(float(dx*dx+dy*dy));
                    nearbyFire += cell.intensity/(dist+1);
                }
            }
        }
        panic = clamp(nearbyFire/200.0f, 0.0f, 1.0f);

        // Flee from fire
        if(panic>0.3f) {
            Vec3 dir = (position - threatPos);
            if(dir.length()<0.1f) dir = Vec3{randFloat()-0.5f,0,randFloat()-0.5f};
            dir = dir.normalized();
            velocity = dir * (2.0f + panic*3.0f);
            // Add randomness when panicked
            velocity.x += (randFloat()-0.5f)*panic*2;
            velocity.z += (randFloat()-0.5f)*panic*2;
        } else {
            // Wander or go to evacuation
            if(isEvacuated) {
                Vec3 dir = (evacuationPoint - position);
                if(dir.length()>1.0f) velocity = dir.normalized()*1.5f;
                else velocity = Vec3{0,0,0};
            } else {
                velocity = velocity * 0.9f;
            }
        }

        position = position + velocity*dt;
        position.y = sim.getTerrain().getHeightWorld(position.x, position.z) + 1.0f;

        // Health damage from smoke/heat
        if(nearbyFire>50) {
            health -= dt*nearbyFire*0.01f;
            if(health<30) isInjured=true;
            if(health<=0) needsRescue=true;
        }
    }

private:
    float clamp(float v,float lo,float hi){ return std::max(lo,std::min(hi,v)); }
    float randFloat(){ return static_cast<float>(rand())/RAND_MAX; }
};

class CivilianManager {
public:
    void spawnCivilians(Vec3 villagePos, int count=5) {
        for(int i=0;i<count;i++){
            Civilian c;
            c.id = (int)civilians.size();
            c.type = (rand()%5==0)?CivilianType::CHILD: (rand()%10==0)?CivilianType::ELDERLY : CivilianType::ADULT;
            c.position = villagePos + Vec3{randRange(-20,20),0,randRange(-20,20)};
            c.homePos = c.position;
            c.evacuationPoint = villagePos + Vec3{100,0,0}; // safe zone
            c.name = "Civil " + std::to_string(c.id);
            civilians.push_back(c);
        }
    }

    void spawnAnimals(Vec3 forestPos, int count=3) {
        for(int i=0;i<count;i++){
            Civilian c;
            c.id = (int)civilians.size();
            c.type = (CivilianType)( (int)CivilianType::DOG + rand()%4 );
            c.position = forestPos + Vec3{randRange(-30,30),0,randRange(-30,30)};
            c.homePos = c.position;
            c.name = "Animal " + std::to_string(c.id);
            civilians.push_back(c);
        }
    }

    void update(float dt, const FireSimulation& sim, const Vec3& firePos) {
        for(auto& c : civilians) c.update(dt, sim, firePos);
        // Remove evacuated that reached safe point
        civilians.erase(std::remove_if(civilians.begin(), civilians.end(), [](const Civilian& c){
            return c.isEvacuated && (c.position - c.evacuationPoint).length() < 2.0f;
        }), civilians.end());
    }

    int countAtRisk() const {
        int cnt=0;
        for(auto& c: civilians) if(!c.isEvacuated && c.panic>0.5f) cnt++;
        return cnt;
    }

    std::vector<Civilian> civilians;

private:
    float randFloat(){ return static_cast<float>(rand())/RAND_MAX; }
    float randRange(float lo,float hi){ return lo + (hi-lo)*randFloat(); }
};

} // namespace Fireline
