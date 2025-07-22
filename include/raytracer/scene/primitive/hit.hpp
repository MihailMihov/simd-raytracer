#pragma once

#include <raytracer/core/math/ray3.hpp>

template <typename F>
struct primitive_hit {
    ray3<F> ray;
    vec3<F> position;
    F distance;
    F u;
    F v;
};
