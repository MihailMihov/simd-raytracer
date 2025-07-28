#pragma once

#include "raytracer/scene/color.hpp"
#include "raytracer/scene/texture/texture.hpp"

template <typename F>
constexpr color<F> sample(const texture_variant<F>& tv, const hit<F>& hit_record, const vec3<vec2<F>>& uvs) {
    return std::visit([&](const auto& texture) {
        return texture.sample(hit_record, uvs);
    }, tv);
}
