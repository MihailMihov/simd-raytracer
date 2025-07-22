#pragma once

#include <memory>
#include <stack>
#include <optional>

#include <raytracer/core/math/aabb3.hpp>
#include <raytracer/scene/scene.hpp>

constexpr std::size_t max_depth = 16;
constexpr std::size_t max_primitive_count = 4;

template <typename F>
struct kd_tree_accel {
    struct kd_tree_node {
	aabb3<F> box;
	int32_t parent;
	int32_t child0;
	int32_t child1;
	std::vector<std::size_t> triangle_indices;
    };

    std::shared_ptr<const scene<F>> scene_ptr;
    std::vector<triangle<F>> triangles;
    std::vector<kd_tree_node> tree;
    std::size_t triangle_limit;

    kd_tree_accel(std::shared_ptr<const scene<F>> scene_ptr) : scene_ptr(std::move(scene_ptr)) {
	aabb3<F> root_box;
	std::vector<std::size_t> triangle_indices;
	for (const auto& mesh : this->scene_ptr->meshes) {
	    root_box.unite(mesh.box);
	    const auto& mesh_triangles = mesh.triangles;

	    std::size_t start_idx = triangles.size();
	    triangles.insert(triangles.end(), mesh_triangles.begin(), mesh_triangles.end());
	    for (std::size_t i = 0; i < mesh_triangles.size(); ++i) {
		triangle_indices.push_back(start_idx + i);
	    }
	}

	triangle_limit = triangle_indices.size();

	kd_tree_node root{root_box, -1, -1, -1, {}};
	tree.push_back(root);
	build_tree(0, 0, triangle_indices);
    }

    constexpr void set_triangle_limit(const std::size_t new_triangle_limit) noexcept {
	triangle_limit = new_triangle_limit;
    }

    constexpr void build_tree(const int32_t parent_idx, const std::size_t depth, const std::vector<std::size_t>& triangle_indices) {
	if (depth == max_depth || triangle_indices.size() <= max_primitive_count) {
	    tree[parent_idx].triangle_indices = triangle_indices;
	    return;
	}

	auto [aabb0, aabb1] = tree[parent_idx].box.split(depth % 3);

	std::vector<std::size_t> child0_triangle_indices;
	std::vector<std::size_t> child1_triangle_indices;
	for (const auto& triangle_idx : triangle_indices) {
	    const auto& triangle = triangles[triangle_idx];

	    if (aabb0.intersect(triangle.box)) {
		child0_triangle_indices.push_back(triangle_idx);
	    }

	    if (aabb1.intersect(triangle.box)) {
		child1_triangle_indices.push_back(triangle_idx);
	    }
	}

	if (!child0_triangle_indices.empty()) {
	    std::size_t child0_idx = tree.size();
	    tree.push_back(kd_tree_node{aabb0, parent_idx, -1, -1, {}});
	    tree[parent_idx].child0 = child0_idx;
	    build_tree(child0_idx, depth + 1, child0_triangle_indices);
	}

	if (!child1_triangle_indices.empty()) {
	    std::size_t child1_idx = tree.size();
	    tree.push_back(kd_tree_node{aabb1, parent_idx, -1, -1, {}});
	    tree[parent_idx].child1 = child1_idx;
	    build_tree(child1_idx, depth + 1, child1_triangle_indices);
	}
    }

    constexpr std::optional<hit_record<F>> trace(const ray3<F>& ray, const bool backface_culling, const F t_min, const F t_max) const {
	std::optional<hit_record<F>> closest_hit;

	std::stack<std::size_t> nodes_to_check;
	nodes_to_check.push(0);

	while (!nodes_to_check.empty()) {
	    auto node_idx = nodes_to_check.top();
	    nodes_to_check.pop();

	    const auto& node = tree[node_idx];
	    if (!node.box.intersect(ray)) {
		continue;
	    }

	    if (node.triangle_indices.empty()) {
		if (node.child0 != -1) {
		    nodes_to_check.push(node.child0);
		}

		if (node.child1 != -1) {
		    nodes_to_check.push(node.child1);
		}
	    } else {
		for (const auto& triangle_idx : node.triangle_indices) {
		    if (triangle_limit < triangle_idx) {
			continue;
		    }

		    const auto& triangle = triangles[triangle_idx];
		    const auto& maybe_hit = triangle.intersect(ray, backface_culling, t_min, t_max);

		    if (maybe_hit && (!closest_hit || maybe_hit->distance < closest_hit->distance)) {
			const auto& mesh = scene_ptr->meshes[triangle.mesh_idx];

			const vec3<F>& v0_normal = mesh.vertex_normals[triangle.vertex_indices[0]];
			const vec3<F>& v1_normal = mesh.vertex_normals[triangle.vertex_indices[1]];
			const vec3<F>& v2_normal = mesh.vertex_normals[triangle.vertex_indices[2]];
			vec3<F> hit_normal = v1_normal * maybe_hit->u + v2_normal * maybe_hit->v + v0_normal * (1 - maybe_hit->u - maybe_hit->v);

			closest_hit = hit_record<F>{
			    maybe_hit->ray,
			    maybe_hit->position,
			    hit_normal,
			    triangle.normal,
			    triangle.uvs,
			    maybe_hit->distance,
			    maybe_hit->u,
			    maybe_hit->v,
			    static_cast<std::size_t>(triangle.mesh_idx),
			};
		    }
		}
	    }
	}

	return closest_hit;
    }
};
