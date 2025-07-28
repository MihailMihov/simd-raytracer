#pragma once

#include <raytracer/scene/color.hpp>
#include <raytracer/render/hit.hpp>

template <typename F>
struct checker_texture {
    color<F> color_a;
    color<F> color_b;
    F square_size;

    color<F> sample(const hit<F>& hit, const vec3<vec2<F>>& uvs) const {
	const F hit_u = hit.u;
	const F hit_v = hit.v;
	const F hit_w = 1. - hit_u - hit_v;

	vec2<F> final_uv = hit_w * uvs.x + hit_u * uvs.y + hit_v * uvs.z;

	int32_t u2 = final_uv.x / square_size;
	int32_t v2 = final_uv.y / square_size;

	if ((u2 + v2) % 2 == 0)
	    return color_a;

	return color_b;
    }
};
