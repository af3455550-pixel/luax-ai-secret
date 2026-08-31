#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <future>
#include <list>
#include <fstream>
#include "../Core/Log/Logger.hpp"
#include "../Core/Threading/TaskGraph.hpp"
#include "../Core/Events/EventBus.hpp"

namespace Apex::Assets {

    enum class AssetType {
        Unknown,
        Texture2D,
        Mesh,
        Material,
        Shader,
        AudioClip,
        Script,
        Level
    };

    enum class AssetState {
        Unloaded,
        Loading,
        Loaded,
        Failed
    };

    struct AssetMetadata {
        std::string guid;
        std::string path;
        AssetType type{AssetType::Unknown};
        size_t memorySizeBytes{0};
        uint32_t version{1};
    };

    class Asset {
    public:
        explicit Asset(AssetMetadata meta) : m_metadata(std::move(meta)), m_state(AssetState::Unloaded) {}
        virtual ~Asset() = default;

        const AssetMetadata& GetMetadata() const { return m_metadata; }
        AssetState GetState() const { return m_state; }
        void SetState(AssetState state) { m_state = state; }

        virtual bool LoadFromDisk() = 0;
        virtual void Unload() = 0;

    protected:
        AssetMetadata m_metadata;
        AssetState m_state;
    };

    // Example Concrete Assets
    class TextureAsset : public Asset {
    public:
        TextureAsset(AssetMetadata meta, uint32_t width = 1024, uint32_t height = 1024, uint32_t channels = 4)
            : Asset(std::move(meta)), m_width(width), m_height(height), m_channels(channels) {}

        bool LoadFromDisk() override {
            m_state = AssetState::Loading;
            // Simulated texture loading / decompression
            m_metadata.memorySizeBytes = m_width * m_height * m_channels;
            m_state = AssetState::Loaded;
            return true;
        }

        void Unload() override {
            m_state = AssetState::Unloaded;
            m_metadata.memorySizeBytes = 0;
        }

        uint32_t GetWidth() const { return m_width; }
        uint32_t GetHeight() const { return m_height; }

    private:
        uint32_t m_width{0};
        uint32_t m_height{0};
        uint32_t m_channels{4};
    };

    class MeshAsset : public Asset {
    public:
        explicit MeshAsset(AssetMetadata meta) : Asset(std::move(meta)), m_vertexCount(0), m_indexCount(0) {}

        bool LoadFromDisk() override {
            m_state = AssetState::Loading;
            // Simulated vertex/index buffer loading
            m_vertexCount = 14500;
            m_indexCount = 28000;
            m_metadata.memorySizeBytes = m_vertexCount * 48 + m_indexCount * 4;
            m_state = AssetState::Loaded;
            return true;
        }

        void Unload() override {
            m_state = AssetState::Unloaded;
            m_vertexCount = 0;
            m_indexCount = 0;
            m_metadata.memorySizeBytes = 0;
        }

        uint32_t GetVertexCount() const { return m_vertexCount; }
        uint32_t GetIndexCount() const { return m_indexCount; }

    private:
        uint32_t m_vertexCount{0};
        uint32_t m_indexCount{0};
    };

    class ShaderAsset : public Asset {
    public:
        explicit ShaderAsset(AssetMetadata meta, std::string sourceCode = "")
            : Asset(std::move(meta)), m_source(std::move(sourceCode)) {}

        bool LoadFromDisk() override {
            m_state = AssetState::Loaded;
            m_metadata.memorySizeBytes = m_source.size();
            return true;
        }

        void Unload() override {
            m_state = AssetState::Unloaded;
        }

        const std::string& GetSource() const { return m_source; }
        void SetSource(std::string src) { m_source = std::move(src); }

    private:
        std::string m_source;
    };

    // Asset Reload Event
    struct AssetReloadedEvent : public Events::IEvent {
        std::string assetPath;
        AssetType type;
        const char* GetName() const override { return "AssetReloadedEvent"; }
    };

