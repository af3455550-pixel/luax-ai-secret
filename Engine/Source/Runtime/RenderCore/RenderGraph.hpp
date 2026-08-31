#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include "../RHI/RHI.hpp"
#include "../Core/Log/Logger.hpp"

namespace Apex::RenderCore {

    class RenderGraphPass {
    public:
        using ExecuteFn = std::function<void(RHI::RHICommandBuffer&)>;

        RenderGraphPass(std::string name, ExecuteFn exec)
            : m_name(std::move(name)), m_execute(std::move(exec)) {}

        const std::string& GetName() const { return m_name; }
        void Execute(RHI::RHICommandBuffer& cmd) {
            if (m_execute) m_execute(cmd);
        }

    private:
        std::string m_name;
        ExecuteFn m_execute;
    };

    class RenderGraph {
    public:
        RenderGraph() = default;

        void AddPass(const std::string& name, RenderGraphPass::ExecuteFn exec) {
            m_passes.emplace_back(name, exec);
        }

        void Compile() {
            // Topological sort / resource barrier calculation
        }

        void Execute(RHI::RHICommandBuffer& cmd) {
            cmd.Begin();
            for (auto& pass : m_passes) {
                pass.Execute(cmd);
            }
            cmd.End();
            m_passes.clear();
        }

    private:
        std::vector<RenderGraphPass> m_passes;
    };

} // namespace Apex::RenderCore
