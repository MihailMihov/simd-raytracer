#pragma once

#include <cstddef>

#include <raytracer/render/tile/queue.hpp>

inline tile_queue single_schedule(std::size_t height, std::size_t width) {
    tile_queue queue;

    queue.push({0, 0, width, height});

    return queue;
}
