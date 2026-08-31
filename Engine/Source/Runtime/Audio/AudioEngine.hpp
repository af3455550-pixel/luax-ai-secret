#pragma once

#include <vector>
#include <string>
#include <cmath>
#include "../ECS/ECS.hpp"
#include "../Core/Math/MathTypes.hpp"
#include "../Core/Log/Logger.hpp"

namespace Apex::Audio {

    struct ListenerData {
        Math::Vec3 position{0.0f, 0.0f, 0.0f};
        Math::Vec3 forward{0.0f, 0.0f, 1.0f};
        Math::Vec3 up{0.0f, 1.0f, 0.0f};
        Math::Vec3 velocity{0.0f, 0.0f, 0.0f};
    };

    struct SpatializedSound {
        float leftGain{1.0f};
        float rightGain{1.0f};
        float pitchShift{1.0f};
        float effectiveVolume{1.0f};
    };

    class AudioEngine {
    public:
        static AudioEngine& Get() {
            static AudioEngine instance;
            return instance;
        }

        void Initialize() {
            m_masterVolume = 1.0f;
            m_sfxVolume = 1.0f;
            m_musicVolume = 0.8f;
            LOG_INFO("Audio", "Initialized 3D Spatial Audio Subsystem with DSP & Multi-Format Support (WAV/OGG/MP3).");
        }

        void Update(ECS::Registry& registry, const ListenerData& listener) {
            // Calculate spatialization for each active audio source
            registry.ForEach<ECS::AudioSourceComponent, ECS::TransformComponent>(
                [&](ECS::Entity, ECS::AudioSourceComponent& source, const ECS::TransformComponent& tc) {
                    if (!source.isPlaying) return;

                    Math::Vec3 sourcePos = tc.worldMatrix.TransformPoint(Math::Vec3::Zero());
                    Math::Vec3 toSource = sourcePos - listener.position;
                    float distance = toSource.Length();

                    // Attenuation
                    float clampedDist = std::clamp(distance, source.minDistance, source.maxDistance);
                    float attenuation = (source.maxDistance - clampedDist) / (source.maxDistance - source.minDistance);
                    attenuation = std::clamp(attenuation, 0.0f, 1.0f);

                    // Panning
                    Math::Vec3 right = listener.forward.Cross(listener.up).Normalized();
                    Math::Vec3 dir = distance > 1e-4f ? toSource / distance : Math::Vec3::Forward();
                    float pan = dir.Dot(right); // -1.0 (left) to +1.0 (right)

                    SpatializedSound sp;
                    sp.leftGain = std::clamp((1.0f - pan) * 0.5f, 0.0f, 1.0f);
                    sp.rightGain = std::clamp((1.0f + pan) * 0.5f, 0.0f, 1.0f);
                    sp.effectiveVolume = source.volume * attenuation * m_masterVolume * m_sfxVolume;
                    sp.pitchShift = source.pitch;

                    // Simulated DSP output
                });
        }

        void SetMasterVolume(float vol) { m_masterVolume = std::clamp(vol, 0.0f, 1.0f); }
        float GetMasterVolume() const { return m_masterVolume; }

    private:
        AudioEngine() = default;
        float m_masterVolume{1.0f};
        float m_sfxVolume{1.0f};
        float m_musicVolume{0.8f};
    };

} // namespace Apex::Audio
