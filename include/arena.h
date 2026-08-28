// Copyright (c) 2026 Akash Pradhan
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include <cstring>
#include <new>

namespace ibex {

// High-performance arena allocator for AST nodes
// All allocations are bump-allocated with zero fragmentation
class Arena {
public:
    static constexpr size_t CHUNK_SIZE = 65536;  // 64 KB chunks

    Arena() : current_chunk_(0), current_offset_(0) {
        chunks_.push_back(std::vector<uint8_t>());
        chunks_[0].resize(CHUNK_SIZE);
    }

    ~Arena() = default;

    // Allocate uninitialized memory
    void* allocate(size_t size, size_t alignment = alignof(std::max_align_t)) {
        // Align current offset
        size_t aligned_offset = (current_offset_ + alignment - 1) & ~(alignment - 1);

        // Check if we need a new chunk
        if (aligned_offset + size > CHUNK_SIZE) {
            current_chunk_++;
            aligned_offset = 0;
            if (current_chunk_ >= chunks_.size()) {
                chunks_.push_back(std::vector<uint8_t>());
                chunks_[current_chunk_].resize(CHUNK_SIZE);
            }
        }

        void* ptr = chunks_[current_chunk_].data() + aligned_offset;
        current_offset_ = aligned_offset + size;
        return ptr;
    }

    // Allocate and construct a single object
    template<typename T, typename... Args>
    T* create(Args&&... args) {
        void* ptr = allocate(sizeof(T), alignof(T));
        return new (ptr) T(std::forward<Args>(args)...);
    }

    // Allocate array of uninitialized objects
    template<typename T>
    T* allocate_array(size_t count) {
        void* ptr = allocate(sizeof(T) * count, alignof(T));
        return static_cast<T*>(ptr);
    }

    // Reset the arena (deallocates all allocations)
    void reset() {
        current_chunk_ = 0;
        current_offset_ = 0;
        chunks_.resize(1);
    }

    // Get total memory usage
    size_t memory_usage() const {
        return chunks_.size() * CHUNK_SIZE;
    }

private:
    std::vector<std::vector<uint8_t>> chunks_;
    size_t current_chunk_ = 0;
    size_t current_offset_ = 0;
};

}  // namespace ibex
