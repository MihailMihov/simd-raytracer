#pragma once

#include <raytracer/core/math/vec3.hpp>

template <typename F>
struct sphere {
    vec3<F> origin;
    F radius;
};
