#pragma once

#include <thread>
#include <future>

#include "raytracer/render/accel/accel.hpp"
#include <raytracer/render/accel/kd_tree.hpp>
#include <raytracer/scene/scene.hpp>
#include <raytracer/scene/material/material_queries.hpp>
#include <raytracer/scene/texture/texture_queries.hpp>
#include <raytracer/render/tile/tile.hpp>
#include <raytracer/render/tile/queue.hpp>
#include <raytracer/render/tile/single.hpp>
#include <raytracer/render/tile/region.hpp>
#include <raytracer/render/tile/bucket.hpp>

constexpr double shadow_bias = 1e-4;
constexpr double reflection_bias = 1e-4;
constexpr double refraction_bias = 1e-4;
constexpr int max_ray_depth = 4;

template <typename A, typename F>
constexpr image<F> render_frame(const A& accel, const scheduling_type threading)
requires accelerator<A, F> {
    const scene<F>& scene = *accel.scene_ptr;

    const std::size_t image_height = scene.config.image_height;
    const std::size_t image_width = scene.config.image_width;
    const camera<F> camera = scene.viewpoint;
    const color<F> background_color = scene.config.background_color;
    const F aspect_ratio = static_cast<F>(image_width) / image_height;

    std::vector<std::vector<color<F>>> pixels(image_height, std::vector<color<F>>(image_width, background_color));
    auto tile_worker = [&](render_tile tile) {
	for(std::size_t y = tile.y0; y < tile.y1; ++y) {
	    for(std::size_t x = tile.x0; x < tile.x1; ++x) {
		const F raster_x = x + static_cast<F>(0.5);
		const F raster_y = y + static_cast<F>(0.5);

		const F ndc_x = raster_x / image_width;
		const F ndc_y = raster_y / image_height;

		F screen_x = (static_cast<F>(2.) * ndc_x) - static_cast<F>(1.);
		F screen_y = static_cast<F>(1.) - (static_cast<F>(2.) * ndc_y);

		screen_x *= aspect_ratio;

		vec3<F> direction{screen_x, screen_y, static_cast<F>(-1.)};
		direction = norm(transpose(camera.matrix) * direction);

		ray3<F> ray(camera.position, direction);

		const auto camera_hit = accel.template trace<true>(ray);

		if(!camera_hit.has_value())
		    continue;

		pixels[y][x] = color_hit(accel, camera_hit.value(), 0uz);
	    }
	}
    };

    const std::size_t num_threads = std::thread::hardware_concurrency();
    tile_queue queue;
    switch (threading) {
	case scheduling_type::SINGLE_TILE:
	    queue = single_schedule(image_height, image_width);
	    break;
        case scheduling_type::REGION_TILES:
	    queue = region_schedule(image_height, image_width, num_threads);
	    break;
        case scheduling_type::BUCKET_TILES:
	    queue = bucket_schedule(image_height, image_width, scene.config.bucket_size);
	    break;
    }

    std::vector<std::future<void>> futures;
    for (std::size_t t = 0; t < num_threads; ++t) {
	futures.emplace_back(std::async(std::launch::async, [&] {
	    while (auto tile = queue.pop()) {
		tile_worker(*tile);
	    }
	}));
    }

    for (auto& future : futures) {
	future.get();
    }

    return {image_height, image_width, std::move(pixels)};
}

template <typename A, typename F>
constexpr auto is_occluded(const A& accel, ray3<F> ray, F max_t)
requires accelerator<A, F> {
    const auto& scene = *accel.scene_ptr;

    while (0 < max_t) {
	const auto hit = accel.template trace<false>(ray);

	if (!hit.has_value() || max_t < hit->distance) {
	    return false;
	}

	const auto& material = scene.materials[scene.meshes[hit->object_idx].material_idx];

	if (!is_transmissive(material)) {
	    return true;
	}

	ray.origin = hit->position + (static_cast<F>(shadow_bias) * ray.direction);
	max_t -= hit->distance;
    }

    return false;
}

