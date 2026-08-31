#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <new>
#include <atomic>
#include <vector>
#include <cassert>
#include <cstring>
#include "../Log/Logger.hpp"

namespace Apex::Memory {

    struct MemoryStats {
        std::atomic<size_t> totalAllocated{0};
        std::atomic<size_t> totalFreed{0};
        std::atomic<size_t> currentUsage{0};
        std::atomic<size_t> allocationCount{0};

        void RecordAlloc(size_t size) {
            totalAllocated += size;
            currentUsage += size;
            allocationCount++;
        }

        void RecordFree(size_t size) {
            totalFreed += size;
            currentUsage -= size;
        }
    };

    inline MemoryStats& GetGlobalMemoryStats() {
        static MemoryStats stats;
        return stats;
    }

    inline void* AlignPointer(void* ptr, size_t alignment) {
        uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
        uintptr_t aligned = (addr + (alignment - 1)) & ~(alignment - 1);
        return reinterpret_cast<void*>(aligned);
    }

    // ==========================================
    // Arena (Linear) Allocator
    // ==========================================
    class ArenaAllocator {
    public:
        explicit ArenaAllocator(size_t capacityBytes)
            : m_capacity(capacityBytes), m_offset(0) {
            m_buffer = static_cast<uint8_t*>(std::malloc(capacityBytes));
            GetGlobalMemoryStats().RecordAlloc(capacityBytes);
        }

        ~ArenaAllocator() {
            if (m_buffer) {
                GetGlobalMemoryStats().RecordFree(m_capacity);
                std::free(m_buffer);
                m_buffer = nullptr;
            }
        }

        ArenaAllocator(const ArenaAllocator&) = delete;
        ArenaAllocator& operator=(const ArenaAllocator&) = delete;

        void* Allocate(size_t size, size_t alignment = alignof(std::max_align_t)) {
            uint8_t* currentPtr = m_buffer + m_offset;
            uint8_t* alignedPtr = static_cast<uint8_t*>(AlignPointer(currentPtr, alignment));
            size_t adjustment = alignedPtr - currentPtr;

            if (m_offset + adjustment + size > m_capacity) {
                LOG_ERROR("Memory", "ArenaAllocator out of memory! Requested: " << size << " bytes, Available: " << (m_capacity - m_offset));
                return nullptr;
            }

            m_offset += adjustment + size;
            return alignedPtr;
        }

        template <typename T, typename... Args>
        T* New(Args&&... args) {
            void* mem = Allocate(sizeof(T), alignof(T));
            if (!mem) return nullptr;
            return new (mem) T(std::forward<Args>(args)...);
        }

        void Reset() {
            m_offset = 0;
        }

        size_t GetUsedBytes() const { return m_offset; }
        size_t GetCapacity() const { return m_capacity; }

    private:
        uint8_t* m_buffer{nullptr};
        size_t m_capacity{0};
        size_t m_offset{0};
    };

    // ==========================================
    // Pool Allocator (Fixed Size Blocks)
    // ==========================================
    template <typename T, size_t BlockCount = 1024>
    class PoolAllocator {
    public:
        PoolAllocator() : m_freeHead(nullptr), m_allocatedCount(0) {
            m_blockSize = sizeof(T) > sizeof(Node) ? sizeof(T) : sizeof(Node);
            m_buffer = static_cast<uint8_t*>(std::malloc(m_blockSize * BlockCount));
            GetGlobalMemoryStats().RecordAlloc(m_blockSize * BlockCount);
            Reset();
        }

        ~PoolAllocator() {
            if (m_buffer) {
                GetGlobalMemoryStats().RecordFree(m_blockSize * BlockCount);
                std::free(m_buffer);
                m_buffer = nullptr;
            }
        }

        void Reset() {
            m_freeHead = nullptr;
            m_allocatedCount = 0;
            for (size_t i = 0; i < BlockCount; ++i) {
                Node* node = reinterpret_cast<Node*>(m_buffer + i * m_blockSize);
                node->next = m_freeHead;
                m_freeHead = node;
            }
        }

        template <typename... Args>
        T* Allocate(Args&&... args) {
            if (!m_freeHead) {
                LOG_ERROR("Memory", "PoolAllocator full! BlockCount: " << BlockCount);
                return nullptr;
            }
            Node* node = m_freeHead;
            m_freeHead = m_freeHead->next;
            m_allocatedCount++;
            return new (node) T(std::forward<Args>(args)...);
        }

        void Deallocate(T* ptr) {
            if (!ptr) return;
            ptr->~T();
            Node* node = reinterpret_cast<Node*>(ptr);
            node->next = m_freeHead;
            m_freeHead = node;
            m_allocatedCount--;
        }

        size_t GetActiveCount() const { return m_allocatedCount; }
        size_t GetCapacity() const { return BlockCount; }

    private:
        struct Node {
            Node* next;
        };

        uint8_t* m_buffer{nullptr};
        size_t m_blockSize{sizeof(T)};
        Node* m_freeHead{nullptr};
        size_t m_allocatedCount{0};
    };

} // namespace Apex::Memory
