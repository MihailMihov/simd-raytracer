#pragma once

#include <string>

template <typename F>
struct texture_material {
    std::string texture;
    bool smooth_shading;
};
