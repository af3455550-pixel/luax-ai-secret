#pragma once
#include "core/Math.hpp"
#include <random>

namespace Fireline {

struct WeatherState {
    Vec2 windDirection = {1,0}; // normalized 2D (x,z)
    float windSpeed = 5.0f;     // km/h
    float temperature = 30.0f;  // Celsius
    float humidity = 30.0f;     // %
    float precipitation = 0.0f; // mm/h
    bool isStorm = false;
    bool isHeatWave = false;
    float fogDensity = 0.0f;

    Vec3 getWindVector3() const {
        // Convert 2D wind to 3D, windDirection x = east, y = north -> z
        return Vec3{windDirection.x * windSpeed, 0, windDirection.y * windSpeed};
    }

    float getWindSpeedMS() const { return windSpeed / 3.6f; }
};

class WeatherSystem {
public:
    WeatherSystem() {
        rng.seed(42);
        current.temperature = 28.0f;
        current.humidity = 35.0f;
        current.windSpeed = 8.0f;
        current.windDirection = Vec2{0.7f,0.3f}.normalized();
    }

    void update(float dt) {
        timeAccum += dt;
        // Slowly vary wind
        windVariationTimer += dt;
        if(windVariationTimer > 10.0f) {
            windVariationTimer = 0;
            float angleDelta = randRange(-15,15) * DEG2RAD;
            float c = std::cos(angleDelta), s = std::sin(angleDelta);
            Vec2 d = current.windDirection;
            current.windDirection.x = d.x*c - d.y*s;
            current.windDirection.y = d.x*s + d.y*c;
            current.windDirection = current.windDirection.normalized();
            current.windSpeed = clamp(current.windSpeed + randRange(-2,2), 0, 80);
        }

        // Heat wave buildup
        if(current.isHeatWave) {
            current.temperature = lerp(current.temperature, 42.0f, dt*0.01f);
            current.humidity = lerp(current.humidity, 12.0f, dt*0.01f);
        }

        // Storm chance
        stormTimer += dt;
        if(stormTimer > 120.0f && !current.isStorm) {
            if(randFloat() < 0.05f) {
                triggerStorm();
            }
            stormTimer = 0;
        }

        if(current.isStorm) {
            stormDuration -= dt;
            if(stormDuration <= 0) {
                endStorm();
            } else {
                // Lightning can cause fires
                lightningTimer += dt;
                if(lightningTimer > randRange(5,15)) {
                    lightningTimer = 0;
                    if(randFloat() < 0.3f) {
                        pendingLightningStrike = true;
                        lightningPos = Vec2{randRange(-500,500), randRange(-500,500)};
                    }
                }
            }
        }

        // Day/night cycle affects temperature
        dayTime = fmod(dayTime + dt * daySpeed, 24.0f);
        float dayFactor = std::sin((dayTime/24.0f)*2*PI - PI/2); // -1 night, 1 day
        float baseTemp = 25.0f + dayFactor*8.0f;
        if(!current.isHeatWave) {
            current.temperature = lerp(current.temperature, baseTemp, dt*0.02f);
        }
    }

    void triggerHeatWave() {
        current.isHeatWave = true;
        current.temperature = 38.0f;
        current.humidity = 18.0f;
        current.windSpeed = std::max(current.windSpeed, 15.0f);
    }

    void triggerStorm() {
        current.isStorm = true;
        stormDuration = randRange(60,180);
        current.windSpeed = randRange(20,50);
        current.precipitation = randRange(5,20);
        current.humidity = randRange(70,95);
    }

    void endStorm() {
        current.isStorm = false;
        current.precipitation = 0;
        stormDuration = 0;
    }

    WeatherState current;
    float dayTime = 12.0f; // 0-24
    float daySpeed = 0.02f; // time acceleration

    bool pendingLightningStrike = false;
    Vec2 lightningPos;

private:
    std::mt19937 rng;
    float timeAccum = 0;
    float windVariationTimer = 0;
    float stormTimer = 0;
    float stormDuration = 0;
    float lightningTimer = 0;

    float randFloat() { std::uniform_real_distribution<float> d(0,1); return d(rng); }
    float randRange(float lo,float hi){ return lo + (hi-lo)*randFloat(); }
};

} // namespace Fireline
