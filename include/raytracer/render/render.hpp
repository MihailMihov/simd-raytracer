#pragma once

#include <thread>
#include <random>

#include <raytracer/config.hpp>
#include <raytracer/scene/scene.hpp>
#include <raytracer/scene/material/queries.hpp>
#include <raytracer/scene/texture/queries.hpp>
#include <raytracer/render/accel/accel.hpp>
#include <raytracer/render/tile/tile.hpp>
#include <raytracer/render/tile/queue.hpp>
#include <raytracer/render/tile/single.hpp>
#include <raytracer/render/tile/region.hpp>
#include <raytracer/render/tile/bucket.hpp>

template <typename F>
F urand01() noexcept {
    const auto get_rng_seed = []{
	if constexpr (fixed_rng_seed.has_value()) {
	    return fixed_rng_seed.value();
	} else {
	    std::random_device rd;
	    return rd();
	}
    };

    thread_local std::minstd_rand engine(get_rng_seed());

    return std::generate_canonical<F, std::numeric_limits<F>::digits>(engine);
}

template <typename F>
constexpr F degrees_to_radians(const F degrees) noexcept {
    return degrees * (std::numbers::pi_v<F> / static_cast<F>(180.));
}

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
    const auto tile_worker = [&](render_tile tile) {
	for (std::size_t y = tile.y0; y < tile.y1; ++y) {
	    for (std::size_t x = tile.x0; x < tile.x1; ++x) {
		color<F> final_color{};

		for (std::size_t s = 0; s < samples_per_pixel; ++s) {
		    F raster_x = x;
		    F raster_y = y;

		    if constexpr (samples_per_pixel == 1) {
			raster_x += static_cast<F>(0.5);
			raster_y += static_cast<F>(0.5);
		    } else {
			raster_x += urand01<F>();
			raster_y += urand01<F>();
		    }

		    const F ndc_x = raster_x / image_width;
		    const F ndc_y = raster_y / image_height;

		    F screen_x = (static_cast<F>(2.) * ndc_x) - static_cast<F>(1.);
		    F screen_y = static_cast<F>(1.) - (static_cast<F>(2.) * ndc_y);

		    screen_x *= aspect_ratio;

		    const F fov_radians = degrees_to_radians(fov_degrees);
		    screen_x *= std::tan(fov_radians / static_cast<F>(2.));
		    screen_y *= std::tan(fov_radians / static_cast<F>(2.));

		    vec3<F> direction{screen_x, screen_y, static_cast<F>(-1.)};
		    direction = normalized(transpose(camera.matrix) * direction);

		    const ray3<F> ray(camera.position, direction);

		    const auto camera_hit = accel.template intersect<true>(ray);
		    if (camera_hit.has_value()) {
			final_color += color_hit(accel, camera_hit.value(), 0uz);
		    } else {
			final_color += background_color;
		    }
		}

		final_color /= static_cast<F>(samples_per_pixel);

		pixels[y][x] = final_color;
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

    std::vector<std::jthread> threads;
    threads.reserve(num_threads);
    for (std::size_t t = 0; t < num_threads; ++t) {
	threads.emplace_back([&] {
	    while (auto tile = queue.pop()) {
		tile_worker(*tile);
	    }
	});
    }

    for (auto& thread : threads) {
	thread.join();
    }

    return {image_height, image_width, std::move(pixels)};
}

template <typename A, typename F>
constexpr auto is_occluded(const A& accel, ray3<F> ray, F max_t)
requires accelerator<A, F> {
    const auto& scene = *accel.scene_ptr;

    while (static_cast<F>(0.) < max_t) {
	const auto maybe_hit = accel.template intersect<false>(ray);
	if (!maybe_hit.has_value() || max_t < maybe_hit->distance) {
	    return false;
	}

	const auto& material = scene.materials[scene.meshes[maybe_hit->mesh_idx].material_idx];
	if (!is_transmissive(material)) {
	    return true;
	}

	ray.origin = maybe_hit->position + (static_cast<F>(shadow_bias) * ray.direction);
	max_t -= maybe_hit->distance;
    }

    return false;
}

template <typename A, typename F>
constexpr color<F> color_hit(const A& accel, const hit<F>& hit_record, const std::size_t ray_depth) noexcept
requires accelerator<A, F> {
    const auto& scene = *accel.scene_ptr;

    if (ray_depth == max_ray_depth)
	return scene.config.background_color;

    auto [incoming_ray, hit_position, hit_normal, face_normal, uvs, hit_distance, u, v, w, mesh_idx] = hit_record;

    const auto& mesh = scene.meshes[mesh_idx];
    const auto& material_variant = scene.materials[mesh.material_idx];

    return std::visit([&](const auto& material) -> color<F> {
	using M = std::decay_t<decltype(material)>;

	if constexpr (std::same_as<M, diffuse_material<F>>) {
	    color<F> final_color{};
	    for (std::size_t i = 0; i < diffuse_reflection_ray_count; ++i) {
		const vec3<F> right_axis = normalized(cross(incoming_ray.direction, hit_normal));
		const vec3<F> up_axis = hit_normal;
		const vec3<F> forward_axis = cross(right_axis, up_axis);

		const mat3<F> local_hit_mat(right_axis, up_axis, forward_axis);

		const F rand_xy_angle = std::numbers::pi_v<F> * urand01<F>();
		vec3<F> rand_xy_vec{std::cos(rand_xy_angle), std::sin(rand_xy_angle), 0};

		const F rand_xz_angle = std::numbers::pi_v<F> * urand01<F>() * static_cast<F>(2.);
		const mat3<F> rotate_y_mat{{
		    std::cos(rand_xz_angle),	static_cast<F>(0.), -std::sin(rand_xz_angle),
		    static_cast<F>(0.),		static_cast<F>(1.), static_cast<F>(0.),
		    std::sin(rand_xz_angle),	static_cast<F>(0.), std::cos(rand_xz_angle)
		}};

		rand_xy_vec = rotate_y_mat * rand_xy_vec;

		const vec3<F> diffuse_reflection_ray_origin = hit_position + (static_cast<F>(reflection_bias) * hit_normal);
		const vec3<F> diffuse_reflection_ray_direction = local_hit_mat * rand_xy_vec;

		const ray3<F> diffuse_reflection_ray{diffuse_reflection_ray_origin, diffuse_reflection_ray_direction};

		const auto diffuse_reflection_hit = accel.template intersect<false>(diffuse_reflection_ray);

		if (!diffuse_reflection_hit.has_value()) {
		    continue;
		}

		final_color += color_hit(accel, diffuse_reflection_hit.value(), ray_depth + 1);
	    }

	    for (const auto& light : scene.lights) {
		const vec3<F> light_position = light.position;
		vec3<F> light_direction = light_position - hit_position;

		const F sphere_radius = light_direction.len();
		const F sphere_area = static_cast<F>(4.) * std::numbers::pi_v<F> * sphere_radius * sphere_radius;

		light_direction = normalized(light_direction);

		F cosine_law;
		if (material.smooth_shading) {
		    cosine_law = std::max(static_cast<F>(0.), dot(light_direction, hit_normal));
		} else {
		    cosine_law = std::max(static_cast<F>(0.), dot(light_direction, face_normal));
		}

		const ray3<F> shadow_ray(hit_position + (static_cast<F>(shadow_bias) * light_direction), light_direction);
		if (is_occluded(accel, shadow_ray, sphere_radius)) {
		    continue;
		}

		final_color += ((light.intensity / sphere_area) * cosine_law) * material.albedo;
	    }

	    final_color /= static_cast<F>(diffuse_reflection_ray_count + 1);

	    return final_color;
	} else if constexpr (std::same_as<M, texture_material<F>>) {
	    color<F> final_color{};
	    for (const auto& light : scene.lights) {
		const vec3<F> light_position = light.position;
		vec3<F> light_direction = light_position - hit_position;

		const F sphere_radius = light_direction.len();
		const F sphere_area = static_cast<F>(4.) * std::numbers::pi_v<F> * sphere_radius * sphere_radius;

		light_direction = normalized(light_direction);

		F cosine_law;
		if (material.smooth_shading) {
		    cosine_law = std::max(static_cast<F>(0.), dot(light_direction, hit_normal));
		} else {
		    cosine_law = std::max(static_cast<F>(0.), dot(light_direction, face_normal));
		}

		const ray3<F> shadow_ray(hit_position + (static_cast<F>(shadow_bias) * light_direction), light_direction);
		if (is_occluded(accel, shadow_ray, sphere_radius)) {
		    continue;
		}

		const auto& texture_variant = scene.textures.at(material.texture);
		final_color += ((light.intensity / sphere_area) * cosine_law) * sample(texture_variant, hit_record, uvs);
	    }

	    return final_color;
	} else if constexpr (std::same_as<M, reflective_material<F>>) {
	    const vec3<F> reflection_direction = incoming_ray.direction - (static_cast<F>(2.) * dot(incoming_ray.direction, hit_normal) * hit_normal);
	    const vec3<F> reflection_origin = hit_position + (static_cast<F>(reflection_bias) * reflection_direction);
	    const ray3<F> reflection_ray(reflection_origin, reflection_direction);

	    const auto reflection_hit = accel.template intersect<false>(reflection_ray);

	    if (!reflection_hit.has_value()) {
		return scene.config.background_color;
	    }

	    return color_hit(accel, reflection_hit.value(), ray_depth + 1);	
	} else if constexpr (std::same_as<M, refractive_material<F>>) {
	    vec3<F> n = normalized(material.smooth_shading ? hit_normal : face_normal);
	    vec3<F> i = normalized(incoming_ray.direction);

	    F eta_i = static_cast<F>(1.);
	    F eta_r = material.ior;

	    if (static_cast<F>(0.) < dot(i, n)) {
		std::swap(eta_i, eta_r);
		n = -n;
	    }

	    const F cos_i_n = -dot(i, n);
	    const F sin_i_n = std::sqrt(static_cast<F>(1.) - cos_i_n * cos_i_n);

	    if (eta_r / eta_i < sin_i_n) {
		const vec3<F> reflection_direction = i - static_cast<F>(2.) * dot(i, n) * n;
		const ray3<F> reflection_ray(hit_position + (static_cast<F>(reflection_bias) * reflection_direction), reflection_direction);
		const auto reflection_hit = accel.template intersect<false>(reflection_ray);

		if (!reflection_hit.has_value()) {
		    return color<F>{};
		}

		return color_hit(accel, reflection_hit.value(), ray_depth + 1);	
	    }

	    const F sin_r_mn = ((sin_i_n * eta_i) / eta_r);
	    const F cos_r_mn = std::sqrt(static_cast<F>(1.) - sin_r_mn * sin_r_mn);

	    const vec3<F> r = (cos_r_mn * (-n)) + sin_r_mn * normalized(i + (cos_i_n * n));

	    const ray3<F> refraction_ray(hit_position + (static_cast<F>(refraction_bias) * r), r);
	    const auto refraction_hit = accel.template intersect<false>(refraction_ray);

	    color<F> refraction_color{};
	    if (refraction_hit.has_value()) {
		refraction_color = color_hit(accel, refraction_hit.value(), ray_depth + 1);
	    }

	    const vec3<F> reflection_direction = i - static_cast<F>(2.) * dot(i, n) * n;
	    const ray3<F> reflection_ray(hit_position + (static_cast<F>(reflection_bias) * reflection_direction), reflection_direction);
	    const auto reflection_hit = accel.template intersect<false>(reflection_ray);

	    color<F> reflection_color{};
	    if (reflection_hit.has_value()) {
		reflection_color = color_hit(accel, reflection_hit.value(), ray_depth + 1);
	    }

	    const F fresnel = 0.5 * std::pow(static_cast<F>(1.) + dot(i, n), 5);
	    return fresnel * reflection_color + (static_cast<F>(1.) - fresnel) * refraction_color;
	} else if constexpr (std::same_as<M, constant_material<F>>) {
	    return material.albedo;
	} else {
	    return color<F>{};
	}
    }, material_variant);
}

