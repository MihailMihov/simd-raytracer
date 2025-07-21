#pragma once

#include <cstddef>

enum struct scheduling_type {
    SINGLE_TILE,
    REGION_TILES,
    BUCKET_TILES,
};

struct render_tile {
    std::size_t x0;
    std::size_t y0;
    std::size_t x1;
    std::size_t y1;
};
