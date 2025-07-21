#pragma once

#include <raytracer/scene/texture/texture.hpp>
#include <raytracer/scene/object/object.hpp>
#include <raytracer/scene/color.hpp>
#include <raytracer/render/hit_record.hpp>

template <typename F>
struct albedo_texture {
    color<F> albedo;

    constexpr color<F> sample(const hit_record<F>& hit_record, const vec3<vec2<F>>& uvs) const {
	return albedo;
    }
};
