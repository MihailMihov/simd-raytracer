#pragma once

template <typename F>
struct refractive_material {
    F ior;
    bool smooth_shading;
};
