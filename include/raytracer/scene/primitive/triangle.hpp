#pragma once

#include <array>

#include <raytracer/core/math/vec3.hpp>

template <typename F>
struct triangle {
    vec3<F> v0, v1, v2;
    vec3<F> normal;
    std::array<std::size_t, 3> vertex_indices;

    triangle(const vec3<F>& v0, const vec3<F>& v1, const vec3<F>& v2, std::array<std::size_t, 3> vertex_indices)
	: v0(v0), v1(v1), v2(v2),
	  vertex_indices(vertex_indices) {
	normal = cross(v1 - v0, v2 - v0).norm();
    }
};
