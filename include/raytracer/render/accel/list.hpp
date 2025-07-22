#pragma once

#include <memory>
#include <optional>

#include <raytracer/core/math/aabb3.hpp>
#include <raytracer/scene/scene.hpp>

template <typename F>
struct list_accel {
    std::shared_ptr<const scene<F>> scene_ptr;
    aabb3<F> root_box;

    constexpr list_accel(std::shared_ptr<const scene<F>> scene_ptr) : scene_ptr(std::move(scene_ptr)) {
	for (const auto& mesh : this->scene_ptr->meshes) {
	    root_box.unite(mesh.box);
	}
    }

    constexpr std::optional<hit_record<F>> trace(const ray3<F>& ray, const bool backface_culling, const F t_min, const F t_max) const {
	std::optional<hit_record<F>> closest_hit;

	for(const auto& [mesh_idx, mesh] : scene_ptr->meshes | std::ranges::views::enumerate) {
	    if (!root_box.intersect(ray)) {
		continue;
	    }

	    auto maybe_hit = mesh.intersect(ray, backface_culling, t_min, t_max);

	    if (maybe_hit && (!closest_hit || maybe_hit->distance < closest_hit->distance)) {
		closest_hit = hit_record<F>{
		    maybe_hit->ray,
		    maybe_hit->position,
		    maybe_hit->hit_normal,
		    maybe_hit->face_normal,
		    maybe_hit->uvs,
		    maybe_hit->distance,
		    maybe_hit->u,
		    maybe_hit->v,
		    static_cast<std::size_t>(mesh_idx)
		};
	    }
	}

	return closest_hit;
    }
};
