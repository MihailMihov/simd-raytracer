#pragma once

#include <memory>
#include <optional>

#include <raytracer/core/math/aabb3.hpp>
#include <raytracer/scene/scene.hpp>
#include <raytracer/scene/object/object_queries.hpp>

template <typename F>
struct list_accel {
    std::shared_ptr<const scene<F>> scene_ptr;
    aabb3<F> root_box;

    list_accel(std::shared_ptr<const scene<F>> scene_ptr) : scene_ptr(std::move(scene_ptr)) {
	for (const auto& object_variant : this->scene_ptr->objects) {
	    root_box.unite(bounding_box_of(object_variant));
	}
    }

    constexpr std::optional<hit_record<F>> trace(const ray3<F>& ray, const bool backface_culling, const F t_min, const F t_max) const {
	std::optional<hit_record<F>> closest_hit;

	for(const auto& [object_idx, object_variant] : scene_ptr->objects | std::ranges::views::enumerate) {
	    if (!root_box.intersect(ray)) {
		continue;
	    }

	    auto object_hit = std::visit([&](const auto& object) {
		return object.intersect(ray, backface_culling, t_min, t_max);
	    }, object_variant);

	    if (object_hit && (!closest_hit || object_hit->distance < closest_hit->distance)) {
		closest_hit = hit_record<F>{
		    object_hit->ray,
		    object_hit->position,
		    object_hit->normal,
		    object_hit->distance,
		    object_hit->u,
		    object_hit->v,
		    static_cast<std::size_t>(object_idx),
		    object_hit->triangle_idx
		};
	    }
	}

	return closest_hit;
    }
};
