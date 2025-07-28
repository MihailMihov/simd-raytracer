#pragma once

#include <variant>

#include <raytracer/scene/material/constant.hpp>
#include <raytracer/scene/material/diffuse.hpp>
#include <raytracer/scene/material/reflective.hpp>
#include <raytracer/scene/material/refractive.hpp>
#include <raytracer/scene/material/texture.hpp>

template <typename F>
using material_variant = std::variant<diffuse_material<F>, reflective_material<F>, refractive_material<F>, constant_material<F>, texture_material<F>>;

template <typename M>
concept has_albedo = requires(const M& m) {
    m.albedo;
};

template <typename M>
concept has_texture = requires(const M& m) {
    m.texture;
};

template <typename M>
concept has_ior = requires(const M& m) {
    m.ior;
};

template <typename M>
concept has_smooth_shading = requires(const M& m) {
    m.smooth_shading;
};
