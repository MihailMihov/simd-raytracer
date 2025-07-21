#pragma once

#include <cstddef>

#include <raytracer/core/math/ray3.hpp>
#include <raytracer/core/math/vec3.hpp>

template <typename F>
struct hit_record {
    ray3<F> ray;
    vec3<F> position;
    vec3<F> normal;
    F distance;
    F u;
    F v;
    std::size_t object_idx;
    std::size_t face_idx;
};
