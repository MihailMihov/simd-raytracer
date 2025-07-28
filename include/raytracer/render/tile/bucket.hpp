#pragma once

#include <cstddef>

#include <raytracer/render/tile/queue.hpp>

inline tile_queue bucket_schedule(std::size_t height, std::size_t width, std::size_t bucket_size) {
    tile_queue queue;
    for (std::size_t ty = 0; ty < height; ty += bucket_size) {
        for (std::size_t tx = 0; tx < width; tx += bucket_size) {
            queue.push(render_tile{
                tx,
                ty,
                std::min(tx + bucket_size, width),
                std::min(ty + bucket_size, height)
            });
        }
    }

    return queue;
}
