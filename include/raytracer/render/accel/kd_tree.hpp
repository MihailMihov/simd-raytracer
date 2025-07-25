#pragma once

#include <memory>
#include <stack>
#include <optional>

#include <raytracer/core/math/aabb3.hpp>
#include <raytracer/scene/scene.hpp>

constexpr std::size_t max_depth = 8;
constexpr std::size_t max_primitive_count = 16;

template <typename F,
	  F eps,
	  std::size_t max_depth = 8,
	  std::size_t max_primitive_count = 16>
struct kd_tree_accel {
    struct kd_tree_node {
	aabb3<F> box;
	int32_t parent;
	int32_t child0;
	int32_t child1;
	int32_t start_idx;
	std::size_t count;
    };

    std::shared_ptr<const scene<F>> scene_ptr;
    std::vector<triangle<F>> triangles;
    std::vector<kd_tree_node> tree;
    std::vector<std::size_t> leaf_indices;

    constexpr kd_tree_accel(std::shared_ptr<const scene<F>> scene_ptr) noexcept
	: scene_ptr(std::move(scene_ptr)) {
	aabb3<F> root_box;
	std::vector<std::size_t> triangle_indices;
	for (const auto& mesh : this->scene_ptr->meshes) {
	    root_box.unite(mesh.box);

	    std::size_t start_idx = triangles.size();
	    triangles.insert(triangles.end(), mesh.triangles.begin(), mesh.triangles.end());
	    for (std::size_t i = 0; i < mesh.triangles.size(); ++i) {
		triangle_indices.push_back(start_idx + i);
	    }
	}

	tree.emplace_back(root_box, -1, -1, -1, -1, 0);
	build_tree(0, 0, triangle_indices);
    }

    constexpr void build_tree(const int32_t parent_idx, const std::size_t depth, const std::vector<std::size_t>& triangle_indices) {
	if (depth == max_depth || triangle_indices.size() <= max_primitive_count) {
	    tree[parent_idx].start_idx = leaf_indices.size();
	    leaf_indices.insert(leaf_indices.end(), triangle_indices.begin(), triangle_indices.end());
	    tree[parent_idx].count = leaf_indices.size() - tree[parent_idx].start_idx;
	    return;
	}

	auto [aabb0, aabb1] = tree[parent_idx].box.split(depth % 3);

	std::vector<std::size_t> child0_triangle_indices;
	child0_triangle_indices.reserve(triangle_indices.size());

	std::vector<std::size_t> child1_triangle_indices;
	child1_triangle_indices.reserve(triangle_indices.size());

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
	    tree.emplace_back(aabb0, parent_idx, -1, -1, -1, 0);
	    tree[parent_idx].child0 = child0_idx;
	    build_tree(child0_idx, depth + 1, child0_triangle_indices);
	}

	if (!child1_triangle_indices.empty()) {
	    std::size_t child1_idx = tree.size();
	    tree.emplace_back(aabb1, parent_idx, -1, -1, -1, 0);
	    tree[parent_idx].child1 = child1_idx;
	    build_tree(child1_idx, depth + 1, child1_triangle_indices);
	}
    }

    template <bool backface_culling>
    constexpr std::optional<scene_hit<F>> intersect(const ray3<F>& ray) const {
	std::optional<scene_hit<F>> closest_hit;

	std::stack<std::size_t, std::vector<std::size_t>> nodes_to_check;
	nodes_to_check.push(0);

	while (!nodes_to_check.empty()) {
	    const auto node_idx = nodes_to_check.top();
	    nodes_to_check.pop();

	    const auto& node = tree[node_idx];

	    const F best_t = closest_hit
		.transform([](const auto& hit) { return hit.distance; })
		.value_or(std::numeric_limits<F>::max());

	    auto maybe_box_hit = node.box.intersect(ray, best_t);
	    if (!maybe_box_hit || best_t <= maybe_box_hit->t_min) {
		continue;
	    }

	    if (node.start_idx == -1) {
		if (node.child0 != -1) {
		    nodes_to_check.push(node.child0);
		}

		if (node.child1 != -1) {
		    nodes_to_check.push(node.child1);
		}
	    } else {
		for (std::size_t triangle_idx = node.start_idx; triangle_idx < node.start_idx + node.count; ++triangle_idx) {
		    const auto& triangle = triangles[leaf_indices[triangle_idx]];

		    const auto maybe_hit = triangle.template intersect<backface_culling>(ray);

		    if (maybe_hit && (!closest_hit || maybe_hit->distance < closest_hit->distance)) {
			const auto& mesh = scene_ptr->meshes[triangle.mesh_idx];

			const auto& v0_normal = mesh.vertex_normals[triangle.vertex_indices[0]];
			const auto& v1_normal = mesh.vertex_normals[triangle.vertex_indices[1]];
			const auto& v2_normal = mesh.vertex_normals[triangle.vertex_indices[2]];

			const F u = maybe_hit->u;
			const F v = maybe_hit->v;
			const F w = static_cast<F>(1.) - u - v;
			const vec3<F> hit_normal = u * v1_normal + v * v2_normal + w * v0_normal;

			const vec3<F> hit_position = ray.origin + (maybe_hit->distance * ray.direction);

			closest_hit = scene_hit<F>{
			    ray,
			    hit_position,
			    hit_normal,
			    triangle.normal,
			    triangle.uvs,
			    maybe_hit->distance,
			    u,
			    v,
			    w,
			    triangle.mesh_idx
			};
		    }
		}
	    }
	}

	return closest_hit;
    }
};
