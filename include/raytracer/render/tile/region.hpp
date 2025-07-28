#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>

#include <raytracer/render/tile/queue.hpp>

inline tile_queue region_schedule(std::size_t height, std::size_t width, std::size_t num_threads) {
    const std::size_t side = std::max<std::size_t>(1, static_cast<std::size_t>(std::sqrt(num_threads)));
    const std::size_t tile_width = (width + side - 1) / side;
    const std::size_t tile_height = (height + side - 1) / side;

    tile_queue queue;
    for (std::size_t ty = 0; ty < side; ++ty) {
        for (std::size_t tx = 0; tx < side; ++tx)
        {
            std::size_t x0 = tx * tile_width;
            std::size_t y0 = ty * tile_height;
            std::size_t x1 = std::min(x0 + tile_width, width);
            std::size_t y1 = std::min(y0 + tile_height, height);
            if (x0 < x1 && y0 < y1)
                queue.push({x0, y0, x1, y1});
        }
    }

    return queue;
}
