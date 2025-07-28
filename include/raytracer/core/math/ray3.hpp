#pragma once

#include <raytracer/core/math/vec3.hpp>

template <typename F>
struct ray3 {
    vec3<F> origin;
    vec3<F> direction;
    vec3<F> inv_direction;

    constexpr ray3(const vec3<F>& origin, const vec3<F>& direction) noexcept
        : origin(origin),
          direction(direction),
          inv_direction(static_cast<F>(1.) / direction) {}
};
