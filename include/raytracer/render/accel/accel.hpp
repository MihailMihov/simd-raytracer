#pragma once

#include <optional>

#include "raytracer/core/math/ray3.hpp"
#include "raytracer/render/hit_record.hpp"

template <typename A, typename F>
concept accelerator = requires(A accel, const ray3<F>& ray, const bool backface_culling) {
    { accel.trace(ray, backface_culling) } -> std::same_as<std::optional<hit_record<F>>>;
};
