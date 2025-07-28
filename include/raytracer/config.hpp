#pragma once

#include <cstddef>
#include <optional>

constexpr double fov_degrees = 90.;

constexpr double epsilon = 1e-6;
constexpr double shadow_bias = 1e-4;
constexpr double reflection_bias = 1e-4;
constexpr double refraction_bias = 1e-4;

constexpr std::size_t samples_per_pixel = 1;
constexpr std::size_t max_ray_depth = 5;
constexpr std::size_t diffuse_reflection_ray_count = 0;

constexpr std::optional fixed_rng_seed = std::make_optional(42);
