#pragma once

#include <optional>

#include "raytracer/core/math/ray3.hpp"
#include "raytracer/render/hit_record.hpp"

template <typename A, typename F>
concept accelerator = requires(A accel, const ray3<F>& ray, const bool backface_culling, const F t_min, const F t_max) {
    { accel.trace(ray, backface_culling, t_min, t_max) } -> std::same_as<std::optional<hit_record<F>>>;
};
