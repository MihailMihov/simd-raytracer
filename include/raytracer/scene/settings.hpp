#pragma once

#include <cstddef>

#include <raytracer/scene/color.hpp>

template <typename F>
struct settings {
    color<F> background_color;
    std::size_t image_height;
    std::size_t image_width;
    std::size_t bucket_size;
};