template <typename A, typename F>
constexpr color<F> color_hit(const A& accel, const hit_record<F>& hit_record, const std::size_t ray_depth)
requires accelerator<A, F> {
    const scene<F>& scene = *accel.scene_ptr;

    if(ray_depth == max_ray_depth)
	return scene.config.background_color;

    auto [incoming_ray, hit_position, hit_normal, face_normal, uvs, hit_distance, u, v, w, mesh_idx] = hit_record;

    const auto& object = scene.meshes[mesh_idx];
    const auto& material = scene.materials[object.material_idx];

    return std::visit([&](const auto& m) -> color<F> {
	using M = std::decay_t<decltype(m)>;

	if constexpr (std::same_as<M, diffuse_material<F>> || std::same_as<M, texture_material<F>>) {
	    color<F> final_color(0., 0., 0.);
	    for (const auto& light : scene.lights) {
		vec3<F> light_position = light.position;
		vec3<F> light_direction = light_position - hit_position;

		F sphere_radius = light_direction.len();
		F sphere_area = 4. * std::numbers::pi_v<F> * sphere_radius * sphere_radius;

		light_direction.normalize();

		F cosine_law;
		if(m.smooth_shading) {
		    cosine_law = std::max(static_cast<F>(0.), dot(light_direction, hit_normal));
		} else {
		    cosine_law = std::max(static_cast<F>(0.), dot(light_direction, face_normal));
		}

		ray3<F> shadow_ray(hit_position + (static_cast<F>(shadow_bias) * light_direction), light_direction);
		if(is_occluded(accel, shadow_ray, sphere_radius))
		    continue;

		if constexpr (std::same_as<M, diffuse_material<F>>) {
		    final_color = final_color + color<F>(m.albedo * ((light.intensity / sphere_area) * cosine_law));
		} else {
		    const auto& texture_variant = scene.textures.at(m.texture);
		    final_color = final_color + color<F>(sample(texture_variant, hit_record, uvs) * ((light.intensity / sphere_area) * cosine_law));
		}
	    }

	    return final_color;
	} else if constexpr (std::same_as<M, reflective_material<F>>) {
	    vec3<F> reflection_direction = incoming_ray.direction - (static_cast<F>(2.) * dot(incoming_ray.direction, hit_normal) * hit_normal);
	    vec3<F> reflection_origin = hit_position + (static_cast<F>(reflection_bias) * reflection_direction);
	    ray3<F> reflection_ray(reflection_origin, reflection_direction);

	    auto reflection_hit = accel.template trace<false>(reflection_ray);

	    if(!reflection_hit.has_value())
		return scene.config.background_color;

	    return color_hit(accel, reflection_hit.value(), ray_depth + 1);	
	} else if constexpr (std::same_as<M, refractive_material<F>>) {
	    vec3<F> n = m.smooth_shading ? hit_normal : face_normal;
	    n.normalize();
	    vec3<F> i = incoming_ray.direction;
	    i.normalize();

	    F eta_i = 1.;
	    F eta_r = m.ior;

	    if(dot(i, n) > 0) {
		std::swap(eta_i, eta_r);
		n = -n;
	    }

	    F cos_i_n = -dot(i, n);
	    F sin_i_n = std::sqrt(static_cast<F>(1.) - cos_i_n * cos_i_n);

	    if(eta_r / eta_i < sin_i_n) {
		vec3<F> reflection_direction = i - static_cast<F>(2.) * dot(i, n) * n;
		ray3<F> reflection_ray(hit_position + (static_cast<F>(reflection_bias) * reflection_direction), reflection_direction);
		auto reflection_hit = accel.template trace<false>(reflection_ray);

		if(!reflection_hit.has_value())
		    return color<F>{};

		return color_hit(accel, reflection_hit.value(), ray_depth + 1);	
	    }

	    F sin_r_mn = ((sin_i_n * eta_i) / eta_r);
	    F cos_r_mn = std::sqrt(1 - sin_r_mn * sin_r_mn);

	    vec3<F> r = (cos_r_mn * (-n)) + sin_r_mn * norm(i + (cos_i_n * n));

	    ray3<F> refraction_ray(hit_position + (static_cast<F>(refraction_bias) * r), r);
	    auto refraction_hit = accel.template trace<false>(refraction_ray);

	    color<F> refraction_color = color<F>{};
	    if(refraction_hit.has_value()) {
		refraction_color = color_hit(accel, refraction_hit.value(), ray_depth + 1);
	    }

	    vec3<F> reflection_direction = i - 2 * dot(i, n) * n;
	    ray3<F> reflection_ray(hit_position + (static_cast<F>(reflection_bias) * reflection_direction), reflection_direction);
	    auto reflection_hit = accel.template trace<false>(reflection_ray);

	    color<F> reflection_color = color<F>{};
	    if(reflection_hit.has_value()) {
		reflection_color = color_hit(accel, reflection_hit.value(), ray_depth + 1);
	    }

	    F fresnel = 0.5 * std::pow(1.0 + dot(i, n), 5);
	    return fresnel * reflection_color + (1 - fresnel) * refraction_color;
	} else if constexpr (std::same_as<M, constant_material<F>>) {
	    return m.albedo;
	}

	throw std::logic_error("unknown material");
	return color<F>{};
    }, material);
}

