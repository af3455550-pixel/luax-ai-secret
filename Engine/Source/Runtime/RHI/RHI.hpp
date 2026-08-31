#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include "../Core/Math/MathTypes.hpp"
#include "../Core/Log/Logger.hpp"

namespace Apex::RHI {

    enum class GraphicsBackend {
        Vulkan,
        OpenGL,
        DirectX12,
        Metal
    };

    enum class BufferType {
        VertexBuffer,
        IndexBuffer,
        UniformBuffer,
        StorageBuffer
    };

    enum class TextureFormat {
        RGBA8_UNORM,
        RGBA16_FLOAT,
        RGBA32_FLOAT,
        R32_FLOAT,
        D24_S8,
        D32_FLOAT
    };

    struct TextureDescriptor {
        uint32_t width{1};
        uint32_t height{1};
        uint32_t depth{1};
        uint32_t mipLevels{1};
        TextureFormat format{TextureFormat::RGBA8_UNORM};
        bool isRenderTarget{false};
        bool isDepthStencil{false};
        std::string debugName{"Texture"};
    };

    class RHITexture {
    public:
        explicit RHITexture(TextureDescriptor desc) : m_desc(desc), m_handle(1) {}
        virtual ~RHITexture() = default;

        const TextureDescriptor& GetDesc() const { return m_desc; }
        uint64_t GetHandle() const { return m_handle; }

    protected:
        TextureDescriptor m_desc;
        uint64_t m_handle{0};
    };

    class RHIBuffer {
    public:
        RHIBuffer(BufferType type, size_t sizeBytes, const void* initialData = nullptr)
            : m_type(type), m_size(sizeBytes) {
            (void)initialData;
        }
        virtual ~RHIBuffer() = default;

        BufferType GetType() const { return m_type; }
        size_t GetSize() const { return m_size; }

    protected:
        BufferType m_type;
        size_t m_size;
    };

    enum class ShaderStage {
        Vertex,
        Pixel,
        Compute
    };

    class RHIShader {
    public:
        RHIShader(ShaderStage stage, std::string name, std::string code)
            : m_stage(stage), m_name(std::move(name)), m_code(std::move(code)) {}
        virtual ~RHIShader() = default;

        ShaderStage GetStage() const { return m_stage; }
        const std::string& GetName() const { return m_name; }

    protected:
        ShaderStage m_stage;
        std::string m_name;
        std::string m_code;
    };

    struct PipelineStateDescriptor {
        std::shared_ptr<RHIShader> vertexShader;
        std::shared_ptr<RHIShader> pixelShader;
        bool depthTestEnable{true};
        bool depthWriteEnable{true};
        bool wireframe{false};
        bool blendEnable{false};
    };

    class RHIPipelineState {
    public:
        explicit RHIPipelineState(PipelineStateDescriptor desc) : m_desc(std::move(desc)) {}
        virtual ~RHIPipelineState() = default;
        const PipelineStateDescriptor& GetDesc() const { return m_desc; }

    private:
        PipelineStateDescriptor m_desc;
    };

    class RHICommandBuffer {
    public:
        virtual ~RHICommandBuffer() = default;

        void Begin() {
            m_drawCallCount = 0;
            m_triangleCount = 0;
        }

        void End() {}

        void SetPipeline(std::shared_ptr<RHIPipelineState> pipeline) {
            m_currentPipeline = pipeline;
        }

        void SetVertexBuffer(std::shared_ptr<RHIBuffer> vb) {
            m_currentVB = vb;
        }

        void SetIndexBuffer(std::shared_ptr<RHIBuffer> ib) {
            m_currentIB = ib;
        }

        void SetTexture(uint32_t slot, std::shared_ptr<RHITexture> texture) {
            (void)slot; (void)texture;
        }

        void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1) {
            m_drawCallCount += instanceCount;
            m_triangleCount += (indexCount / 3) * instanceCount;
        }

        void Draw(uint32_t vertexCount, uint32_t instanceCount = 1) {
            m_drawCallCount += instanceCount;
            m_triangleCount += (vertexCount / 3) * instanceCount;
        }

        uint32_t GetDrawCallCount() const { return m_drawCallCount; }
        uint32_t GetTriangleCount() const { return m_triangleCount; }

    private:
        std::shared_ptr<RHIPipelineState> m_currentPipeline;
        std::shared_ptr<RHIBuffer> m_currentVB;
        std::shared_ptr<RHIBuffer> m_currentIB;
        uint32_t m_drawCallCount{0};
        uint32_t m_triangleCount{0};
    };

    class RHIDevice {
    public:
        static RHIDevice& Get() {
            static RHIDevice instance;
            return instance;
        }

        void Initialize(GraphicsBackend backend = GraphicsBackend::Vulkan) {
            m_backend = backend;
            m_cmdBuffer = std::make_shared<RHICommandBuffer>();

            const char* name = "Vulkan";
            if (backend == GraphicsBackend::OpenGL) name = "OpenGL 4.6 Core";
            if (backend == GraphicsBackend::DirectX12) name = "DirectX 12 Ultimate";

            LOG_INFO("RHI", "Initialized Graphics Hardware Device: " << name);
        }

        std::shared_ptr<RHITexture> CreateTexture(const TextureDescriptor& desc) {
            return std::make_shared<RHITexture>(desc);
        }

        std::shared_ptr<RHIBuffer> CreateBuffer(BufferType type, size_t size, const void* data = nullptr) {
            return std::make_shared<RHIBuffer>(type, size, data);
        }

        std::shared_ptr<RHIShader> CreateShader(ShaderStage stage, const std::string& name, const std::string& code) {
            return std::make_shared<RHIShader>(stage, name, code);
        }

        std::shared_ptr<RHIPipelineState> CreatePipelineState(const PipelineStateDescriptor& desc) {
            return std::make_shared<RHIPipelineState>(desc);
        }

        std::shared_ptr<RHICommandBuffer> GetCommandBuffer() { return m_cmdBuffer; }
        GraphicsBackend GetBackend() const { return m_backend; }

    private:
        RHIDevice() = default;
        GraphicsBackend m_backend{GraphicsBackend::Vulkan};
        std::shared_ptr<RHICommandBuffer> m_cmdBuffer;
    };

} // namespace Apex::RHI
