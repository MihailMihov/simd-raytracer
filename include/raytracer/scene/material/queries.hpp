#pragma once

#include <variant>

#include <raytracer/scene/material/material.hpp>

template <typename F>
constexpr bool smooth_shading_of(const material_variant<F>& mv) {
    return std::visit([](const auto& material) {
        return material.smooth_shading;
    }, mv);
}

template <typename F>
constexpr F ior_of(const material_variant<F>& mv) {
    return std::visit([](const auto& material) {
        using M = std::decay_t<decltype(material)>;

        if constexpr (std::same_as<M, refractive_material<F>>) {
            return material.ior;
        } else {
            return 1.;
        }
    }, mv);
}

template <typename F>
constexpr bool is_transmissive(const material_variant<F>& mv) {
    return std::holds_alternative<refractive_material<F>>(mv);
}
