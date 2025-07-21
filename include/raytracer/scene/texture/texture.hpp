#pragma once

#include <variant>

#include <raytracer/scene/texture/albedo.hpp>
#include <raytracer/scene/texture/edge.hpp>
#include <raytracer/scene/texture/checker.hpp>
#include <raytracer/scene/texture/bitmap.hpp>

template <typename F>
using texture_variant = std::variant<albedo_texture<F>, edge_texture<F>, checker_texture<F>, bitmap_texture<F>>;

template <typename F>
struct named_texture {
    std::string name;
    texture_variant<F> texture;
};
