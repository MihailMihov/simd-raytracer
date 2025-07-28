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
struct mesh_object {
    std::size_t material_idx;
    std::vector<vec3<F>> vertices;
    std::vector<vec3<F>> vertex_normals;
    std::vector<vec2<F>> uvs;
    std::vector<triangle<F>> triangles;
    std::vector<vec3<F>> triangle_normals;
    aabb3<F> box;

    mesh_object(std::size_t material_index, std::vector<vec3<F>> vertices, std::vector<vec2<F>> uvs, std::vector<triangle<F>> triangles)
	: material_idx(material_index), vertices(vertices), uvs(uvs), triangles(triangles) {
	triangle_normals.assign(triangles.size(), vec3<F>({0., 0., 0.}));
	vertex_normals.assign(vertices.size(), vec3<F>({0., 0., 0.}));
	for (const auto& [idx, triangle] : triangles | std::views::enumerate) {
	    auto [v0_idx, v1_idx, v2_idx] = triangle.vertex_indices;

	    box.expand(triangle.v0);
	    box.expand(triangle.v1);
	    box.expand(triangle.v2);

	    triangle_normals[idx] = normalized(cross(triangle.v1 - triangle.v0, triangle.v2 - triangle.v0));

	    vertex_normals[v0_idx] += triangle_normals[idx];
	    vertex_normals[v1_idx] += triangle_normals[idx];
	    vertex_normals[v2_idx] += triangle_normals[idx];
	}

	for (auto& vn : vertex_normals) {
	    vn = normalized(vn);
	}
    }

    template <bool backface_culling, F eps>
    constexpr std::optional<mesh_hit<F>> intersect(const ray3<F>& ray) const {
	std::optional<mesh_hit<F>> closest_hit;

	for (const auto& [triangle_idx, triangle] : triangles | std::ranges::views::enumerate) {
	    const auto maybe_hit = triangle.template intersect<backface_culling, eps>(ray);

	    if (!maybe_hit || (closest_hit && closest_hit->distance < maybe_hit->distance)) {
		continue;
	    }

	    const vec3<F> v0_normal = vertex_normals[triangle.vertex_indices[0]];
	    const vec3<F> v1_normal = vertex_normals[triangle.vertex_indices[1]];
	    const vec3<F> v2_normal = vertex_normals[triangle.vertex_indices[2]];
	    const vec3<F> hit_position = ray.origin + (maybe_hit->distance * ray.direction);
	    const vec3<F> hit_normal = maybe_hit->u * v1_normal + maybe_hit->v * v2_normal + (static_cast<F>(1.) - maybe_hit->u - maybe_hit->v) * v0_normal;

	    closest_hit = mesh_hit<F>{
		hit_position,
		hit_normal,
		triangle.normal,
		triangle.uvs,
		maybe_hit->distance,
		maybe_hit->u,
		maybe_hit->v,
		static_cast<std::size_t>(triangle_idx)
	    };
	}

	return closest_hit;
    }
};
