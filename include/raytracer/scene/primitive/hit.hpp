#pragma once

#include <raytracer/core/math/ray3.hpp>

template <typename F>
struct triangle_hit {
    F distance;
    F u;
    F v;
};
