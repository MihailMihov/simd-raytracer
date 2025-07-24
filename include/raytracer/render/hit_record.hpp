#pragma once

#include <cstddef>

#include <raytracer/core/math/vec2.hpp>
#include <raytracer/core/math/ray3.hpp>
#include <raytracer/core/math/vec3.hpp>

template <typename F>
struct scene_hit {
    ray3<F> ray;
    vec3<F> position;
    vec3<F> hit_normal;
    vec3<F> face_normal;
    vec3<vec2<F>> uvs;
    F distance;
    F u;
    F v;
    F w;
    std::size_t object_idx;
};