    class AssetManager {
    public:
        static AssetManager& Get() {
            static AssetManager instance;
            return instance;
        }

        void Initialize(size_t memoryBudgetBytes = 512 * 1024 * 1024) {
            m_memoryBudget = memoryBudgetBytes;
            m_currentMemoryUsage = 0;
            LOG_INFO("AssetManager", "Initialized with budget " << (m_memoryBudget / (1024 * 1024)) << " MB.");
        }

        template <typename T, typename... Args>
        std::shared_ptr<T> LoadSync(const std::string& path, AssetType type, Args&&... args) {
            std::lock_guard<std::mutex> lock(m_mutex);

            auto it = m_assets.find(path);
            if (it != m_assets.end()) {
                TouchLRU(path);
                return std::dynamic_pointer_cast<T>(it->second);
            }

            AssetMetadata meta;
            meta.guid = path;
            meta.path = path;
            meta.type = type;

            auto asset = std::make_shared<T>(meta, std::forward<Args>(args)...);
            if (asset->LoadFromDisk()) {
                m_assets[path] = asset;
                m_lruList.push_front(path);
                m_lruMap[path] = m_lruList.begin();
                m_currentMemoryUsage += asset->GetMetadata().memorySizeBytes;

                CheckAndEvictBudget();
                LOG_INFO("AssetManager", "Loaded asset sync: " << path << " (" << (asset->GetMetadata().memorySizeBytes / 1024) << " KB)");
                return asset;
            }

            LOG_ERROR("AssetManager", "Failed to load asset sync: " << path);
            return nullptr;
        }

        template <typename T, typename... Args>
        std::future<std::shared_ptr<T>> LoadAsync(const std::string& path, AssetType type, Args&&... args) {
            return Threading::TaskGraph::Get().Enqueue([this, path, type, ...args = std::forward<Args>(args)]() mutable {
                return this->LoadSync<T>(path, type, std::forward<Args>(args)...);
            });
        }

        void HotReloadAsset(const std::string& path) {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto it = m_assets.find(path);
            if (it != m_assets.end()) {
                LOG_INFO("AssetManager", "Hot-reloading asset: " << path);
                it->second->Unload();
                it->second->LoadFromDisk();

                AssetReloadedEvent evt;
                evt.assetPath = path;
                evt.type = it->second->GetMetadata().type;
                Events::EventBus::Get().PublishAsync<AssetReloadedEvent>(evt);
            }
        }

        size_t GetCurrentMemoryUsage() const { return m_currentMemoryUsage; }
        size_t GetLoadedAssetCount() const { return m_assets.size(); }

    private:
        AssetManager() = default;

        void TouchLRU(const std::string& path) {
            auto it = m_lruMap.find(path);
            if (it != m_lruMap.end()) {
                m_lruList.erase(it->second);
                m_lruList.push_front(path);
                m_lruMap[path] = m_lruList.begin();
            }
        }

        void CheckAndEvictBudget() {
            while (m_currentMemoryUsage > m_memoryBudget && !m_lruList.empty()) {
                std::string oldest = m_lruList.back();
                auto it = m_assets.find(oldest);
                if (it != m_assets.end() && it->second.use_count() <= 1) { // Only held by manager
                    m_currentMemoryUsage -= it->second->GetMetadata().memorySizeBytes;
                    it->second->Unload();
                    m_assets.erase(it);
                    m_lruMap.erase(oldest);
                    m_lruList.pop_back();
                    LOG_INFO("AssetManager", "Evicted asset to preserve budget: " << oldest);
                } else {
                    break;
                }
            }
        }

        std::unordered_map<std::string, std::shared_ptr<Asset>> m_assets;
        std::list<std::string> m_lruList;
        std::unordered_map<std::string, std::list<std::string>::iterator> m_lruMap;
        size_t m_memoryBudget{512 * 1024 * 1024};
        size_t m_currentMemoryUsage{0};
        std::mutex m_mutex;
    };

} // namespace Apex::Assets
