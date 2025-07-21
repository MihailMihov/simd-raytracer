#pragma once

#include <variant>
#include <optional>

#include <raytracer/core/math/ray3.hpp>
#include <raytracer/scene/primitive/triangle.hpp>
#include <raytracer/scene/primitive/sphere.hpp>
#include <raytracer/render/hit_record.hpp>

template <typename F>
using primitive_variant = std::variant<sphere<F>, triangle<F>>;

template <typename P, typename F>
concept is_intersectable = requires (const P& primitive, const ray3<F>& ray, const F t_min, const F t_max) {
    { primitive.hit(ray, t_min, t_max) } -> std::same_as<std::optional<hit_record<F>>>;
};
