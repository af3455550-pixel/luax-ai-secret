#pragma once
#include "core/Math.hpp"
#include <vector>

namespace Fireline {

enum class ParticleType {
    SMOKE,
    ASH,
    EMBER,
    SPARK,
    STEAM,
    DUST
};

struct Particle {
    Vec3 position;
    Vec3 velocity;
    Vec3 acceleration;
    Color color;
    float size = 1.0f;
    float life = 1.0f; // 0-1
    float maxLife = 1.0f;
    float rotation = 0;
    float rotationSpeed = 0;
    ParticleType type;
    bool active = true;
};

class ParticleSystem {
public:
    ParticleSystem(int maxParticles=10000) : maxParticles(maxParticles) {
        particles.reserve(maxParticles);
    }

    void emitSmoke(Vec3 pos, Vec3 dir, int count=5, float intensity=1.0f) {
        for(int i=0;i<count && (int)particles.size()<maxParticles; i++){
            Particle p;
            p.position = pos + Vec3{randRange(-1,1), randRange(0,2), randRange(-1,1)};
            p.velocity = dir*randRange(0.5f,2.0f) + Vec3{randRange(-0.5f,0.5f), randRange(0.5f,2.0f), randRange(-0.5f,0.5f)};
            p.velocity = p.velocity * intensity;
            p.acceleration = Vec3{0,0.2f,0};
            p.color = Color{0.2f+randRange(0,0.1f),0.2f+randRange(0,0.1f),0.2f+randRange(0,0.1f), 0.6f};
            p.size = randRange(1.0f,3.0f)*intensity;
            p.maxLife = randRange(3.0f,8.0f);
            p.life = p.maxLife;
            p.type = ParticleType::SMOKE;
            p.rotation = randRange(0,360);
            p.rotationSpeed = randRange(-30,30);
            particles.push_back(p);
        }
    }

    void emitEmbers(Vec3 pos, int count=10) {
        for(int i=0;i<count && (int)particles.size()<maxParticles; i++){
            Particle p;
            p.position = pos;
            p.velocity = Vec3{randRange(-3,3), randRange(2,6), randRange(-3,3)};
            p.acceleration = Vec3{0,-2.0f,0};
            p.color = Color{1.0f, randRange(0.3f,0.7f), 0.0f, 1.0f};
            p.size = randRange(0.1f,0.4f);
            p.maxLife = randRange(1.0f,3.0f);
            p.life = p.maxLife;
            p.type = ParticleType::EMBER;
            particles.push_back(p);
        }
    }

    void emitAsh(Vec3 pos, Vec3 wind, int count=20) {
        for(int i=0;i<count && (int)particles.size()<maxParticles; i++){
            Particle p;
            p.position = pos + Vec3{randRange(-10,10), randRange(5,20), randRange(-10,10)};
            p.velocity = wind*0.3f + Vec3{randRange(-0.5f,0.5f), randRange(-0.5f,0.0f), randRange(-0.5f,0.5f)};
            p.acceleration = Vec3{0,-0.05f,0};
            p.color = Color{0.15f,0.15f,0.15f,0.4f};
            p.size = randRange(0.05f,0.2f);
            p.maxLife = randRange(10.0f,20.0f);
            p.life = p.maxLife;
            p.type = ParticleType::ASH;
            p.rotation = randRange(0,360);
            p.rotationSpeed = randRange(-20,20);
            particles.push_back(p);
        }
    }

    void emitSteam(Vec3 pos, int count=8) {
        for(int i=0;i<count && (int)particles.size()<maxParticles; i++){
            Particle p;
            p.position = pos;
            p.velocity = Vec3{randRange(-0.5f,0.5f), randRange(1,3), randRange(-0.5f,0.5f)};
            p.acceleration = Vec3{0,0.1f,0};
            p.color = Color{0.9f,0.9f,0.95f,0.5f};
            p.size = randRange(0.5f,1.5f);
            p.maxLife = randRange(1.0f,2.5f);
            p.life = p.maxLife;
            p.type = ParticleType::STEAM;
            particles.push_back(p);
        }
    }

    void update(float dt, const Vec3& wind) {
        for(auto& p : particles) {
            if(!p.active) continue;
            p.life -= dt;
            if(p.life <= 0) { p.active=false; continue; }

            p.velocity = p.velocity + (p.acceleration + wind*0.05f)*dt;
            p.position = p.position + p.velocity*dt;
            p.rotation += p.rotationSpeed*dt;

            // Fade
            float lifeRatio = p.life / p.maxLife;
            if(p.type==ParticleType::SMOKE) {
                p.color.a = lifeRatio*0.6f;
                p.size += dt*0.5f;
            } else if(p.type==ParticleType::EMBER) {
                p.color.a = lifeRatio;
                if(lifeRatio<0.3f) p.color.r = 0.5f;
            } else if(p.type==ParticleType::ASH) {
                p.color.a = lifeRatio*0.4f;
            }
        }
        // Remove dead
        particles.erase(std::remove_if(particles.begin(), particles.end(), [](const Particle& p){ return !p.active; }), particles.end());
    }

    void clear(){ particles.clear(); }

    std::vector<Particle> particles;
    int maxParticles;

private:
    float randFloat(){ return static_cast<float>(rand())/RAND_MAX; }
    float randRange(float lo,float hi){ return lo + (hi-lo)*randFloat(); }
};

} // namespace Fireline
