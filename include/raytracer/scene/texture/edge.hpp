#pragma once

#include <raytracer/core/math/vec2.hpp>
#include <raytracer/scene/color.hpp>
#include <raytracer/render/hit.hpp>

template <typename F>
struct edge_texture {
    color<F> edge_color;
    color<F> inner_color;
    F edge_width;

    color<F> sample(const hit<F>& hit, [[maybe_unused]] const vec3<vec2<F>>& uvs) const {
        const F hit_u = hit.u;
        const F hit_v = hit.v;
        const F hit_w = 1. - hit_u - hit_v;

        if (hit_u < edge_width || hit_v < edge_width || hit_w < edge_width)
            return edge_color;

        return inner_color;
    }
};
