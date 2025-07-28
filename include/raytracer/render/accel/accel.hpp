#pragma once

#include <optional>

#include <raytracer/core/math/ray3.hpp>
#include <raytracer/render/hit.hpp>

template <typename A, typename F>
concept accelerator = requires(A accel, const ray3<F>& ray) {
    { accel.template intersect<true>(ray) } -> std::same_as<std::optional<hit<F>>>;
    { accel.template intersect<false>(ray) } -> std::same_as<std::optional<hit<F>>>;
};
