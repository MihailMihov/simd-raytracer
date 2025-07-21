#pragma once

#include <raytracer/core/math/vec3.hpp>

template <typename F>
struct light {
    vec3<F> position;
    F intensity;
};
