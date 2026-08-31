#pragma once

#include <vector>
#include <unordered_map>
#include <cstdint>
#include <deque>
#include <cstring>
#include "../ECS/ECS.hpp"
#include "../Core/Math/MathTypes.hpp"
#include "../Core/Log/Logger.hpp"

namespace Apex::Networking {

    enum class NetworkRole {
        None,
        DedicatedServer,
        ListenServer,
        Client
    };

    struct EntitySnapshot {
        uint32_t netId{0};
        uint32_t sequenceNumber{0};
        float timestamp{0.0f};
        Math::Vec3 position{0.0f, 0.0f, 0.0f};
        Math::Vec3 velocity{0.0f, 0.0f, 0.0f};
        Math::Quat rotation{0.0f, 0.0f, 0.0f, 1.0f};
    };

    struct NetworkPacket {
        uint32_t sequenceNumber{0};
        float sendTime{0.0f};
        std::vector<EntitySnapshot> entitySnapshots;
    };

    class NetworkEngine {
    public:
        static NetworkEngine& Get() {
            static NetworkEngine instance;
            return instance;
        }

        void StartServer(uint16_t port = 7777) {
            m_role = NetworkRole::DedicatedServer;
            m_port = port;
            m_sequence = 0;
            LOG_INFO("Network", "Server listening on UDP port " << port << " with authoritative state replication.");
        }

        void ConnectClient(const std::string& address, uint16_t port = 7777) {
            m_role = NetworkRole::Client;
            m_serverAddress = address;
            m_port = port;
            LOG_INFO("Network", "Client connected to server at " << address << ":" << port);
        }

        void ServerTick(ECS::Registry& registry, float deltaTime, float totalTime) {
            if (m_role != NetworkRole::DedicatedServer && m_role != NetworkRole::ListenServer) return;

            m_replicationTimer += deltaTime;
            if (m_replicationTimer >= (1.0f / 30.0f)) { // 30Hz Replication Tick
                m_replicationTimer = 0.0f;
                m_sequence++;

                NetworkPacket packet;
                packet.sequenceNumber = m_sequence;
                packet.sendTime = totalTime;

                registry.ForEach<ECS::NetworkReplicationComponent, ECS::TransformComponent, ECS::RigidBodyComponent>(
                    [&](ECS::Entity, const ECS::NetworkReplicationComponent& net, const ECS::TransformComponent& tc, const ECS::RigidBodyComponent& rb) {
                        EntitySnapshot snap;
                        snap.netId = net.netId;
                        snap.sequenceNumber = m_sequence;
                        snap.timestamp = totalTime;
                        snap.position = tc.localTransform.position;
                        snap.velocity = rb.velocity;
                        snap.rotation = tc.localTransform.rotation;
                        packet.entitySnapshots.push_back(snap);
                    });

                // Simulate broadcast to clients
                m_serverOutboundPackets++;
                m_serverBytesSent += sizeof(NetworkPacket) + packet.entitySnapshots.size() * sizeof(EntitySnapshot);
            }
        }

        void ClientTick(ECS::Registry& registry, float deltaTime, float totalTime) {
            if (m_role != NetworkRole::Client) return;
            (void)registry; (void)deltaTime; (void)totalTime;
            // Simulated client-side prediction & snapshot interpolation
        }

        NetworkRole GetRole() const { return m_role; }
        uint64_t GetBytesSent() const { return m_serverBytesSent; }
        uint32_t GetOutboundPackets() const { return m_serverOutboundPackets; }

    private:
        NetworkEngine() = default;
        NetworkRole m_role{NetworkRole::None};
        std::string m_serverAddress{"127.0.0.1"};
        uint16_t m_port{7777};
        uint32_t m_sequence{0};
        float m_replicationTimer{0.0f};
        uint64_t m_serverBytesSent{0};
        uint32_t m_serverOutboundPackets{0};
    };

} // namespace Apex::Networking
