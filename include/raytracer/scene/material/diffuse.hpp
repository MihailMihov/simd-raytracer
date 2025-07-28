#pragma once

#include <raytracer/scene/color.hpp>

template <typename F>
struct diffuse_material {
    color<F> albedo;
    bool smooth_shading;
};
