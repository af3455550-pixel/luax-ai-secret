#pragma once
#include "core/Math.hpp"
#include <string>

namespace Fireline {

enum class CameraMode {
    FIRST_PERSON,
    THIRD_PERSON,
    VEHICLE_INTERIOR,
    VEHICLE_CHASE,
    CINEMATIC,
    TOP_DOWN,
    FREE
};

class CameraSystem {
public:
    CameraSystem() {
        position = Vec3{0,10,-20};
        target = Vec3{0,0,0};
        up = Vec3{0,1,0};
        fov = 70.0f;
        nearPlane = 0.1f;
        farPlane = 2000.0f;
        mode = CameraMode::THIRD_PERSON;
    }

    void update(float dt, Vec3 playerPos, Vec3 playerForward, Vec3 vehiclePos=Vec3{0,0,0}, bool inVehicle=false) {
        time += dt;

        // Shake
        if(shakeIntensity > 0) {
            shakeIntensity = std::max(0.0f, shakeIntensity - dt*shakeDecay);
            Vec3 shakeOffset{
                (randFloat()-0.5f)*2*shakeIntensity,
                (randFloat()-0.5f)*2*shakeIntensity,
                (randFloat()-0.5f)*2*shakeIntensity
            };
            position = position + shakeOffset;
        }

        // Mode-specific update
        switch(mode){
            case CameraMode::FIRST_PERSON:
                {
                    position = playerPos + Vec3{0,1.7f,0};
                    target = position + playerForward*10.0f;
                    // Subtle breathing shake
                    float breath = std::sin(time*1.5f)*0.02f;
                    position.y += breath;
                }
                break;
            case CameraMode::THIRD_PERSON:
                {
                    Vec3 desiredPos = playerPos - playerForward*5.0f + Vec3{0,2.5f,0};
                    position = Vec3::lerp(position, desiredPos, dt*5.0f);
                    target = Vec3::lerp(target, playerPos + Vec3{0,1.5f,0}, dt*8.0f);
                }
                break;
            case CameraMode::VEHICLE_INTERIOR:
                {
                    position = vehiclePos + Vec3{0,1.2f,0.5f};
                    target = vehiclePos + Vec3{0,1.2f,0} + Vec3{std::sin(yaw),0,std::cos(yaw)}*20.0f;
                }
                break;
            case CameraMode::VEHICLE_CHASE:
                {
                    Vec3 forward{std::sin(yaw),0,std::cos(yaw)};
                    Vec3 desiredPos = vehiclePos - forward*12.0f + Vec3{0,4.0f,0};
                    position = Vec3::lerp(position, desiredPos, dt*4.0f);
                    target = Vec3::lerp(target, vehiclePos + Vec3{0,1.5f,0}, dt*6.0f);
                }
                break;
            case CameraMode::CINEMATIC:
                {
                    // Slow orbital + dolly
                    float angle = time*0.1f + cinematicAngle;
                    float radius = 30.0f + std::sin(time*0.05f)*5.0f;
                    float height = 12.0f + std::sin(time*0.07f)*3.0f;
                    Vec3 center = cinematicTarget;
                    position.x = center.x + std::cos(angle)*radius;
                    position.z = center.z + std::sin(angle)*radius;
                    position.y = center.y + height;
                    target = center;
                    // Add slow zoom
                    fov = 60.0f + std::sin(time*0.03f)*10.0f;
                }
                break;
            case CameraMode::TOP_DOWN:
                {
                    position = playerPos + Vec3{0,80,0};
                    target = playerPos;
                    up = Vec3{0,0,-1};
                }
                break;
            case CameraMode::FREE:
                // Manual control
                break;
        }

        // Depth of Field simulation - focus distance
        focusDistance = (target - position).length();
        // Motion blur - based on velocity
        Vec3 vel = position - lastPosition;
        motionBlurIntensity = clamp(vel.length()*0.1f, 0.0f, 1.0f);
        lastPosition = position;

        // Update view and projection matrices
        viewMatrix = Mat4::LookAt(position, target, up);
        projectionMatrix = Mat4::Perspective(fov, aspectRatio, nearPlane, farPlane);
    }

    void setMode(CameraMode m){ mode=m; }
    void addShake(float intensity, float decay=3.0f){ shakeIntensity = std::max(shakeIntensity, intensity); shakeDecay=decay; }
    void setCinematicTarget(Vec3 t, float angle=0){ cinematicTarget=t; cinematicAngle=angle; }

    Mat4 getViewMatrix() const { return viewMatrix; }
    Mat4 getProjectionMatrix() const { return projectionMatrix; }
    Mat4 getViewProjection() const { return projectionMatrix * viewMatrix; }

    Vec3 position;
    Vec3 target;
    Vec3 up;
    Vec3 lastPosition;
    float fov;
    float aspectRatio = 16.0f/9.0f;
    float nearPlane, farPlane;
    float yaw = 0, pitch = 0;
    CameraMode mode;

    // Effects
    float shakeIntensity = 0;
    float shakeDecay = 2.0f;
    float focusDistance = 20.0f;
    float dofBlur = 0.02f;
    float motionBlurIntensity = 0;
    float time = 0;

    // Cinematic
    Vec3 cinematicTarget{0,0,0};
    float cinematicAngle = 0;

    Mat4 viewMatrix;
    Mat4 projectionMatrix;

private:
    float randFloat(){ return static_cast<float>(rand())/RAND_MAX; }
    float clamp(float v,float lo,float hi){ return std::max(lo,std::min(hi,v)); }
};

} // namespace Fireline
