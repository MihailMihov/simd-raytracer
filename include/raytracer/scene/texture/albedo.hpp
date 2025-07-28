#pragma once

#include <raytracer/scene/texture/texture.hpp>
#include <raytracer/scene/color.hpp>
#include <raytracer/render/hit.hpp>

template <typename F>
struct albedo_texture {
    color<F> albedo;

    constexpr color<F> sample([[maybe_unused]] const hit<F>& hit_record, [[maybe_unused]] const vec3<vec2<F>>& uvs) const {
        return albedo;
    }
};
