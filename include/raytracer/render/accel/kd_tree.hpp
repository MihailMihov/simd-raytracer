#pragma once

#include <memory>
#include <stack>
#include <optional>

#include <raytracer/core/math/aabb3.hpp>
#include <raytracer/scene/scene.hpp>
#include <raytracer/scene/object/object_queries.hpp>
#include <raytracer/scene/primitive/primitive.hpp>
#include <raytracer/scene/primitive/primitive_queries.hpp>

constexpr std::size_t max_depth = 16;
constexpr std::size_t max_primitive_count = 4;

template <typename F>
struct kd_tree_accel {
    struct kd_tree_node {
	aabb3<F> box;
	int32_t parent;
	int32_t child0;
	int32_t child1;
	std::vector<std::size_t> primitive_indices;
    };

    std::shared_ptr<const scene<F>> scene_ptr;
    std::vector<primitive_variant<F>> primitives;
    std::vector<kd_tree_node> tree;
    std::size_t primitives_limit;

    kd_tree_accel(std::shared_ptr<const scene<F>> scene_ptr) : scene_ptr(std::move(scene_ptr)) {
	aabb3<F> root_box;
	std::vector<std::size_t> primitive_indices;
	for (const auto& object_variant : this->scene_ptr->objects) {
	    root_box.unite(bounding_box_of(object_variant));
	    const auto& object_primitives = primitives_of(object_variant);

	    std::size_t start_idx = primitives.size();
	    primitives.insert(primitives.end(), object_primitives.begin(), object_primitives.end());
	    for (std::size_t i = 0; i < object_primitives.size(); ++i) {
		primitive_indices.push_back(start_idx + i);
	    }
	}

	primitives_limit = primitive_indices.size();

	kd_tree_node root{root_box, -1, -1, -1, {}};
	tree.push_back(root);
	build_tree(0, 0, primitive_indices);
    }

    constexpr void set_primitives_limit(const std::size_t new_primitives_limit) noexcept {
	primitives_limit = new_primitives_limit;
    }

    constexpr void build_tree(const int32_t parent_idx, const std::size_t depth, const std::vector<std::size_t>& primitive_indices) {
	if (depth == max_depth || primitive_indices.size() <= max_primitive_count) {
	    tree[parent_idx].primitive_indices = primitive_indices;
	    return;
	}

	auto [aabb0, aabb1] = tree[parent_idx].box.split(depth % 3);

	std::vector<std::size_t> child0_primitive_indices;
	std::vector<std::size_t> child1_primitive_indices;
	for (const auto& primitive_idx : primitive_indices) {
	    const auto& primitive_variant = primitives[primitive_idx];
	    aabb3<F> primitive_box = bounding_box_of(primitive_variant);

	    if (aabb0.intersect(primitive_box)) {
		child0_primitive_indices.push_back(primitive_idx);
	    }

	    if (aabb1.intersect(primitive_box)) {
		child1_primitive_indices.push_back(primitive_idx);
	    }
	}

	if (!child0_primitive_indices.empty()) {
	    int32_t child0_idx = tree.size();
	    tree.push_back(kd_tree_node{aabb0, parent_idx, -1, -1, {}});
	    tree[parent_idx].child0 = child0_idx;
	    build_tree(child0_idx, depth + 1, child0_primitive_indices);
	}

	if (!child1_primitive_indices.empty()) {
	    int32_t child1_idx = tree.size();
	    tree.push_back(kd_tree_node{aabb1, parent_idx, -1, -1, {}});
	    tree[parent_idx].child1 = child1_idx;
	    build_tree(child1_idx, depth + 1, child1_primitive_indices);
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

	    if (node.primitive_indices.empty()) {
		if (node.child0 != -1) {
		    nodes_to_check.push(node.child0);
		}

		if (node.child1 != -1) {
		    nodes_to_check.push(node.child1);
		}
	    } else {
		for (const auto& primitive_idx : node.primitive_indices) {
		    if (primitives_limit < primitive_idx) {
			continue;
		    }

		    const auto& primitive_variant = primitives[primitive_idx];
		    std::visit([&](const auto& primitive) {
			const auto& maybe_hit = primitive.intersect(ray, backface_culling, t_min, t_max);

			if (maybe_hit && (!closest_hit || maybe_hit->distance < closest_hit->distance)) {
			    const auto& object = std::get<mesh<F>>(scene_ptr->objects[primitive.object_idx]);

			    const vec3<F>& v0_normal = object.vertex_normals[primitive.vertex_indices[0]];
			    const vec3<F>& v1_normal = object.vertex_normals[primitive.vertex_indices[1]];
			    const vec3<F>& v2_normal = object.vertex_normals[primitive.vertex_indices[2]];
			    vec3<F> hit_normal = v1_normal * maybe_hit->u + v2_normal * maybe_hit->v + v0_normal * (1 - maybe_hit->u - maybe_hit->v);

			    closest_hit = hit_record<F>{
				maybe_hit->ray,
				maybe_hit->position,
				hit_normal,
				primitive.normal,
				primitive.uvs,
				maybe_hit->distance,
				maybe_hit->u,
				maybe_hit->v,
				static_cast<std::size_t>(primitive.object_idx),
			    };
			}
		    }, primitive_variant);
		}
	    }
	}

	return closest_hit;
    }
};
