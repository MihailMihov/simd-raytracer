#pragma once

#include <mutex>
#include <optional>
#include <queue>

#include <raytracer/render/tile/tile.hpp>

struct tile_queue {
    std::queue<render_tile> queue;
    std::mutex mutex;

    tile_queue() = default;
    tile_queue(const tile_queue&) = delete;
    tile_queue& operator=(const tile_queue&) = delete;
    
    tile_queue(tile_queue&& other) noexcept
        : queue(std::move(other.queue)) {}

    std::size_t size() {
        std::lock_guard guard(mutex);
        return queue.size();
    }

    void push(render_tile new_tile) {
        std::lock_guard guard(mutex);
        queue.push(new_tile);
    }

    std::optional<render_tile> pop() {
        std::lock_guard guard(mutex);

        if (queue.empty()) {
            return std::nullopt;
        }

        render_tile popped_tile = queue.front();
        queue.pop();

        return popped_tile;
    }

    tile_queue& operator=(tile_queue&& other) noexcept {
        if (this != &other) {
            std::scoped_lock lock(mutex, other.mutex);
            queue = std::move(other.queue);
        }

        return *this;
    }
};
