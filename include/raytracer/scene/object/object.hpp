#pragma once

#include <variant>

#include <raytracer/core/math/vec3.hpp>
#include <raytracer/core/math/ray3.hpp>
#include <raytracer/scene/object/mesh.hpp>

template <typename F>
using object_variant = std::variant<mesh<F>>;

template <typename O>
concept has_material_index = requires(const O& o) {
    o.material_index;
};
