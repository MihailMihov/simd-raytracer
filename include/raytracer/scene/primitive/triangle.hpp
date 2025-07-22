#pragma once

#include <array>

#include <raytracer/core/math/vec2.hpp>
#include <raytracer/core/math/vec3.hpp>
#include <raytracer/core/math/aabb3.hpp>
#include <raytracer/scene/primitive/hit.hpp>

template <typename F>
struct triangle {
    vec3<F> v0, v1, v2;
    std::array<std::size_t, 3> vertex_indices;
    std::size_t object_idx;
    vec3<F> e0, e1, e2;
    vec3<F> normal;
    aabb3<F> box;
    vec3<vec2<F>> uvs;

    constexpr triangle(const vec3<F>& v0, const vec3<F>& v1, const vec3<F>& v2, const std::array<std::size_t, 3>& vertex_indices, const std::size_t object_idx, const vec3<vec2<F>>& uvs)
	: v0(v0), v1(v1), v2(v2), vertex_indices(vertex_indices), object_idx(object_idx), uvs(uvs) {
	normal = cross(v1 - v0, v2 - v0).norm();

	e0 = v1 - v0;
	e1 = v2 - v1;
	e2 = v0 - v2;
	
	box.expand(v0);
	box.expand(v1);
	box.expand(v2);
    }

    constexpr std::optional<primitive_hit<F>> intersect(const ray3<F>& ray, const bool backface_culling, const F t_min, const F t_max) const {
	if(std::abs(dot(ray.direction, normal)) < std::numeric_limits<F>::epsilon())
	    return std::nullopt;

	if(backface_culling && dot(ray.direction, normal) > 0)
	    return std::nullopt;

	F dist = dot(normal, v0 - ray.origin);
	F proj = dot(normal, ray.direction);
	F hit_distance = dist / proj;

	if(hit_distance < t_min || t_max < hit_distance)
	    return std::nullopt;

	vec3<F> hit_position = ray.origin + (ray.direction * hit_distance);
	if(dot(normal, cross(e0, hit_position - v0)) < 0 ||
	   dot(normal, cross(e1, hit_position - v1)) < 0 ||
	   dot(normal, cross(e2, hit_position - v2)) < 0) {
	    return std::nullopt;
	}

	F u = cross(v0 - hit_position, v2 - v0).len() / cross(v1 - v0, v2 - v0).len();
	F v = cross(v1 - v0, hit_position - v0).len() / cross(v1 - v0, v2 - v0).len();

	return primitive_hit<F>{ray, hit_position, hit_distance, u, v};
    }
};
