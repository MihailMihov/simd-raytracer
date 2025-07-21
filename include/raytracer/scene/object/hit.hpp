#pragma once

#include <cstddef>

#include <raytracer/core/math/vec3.hpp>
#include <raytracer/core/math/ray3.hpp>

template <typename F>
struct object_hit {
    ray3<F> ray;
    vec3<F> position;
    vec3<F> normal;
    F distance;
    F u;
    F v;
    std::size_t triangle_idx;
};
