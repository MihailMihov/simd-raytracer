#pragma once

#include "raytracer/scene/object/object.hpp"
#include "raytracer/scene/primitive/primitive.hpp"

template <typename F>
constexpr std::size_t material_idx_of(const object_variant<F>& ov) {
    return std::visit([](const auto& object) {
	return object.material_idx;
    }, ov);
}

template <typename F>
constexpr aabb3<F> bounding_box_of(const object_variant<F>& ov) {
    return std::visit([](const auto& object) {
	return object.box;
    }, ov);
}

template <typename F>
constexpr std::vector<primitive_variant<F>> primitives_of(const object_variant<F>& ov) {
    return std::visit([](const auto& object) {
	using O = std::decay_t<decltype(object)>;

	if constexpr (std::same_as<O, mesh<F>>) {
	    return std::vector<primitive_variant<F>>(object.triangles.begin(), object.triangles.end());
	} else {
	    return std::vector<primitive_variant<F>>();
	}
    }, ov);
}
