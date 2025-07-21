#pragma once

#include <memory>
#include <optional>

#include <raytracer/core/math/aabb3.hpp>
#include <raytracer/scene/scene.hpp>
#include <raytracer/scene/object/object_queries.hpp>
#include <raytracer/scene/primitive/primitive.hpp>
#include <stack>

constexpr std::size_t max_depth = 8;
constexpr std::size_t max_primitive_count = 16;

template <typename F>
struct kd_tree_accel {
    struct kd_tree_node {
	aabb3<F> box;
	int32_t parent;
	int32_t child0;
	int32_t child1;
	std::vector<primitive_variant<F>> primitives;
    };

    std::shared_ptr<const scene<F>> scene_ptr;
    std::vector<kd_tree_node> tree;

    kd_tree_accel(std::shared_ptr<const scene<F>> scene_ptr) : scene_ptr(std::move(scene_ptr)) {
	aabb3<F> root_box;
	std::vector<primitive_variant<F>> primitives;
	for (const auto& object_variant : this->scene_ptr->objects) {
	    root_box.unite(bounding_box_of(object_variant));
	    const auto& object_primitives = primitives_of(object_variant);
	    primitives.insert(primitives.end(), object_primitives.begin(), object_primitives.end());
	}

	kd_tree_node root{root_box, -1, -1, -1, {}};
	build_tree(0, 0, primitives);
    }

    void build_tree(int32_t parent_idx, const std::size_t depth, const std::vector<primitive_variant<F>>& primitives) {
	if (depth == max_depth || primitives.size() <= max_primitive_count) {
	    tree[parent_idx].primitives = primitives;
	    return;
	}

	auto [aabb0, aabb1] = tree[parent_idx].box.split(depth % 3);

	std::vector<primitive_variant<F>> child0_primitives;
	std::vector<primitive_variant<F>> child1_primitives;
	for (const auto& primitive_variant : primitives) {
	    // FIXME
	    aabb3<F> primitive_box = aabb3<F>();

	    if (aabb0.intersect(primitive_box)) {
		child0_primitives.push_back(primitive_variant);
	    }

	    if (aabb1.intersect(primitive_box)) {
		child1_primitives.push_back(primitive_variant);
	    }
	}

	if (!child0_primitives.empty()) {
	    int32_t child0_idx = tree.size();
	    tree.push_back(kd_tree_node{aabb0, parent_idx, -1, -1, {}});
	    build_tree(child0_idx, depth + 1, child0_primitives);
	}

	if (!child1_primitives.empty()) {
	    int32_t child1_idx = tree.size();
	    tree.push_back(kd_tree_node{aabb1, parent_idx, -1, -1, {}});
	    build_tree(child1_idx, depth + 1, child1_primitives);
	}
    }

    constexpr std::optional<hit_record<F>> trace(const ray3<F>& ray, const bool backface_culling, const F t_min, const F t_max) const {
	std::optional<hit_record<F>> closest_hit;

	std::stack<std::size_t> nodes_to_check;
	nodes_to_check.push(0);

	while (nodes_to_check.empty()) {
	    auto node_idx = nodes_to_check.top();
	    nodes_to_check.pop();

	    auto node = tree[node_idx];
	    if (!node.box.intersect(ray)) {
		continue;
	    }

	    if (node.primitives.empty()) {
		if (node.child0 != -1) {
		    nodes_to_check.push(node.child0);
		}

		if (node.child1 != -1) {
		    nodes_to_check.push(node.child1);
		}
	    } else {
		// FIXME
		for(const auto& [object_idx, object_variant] : scene_ptr->objects | std::ranges::views::enumerate) {
		    auto object_hit = std::visit([&](const auto& object) {
			return object.intersect(ray, backface_culling, t_min, t_max);
		    }, object_variant);

		    if (object_hit && (!closest_hit || object_hit->distance < closest_hit->distance)) {
			closest_hit = hit_record<F>{
			    object_hit->ray,
			    object_hit->position,
			    object_hit->normal,
			    object_hit->distance,
			    object_hit->u,
			    object_hit->v,
			    static_cast<std::size_t>(object_idx),
			    object_hit->triangle_idx
			};
		    }
		}
	    }
	}

	return closest_hit;
    }
};
