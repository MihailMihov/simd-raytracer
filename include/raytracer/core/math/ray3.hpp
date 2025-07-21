#pragma once

#include <raytracer/core/math/vec3.hpp>

template <typename F>
struct ray3 {
    vec3<F> origin;
    vec3<F> direction;
};
