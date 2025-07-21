#pragma once

#include <thread>
#include <future>
#include <print>

#include <raytracer/scene/scene.hpp>
#include <raytracer/scene/object/object_queries.hpp>
#include <raytracer/scene/material/material_queries.hpp>
#include <raytracer/scene/texture/texture_queries.hpp>
#include <raytracer/render/accel/accel.hpp>
#include <raytracer/render/tile/tile.hpp>
#include <raytracer/render/tile/queue.hpp>
#include <raytracer/render/tile/single.hpp>
#include <raytracer/render/tile/region.hpp>
#include <raytracer/render/tile/bucket.hpp>

constexpr double shadow_bias = 1e-5;
constexpr double reflection_bias = 1e-5;
constexpr double refraction_bias = 1e-5;
constexpr int max_ray_depth = 6;

template <typename F>
constexpr image<F> render_scene(const accel_variant<F>& accel_variant, scheduling_type threading) {
    const scene<F>& scene = std::visit([&](const auto& accel) {
	return *accel.scene_ptr;
    }, accel_variant);

    const std::size_t image_height = scene.config.image_height;
    const std::size_t image_width = scene.config.image_width;
    const camera<F> camera = scene.viewpoint;
    const color<F> background_color = scene.config.background_color;
    const F aspect_ratio = static_cast<F>(image_width) / image_height;

    std::vector<std::vector<color<F>>> pixels(image_height, std::vector<color<F>>(image_width, background_color));
    auto tile_worker = [&](render_tile tile) {
	for(std::size_t y = tile.y0; y < tile.y1; ++y) {
	    for(std::size_t x = tile.x0; x < tile.x1; ++x) {
		F raster_x = x + 0.5;
		F raster_y = y + 0.5;

		F ndc_x = raster_x / image_width;
		F ndc_y = raster_y / image_height;

		F screen_x = (2.0 * ndc_x) - 1.0;
		F screen_y = 1.0 - (2.0 * ndc_y);

		screen_x *= aspect_ratio;

		vec3<F> direction({screen_x, screen_y, -1.0});
		direction = direction * camera.matrix;
		ray3<F> ray(camera.position, direction.norm());

		auto camera_hit = std::visit([&](const auto& accel) {
		    return accel.trace(ray, true, 0., std::numeric_limits<F>::max());
		}, accel_variant);

		if(!camera_hit.has_value())
		    continue;

		pixels[y][x] = color_hit(accel_variant, camera_hit.value(), 0);
	    }
	}
    };

    std::size_t num_threads = std::thread::hardware_concurrency();
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

    if !consteval {
	std::println("Rendering {} tiles.", queue.size());
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

    return { image_height, image_width, std::move(pixels) };
}

template <typename F>
constexpr auto is_occluded(const accel_variant<F>& accel_variant, ray3<F> ray, F max_t) {
    const scene<F>& scene = std::visit([&](const auto& accel) {
	return *accel.scene_ptr;
    }, accel_variant);

    while (0 < max_t) {
	auto hit = std::visit([&](const auto& accel) {
	    return accel.trace(ray, false, 0., max_t);
	}, accel_variant);

	if (!hit.has_value()) {
	    return false;
	}

	const auto& object = std::get<mesh<F>>(scene.objects[hit->object_idx]);
	const auto& material = scene.materials[object.material_idx];

	if (!is_transmissive(material)) {
	    return true;
	}

	ray.origin = hit->position + shadow_bias * ray.direction;
	max_t -= hit->distance;
    }

    return false;
}

template<typename F>
constexpr color<F> color_hit(const accel_variant<F>& accel_variant, const hit_record<F>& hit_record, const std::size_t ray_depth) {
    const scene<F>& scene = std::visit([&](const auto& accel) {
	return *accel.scene_ptr;
    }, accel_variant);

    if(ray_depth == max_ray_depth)
	return scene.config.background_color;

    auto [incoming_ray, hit_position, hit_normal, hit_distance, u, v, object_idx, face_idx] = hit_record;

    const auto& object = std::get<mesh<F>>(scene.objects[object_idx]);
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

		light_direction = light_direction.norm();

		vec3<F> triangle_normal = object.triangle_normals[face_idx];

		F cosine_law;
		if(m.smooth_shading) {
		    cosine_law = std::max(0., dot(light_direction, hit_normal));
		} else {
		    cosine_law = std::max(0., dot(light_direction, triangle_normal));
		}

		ray3<F> shadow_ray(hit_position + shadow_bias * light_direction, light_direction);
		if(is_occluded(accel_variant, shadow_ray, sphere_radius))
		    continue;

		if constexpr (std::same_as<M, diffuse_material<F>>) {
		    final_color = final_color + color<F>(m.albedo * ((light.intensity / sphere_area) * cosine_law));
		} else {
		    const auto& texture_variant = scene.textures.at(m.texture);
		    vec2<F> uv0 = object.uvs[object.triangles[face_idx].vertex_indices[0]];
		    vec2<F> uv1 = object.uvs[object.triangles[face_idx].vertex_indices[1]];
		    vec2<F> uv2 = object.uvs[object.triangles[face_idx].vertex_indices[2]]; 
		    final_color = final_color + color<F>(sample(texture_variant, hit_record, {uv0, uv1, uv2}) * ((light.intensity / sphere_area) * cosine_law));
		}
	    }

	    return final_color;
	} else if constexpr (std::same_as<M, reflective_material<F>>) {
	    vec3<F> reflection_origin = hit_position;
	    vec3<F> reflection_direction = incoming_ray.direction - hit_normal * dot(incoming_ray.direction, hit_normal) * 2.;
	    ray3<F> reflection_ray(reflection_origin, reflection_direction);

	    auto reflection_hit = std::visit([&](const auto& accel) {
		return accel.trace(reflection_ray, false, reflection_bias, std::numeric_limits<F>::max());
	    }, accel_variant);

	    if(!reflection_hit.has_value())
		return scene.config.background_color;

	    return color_hit(accel_variant, reflection_hit.value(), ray_depth + 1);	
	} else if constexpr (std::same_as<M, refractive_material<F>>) {
	    vec3<F> n = m.smooth_shading ? hit_normal : object.triangle_normals[face_idx];
	    n = n.norm();
	    vec3<F> i = incoming_ray.direction;
	    i = i.norm();

	    F eta_i = 1.;
	    F eta_r = m.ior;

	    if(dot(i, n) > 0) {
		std::swap(eta_i, eta_r);
		n = -n;
	    }

	    F cos_i_n = -dot(i, n);
	    F sin_i_n = std::sqrt(1 - cos_i_n * cos_i_n);

	    if(eta_r / eta_i < sin_i_n) {
		ray3<F> reflection_ray(hit_position, i - n * 2. * dot(i, n));
		auto reflection_hit = std::visit([&](const auto& accel) {
		    return accel.trace(reflection_ray, false, reflection_bias, std::numeric_limits<F>::max());
		}, accel_variant);

		if(!reflection_hit.has_value())
		    return color<F>{};

		return color_hit(accel_variant, reflection_hit.value(), ray_depth + 1);	
	    }

	    F sin_r_mn = ((sin_i_n * eta_i) / eta_r);
	    F cos_r_mn = std::sqrt(1 - sin_r_mn * sin_r_mn);

	    vec3<F> r = -n * cos_r_mn + (i + n * cos_i_n).norm() * sin_r_mn;

	    ray3<F> refraction_ray(hit_position, r);
	    auto refraction_hit = std::visit([&](const auto& accel) {
		return accel.trace(refraction_ray, false, refraction_bias, std::numeric_limits<F>::max());
	    }, accel_variant);

	    color<F> refraction_color = color<F>{};
	    if(refraction_hit.has_value()) {
		refraction_color = color_hit(accel_variant, refraction_hit.value(), ray_depth + 1);
	    }

	    ray3<F> reflection_ray(hit_position, i - n * 2. * dot(i, n));
	    auto reflection_hit = std::visit([&](const auto& accel) {
		return accel.trace(reflection_ray, false, reflection_bias, std::numeric_limits<F>::max());
	    }, accel_variant);

	    color<F> reflection_color = color<F>{};
	    if(reflection_hit.has_value()) {
		reflection_color = color_hit(accel_variant, reflection_hit.value(), ray_depth + 1);
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

