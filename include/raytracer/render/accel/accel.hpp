#pragma once

#include <variant>

#include <raytracer/render/accel/list.hpp>
#include <raytracer/render/accel/kd_tree.hpp>

template <typename F>
using accel_variant = std::variant<list_accel<F>, kd_tree_accel<F>>;

template <typename A, typename F>
concept accelerator = requires(const A& accel, const ray3<F>& ray, bool backface_culling, F t_min, F t_max) {
    { accel.trace(ray, backface_culling, t_min, t_max) } -> std::same_as<std::optional<hit_record<F>>>;
};

template <typename A, typename F>
requires accelerator<A, F>
accel_variant<F> build_accel(std::shared_ptr<const scene<F>> scene_ptr) {
    if constexpr (std::same_as<A, list_accel<F>>) {
	return A(scene_ptr);
    } else {
	return A(scene_ptr);
    }
}
