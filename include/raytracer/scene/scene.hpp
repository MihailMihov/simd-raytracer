#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <raytracer/scene/object/object.hpp>
#include <raytracer/scene/material/material.hpp>
#include <raytracer/scene/texture/texture.hpp>
#include <raytracer/scene/camera.hpp>
#include <raytracer/scene/light.hpp>
#include <raytracer/scene/settings.hpp>

template <typename F>
struct scene {
    settings<F> config;
    camera<F> viewpoint;
    std::vector<light<F>> lights;
    std::unordered_map<std::string, texture_variant<F>> textures;
    std::vector<material_variant<F>> materials;
    std::vector<object_variant<F>> objects;
};
