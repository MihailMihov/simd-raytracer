#pragma once

#include <raytracer/core/math/aabb3.hpp>
#include <raytracer/scene/primitive/primitive.hpp>

template <typename F>
aabb3<F> bounding_box_of(const primitive_variant<F>& pv) {
    return std::visit([](const auto& primitive) {
	return primitive.box;
    }, pv);
}
