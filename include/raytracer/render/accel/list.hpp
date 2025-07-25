#pragma once

#include <memory>
#include <optional>

#include <raytracer/core/math/ray3.hpp>
#include <raytracer/core/math/aabb3.hpp>
#include <raytracer/scene/scene.hpp>

template <typename F, F eps>
struct list_accel {
    std::shared_ptr<const scene<F>> scene_ptr;
    aabb3<F> root_box;

    constexpr list_accel(std::shared_ptr<const scene<F>> scene_ptr) : scene_ptr(std::move(scene_ptr)) {
	for (const auto& mesh : this->scene_ptr->meshes) {
	    root_box.unite(mesh.box);
	}
    }

    template <bool backface_culling>
    constexpr std::optional<hit<F>> intersect(const ray3<F>& ray) const {
	std::optional<hit<F>> closest_hit;

	for(const auto& [mesh_idx, mesh] : scene_ptr->meshes | std::ranges::views::enumerate) {
	    if (!root_box.intersect(ray)) {
		continue;
	    }

	    const auto maybe_hit = mesh.template intersect<backface_culling, eps>(ray);

	    if (maybe_hit && (!closest_hit || maybe_hit->distance < closest_hit->distance)) {
		const F u = maybe_hit->u;
		const F v = maybe_hit->v;
		const F w = static_cast<F>(1.) - u - v;

		closest_hit = hit<F>{
		    ray,
		    maybe_hit->position,
		    maybe_hit->hit_normal,
		    maybe_hit->face_normal,
		    maybe_hit->uvs,
		    maybe_hit->distance,
		    u,
		    v,
		    w,
		    static_cast<std::size_t>(mesh_idx)
		};
	    }
	}

	return closest_hit;
    }
};
