#pragma once

#include <ranges>
#include <optional>
#include <vector>

#include "raytracer/core/math/aabb3.hpp"
#include "raytracer/core/math/vec2.hpp"
#include "raytracer/scene/object/hit.hpp"
#include <raytracer/core/math/vec3.hpp>
#include <raytracer/scene/primitive/triangle.hpp>

template <typename F>
struct mesh {
    std::size_t material_idx;
    std::vector<vec3<F>> vertices;
    std::vector<vec3<F>> vertex_normals;
    std::vector<vec2<F>> uvs;
    std::vector<triangle<F>> triangles;
    std::vector<vec3<F>> triangle_normals;
    aabb3<F> box;

    mesh(std::size_t material_index, std::vector<vec3<F>> vertices, std::vector<vec2<F>> uvs, std::vector<triangle<F>> triangles)
	: material_idx(material_index), vertices(vertices), uvs(uvs), triangles(triangles) {
	triangle_normals.assign(triangles.size(), vec3<F>({0., 0., 0.}));
	vertex_normals.assign(vertices.size(), vec3<F>({0., 0., 0.}));
	for(auto&& [idx, triangle] : triangles | std::views::enumerate) {
	    auto [v0_idx, v1_idx, v2_idx] = triangle.vertex_indices;

	    auto const& v0 = vertices[v0_idx];
	    auto const& v1 = vertices[v1_idx];
	    auto const& v2 = vertices[v2_idx];
	    box.expand(v0);
	    box.expand(v1);
	    box.expand(v2);

	    triangle_normals[idx] = cross(v1 - v0, v2 - v0).norm();

	    vertex_normals[v0_idx] += triangle_normals[idx];
	    vertex_normals[v1_idx] += triangle_normals[idx];
	    vertex_normals[v2_idx] += triangle_normals[idx];
	}

	for(auto& vn : vertex_normals) {
	    vn = vn.norm();
	}
    }

    constexpr std::optional<object_hit<F>> intersect(const ray3<F>& ray, bool backface_culling, F t_min, F t_max) const {
	std::optional<object_hit<F>> closest_hit;

	for(const auto& [triangle_idx, triangle] : triangles | std::ranges::views::enumerate) {
	    vec3<F> triangle_normal = triangle_normals[triangle_idx];

	    if(std::abs(dot(ray.direction, triangle_normal)) < std::numeric_limits<F>::epsilon())
		continue;

	    if(backface_culling && dot(ray.direction, triangle_normal) > 0)
		continue;

	    vec3<F> triangle_v0 = vertices[triangle.vertex_indices[0]];
	    vec3<F> triangle_v1 = vertices[triangle.vertex_indices[1]];
	    vec3<F> triangle_v2 = vertices[triangle.vertex_indices[2]];
	    vec3<F> triangle_e0 = triangle_v1 - triangle_v0;
	    vec3<F> triangle_e1 = triangle_v2 - triangle_v1;
	    vec3<F> triangle_e2 = triangle_v0 - triangle_v2;

	    F dist = dot(triangle_normal, triangle_v0 - ray.origin);
	    F proj = dot(triangle_normal, ray.direction);
	    F hit_distance = dist / proj;

	    if(hit_distance < t_min || t_max < hit_distance)
		continue;

	    if(closest_hit.has_value() && closest_hit.value().distance < hit_distance)
		continue;

	    vec3<F> hit_position = ray.origin + (ray.direction * hit_distance);

	    if(dot(triangle_normal, cross(triangle_e0, hit_position - triangle_v0)) < 0 ||
	       dot(triangle_normal, cross(triangle_e1, hit_position - triangle_v1)) < 0 ||
	       dot(triangle_normal, cross(triangle_e2, hit_position - triangle_v2)) < 0) {
		continue;
	    }

	    if(!closest_hit.has_value() || hit_distance < closest_hit.value().distance) {
		F u = cross(triangle_v0 - hit_position, triangle_v2 - triangle_v0).len() / cross(triangle_v1 - triangle_v0, triangle_v2 - triangle_v0).len();
		F v = cross(triangle_v1 - triangle_v0, hit_position - triangle_v0).len() / cross(triangle_v1 - triangle_v0, triangle_v2 - triangle_v0).len();
		vec3<F> v0_normal = vertex_normals[triangle.vertex_indices[0]];
		vec3<F> v1_normal = vertex_normals[triangle.vertex_indices[1]];
		vec3<F> v2_normal = vertex_normals[triangle.vertex_indices[2]];
		vec3<F> hit_normal = v1_normal * u + v2_normal * v + v0_normal * (1 - u - v);
		closest_hit = object_hit<F>(ray, hit_position, hit_normal, hit_distance, u, v, triangle_idx);
	    }
	}

	return closest_hit;
    }
};
