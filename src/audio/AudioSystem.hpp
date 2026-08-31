#pragma once
#include "core/Math.hpp"
#include <string>
#include <vector>
#include <map>

namespace Fireline {

enum class SoundType {
    FIRE_CRACKLE,
    TREE_FALL,
    EXPLOSION,
    WIND,
    RADIO_CHATTER,
    SIREN,
    FIREFIGHTER_VOICE,
    BREATHING_MASK,
    WATER_SPRAY,
    ENGINE_IDLE,
    ENGINE_REV,
    HELICOPTER_ROTOR,
    FOOTSTEP,
    CHAINSAW,
    AXE_HIT,
    AMBIENT_FOREST,
    MUSIC_LOW,
    MUSIC_HIGH,
    MUSIC_EXTREME
};

struct AudioSource {
    Vec3 position;
    float volume = 1.0f;
    float pitch = 1.0f;
    float minDistance = 5.0f;
    float maxDistance = 100.0f;
    bool loop = false;
    bool is3D = true;
    SoundType type;
    float playTime = 0;
    bool active = true;
};

class AudioSystem {
public:
    AudioSystem() {
        masterVolume = 0.8f;
        musicIntensity = 0.0f;
    }

    void update(float dt, Vec3 listenerPos, Vec3 listenerForward, float fireIntensity) {
        time += dt;
        // Update music intensity based on fire
        float targetIntensity = clamp(fireIntensity/1000.0f, 0.0f, 1.0f);
        musicIntensity = lerp(musicIntensity, targetIntensity, dt*0.5f);

        // Update sources
        for(auto& src : sources) {
            if(!src.active) continue;
            src.playTime += dt;
            // 3D attenuation
            if(src.is3D) {
                float dist = (src.position - listenerPos).length();
                float atten = 1.0f - clamp((dist - src.minDistance)/(src.maxDistance - src.minDistance), 0.0f, 1.0f);
                src.volume = atten;
            }
            // Auto-remove short sounds
            if(!src.loop && src.playTime > getSoundDuration(src.type)) {
                src.active = false;
            }
        }
        // Remove inactive
        sources.erase(std::remove_if(sources.begin(), sources.end(), [](const AudioSource& s){ return !s.active; }), sources.end());

        // Ambient wind based on weather
        // ...
    }

    void playSound(SoundType type, Vec3 pos, float volume=1.0f, bool loop=false) {
        AudioSource src;
        src.type = type;
        src.position = pos;
        src.volume = volume * masterVolume;
        src.loop = loop;
        src.is3D = true;
        src.minDistance = getMinDistance(type);
        src.maxDistance = getMaxDistance(type);
        sources.push_back(src);

        // Console log for debug
        if(type==SoundType::RADIO_CHATTER || type==SoundType::FIREFIGHTER_VOICE) {
            lastRadio = soundTypeToString(type) + " at " + std::to_string((int)pos.x) + "," + std::to_string((int)pos.z);
        }
    }

    void playRadio(const std::string& message, Vec3 pos) {
        playSound(SoundType::RADIO_CHATTER, pos, 0.9f);
        lastRadio = message;
        radioTimer = 3.0f;
    }

    float getSoundDuration(SoundType t) const {
        switch(t){
            case SoundType::FIRE_CRACKLE: return 9999; // loop
            case SoundType::TREE_FALL: return 2.5f;
            case SoundType::EXPLOSION: return 3.0f;
            case SoundType::WIND: return 9999;
            case SoundType::RADIO_CHATTER: return 2.0f;
            case SoundType::SIREN: return 9999;
            case SoundType::FIREFIGHTER_VOICE: return 1.5f;
            case SoundType::BREATHING_MASK: return 9999;
            case SoundType::WATER_SPRAY: return 9999;
            case SoundType::ENGINE_IDLE: return 9999;
            case SoundType::HELICOPTER_ROTOR: return 9999;
            default: return 1.0f;
        }
    }

    float getMinDistance(SoundType t) const {
        switch(t){
            case SoundType::FIRE_CRACKLE: return 10.0f;
            case SoundType::EXPLOSION: return 20.0f;
            case SoundType::SIREN: return 5.0f;
            case SoundType::HELICOPTER_ROTOR: return 30.0f;
            default: return 3.0f;
        }
    }
    float getMaxDistance(SoundType t) const {
        switch(t){
            case SoundType::FIRE_CRACKLE: return 150.0f;
            case SoundType::EXPLOSION: return 500.0f;
            case SoundType::SIREN: return 300.0f;
            case SoundType::HELICOPTER_ROTOR: return 400.0f;
            case SoundType::WIND: return 50.0f;
            default: return 80.0f;
        }
    }

    std::string soundTypeToString(SoundType t) const {
        switch(t){
            case SoundType::FIRE_CRACKLE: return "Fogo a crepitar";
            case SoundType::TREE_FALL: return "Arvore a partir";
            case SoundType::EXPLOSION: return "Explosao";
            case SoundType::WIND: return "Vento";
            case SoundType::RADIO_CHATTER: return "Radio";
            case SoundType::SIREN: return "Sirene";
            case SoundType::FIREFIGHTER_VOICE: return "Bombeiro a comunicar";
            case SoundType::BREATHING_MASK: return "Respiracao na mascara";
            case SoundType::WATER_SPRAY: return "Agua";
            case SoundType::ENGINE_IDLE: return "Motor";
            case SoundType::HELICOPTER_ROTOR: return "Helicoptero";
            default: return "Som";
        }
    }

    std::vector<AudioSource> sources;
    float masterVolume;
    float musicIntensity;
    float time=0;
    std::string lastRadio;
    float radioTimer=0;

private:
    float clamp(float v,float lo,float hi){ return std::max(lo,std::min(hi,v)); }
    float lerp(float a,float b,float t){ return a + (b-a)*t; }
};

} // namespace Fireline
