#pragma once

#include <array>

#include <raytracer/core/math/vec2.hpp>
#include <raytracer/core/math/vec3.hpp>
#include <raytracer/core/math/aabb3.hpp>
#include <raytracer/scene/primitive/hit.hpp>

template <typename F>
struct triangle {
    vec3<F> v0, v1, v2;
    vec3<F> e1, e2;
    vec3<F> normal;
    std::array<std::size_t, 3> vertex_indices;
    std::size_t mesh_idx;
    aabb3<F> box;
    vec3<vec2<F>> uvs;

    constexpr triangle(const vec3<F>& v0, const vec3<F>& v1, const vec3<F>& v2, const std::array<std::size_t, 3>& vertex_indices, const std::size_t mesh_idx, const vec3<vec2<F>>& uvs)
	: v0(v0), v1(v1), v2(v2), vertex_indices(vertex_indices), mesh_idx(mesh_idx), uvs(uvs) {
	normal = norm(cross(v1 - v0, v2 - v0));

	e1 = v1 - v0;
	e2 = v2 - v0;
	
	box.expand(v0);
	box.expand(v1);
	box.expand(v2);
    }

    template <bool backface_culling>
    constexpr std::optional<triangle_hit<F>> intersect(const ray3<F>& ray) const noexcept {
	const vec3<F> pvec = cross(ray.direction, e2);
	const F det = dot(e1, pvec);

	if constexpr (backface_culling) {
	    if (det <= std::numeric_limits<F>::epsilon()) {
		return std::nullopt;
	    }
	} else {
	    if (std::abs(det) <= std::numeric_limits<F>::epsilon()) {
		return std::nullopt;
	    }
	}

	const F inv_det = static_cast<F>(1.) / det;
	const vec3<F> tvec = ray.origin - v0;

	const F u = dot(tvec, pvec) * inv_det;
	if (u < static_cast<F>(0.) || static_cast<F>(1.) < u) {
	    return std::nullopt;
	}

	const vec3<F> qvec = cross(tvec, e1);
	const F v = dot(ray.direction, qvec) * inv_det;
	if (v < static_cast<F>(0.) || static_cast<F>(1.) < u + v) {
	    return std::nullopt;
	}

	const F dist = dot(e2, qvec) * inv_det;
	if (dist < std::numeric_limits<F>::epsilon()) {
	    return std::nullopt;
	}

	const vec3<F> hit_position = ray.origin + (dist * ray.direction);

	return {dist, u, v};
    }
};
