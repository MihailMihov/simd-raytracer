#pragma once

#include <cstdint>
#include <limits>
#include <algorithm>

#include <raytracer/core/math/vec3.hpp>
#include <raytracer/core/math/ray3.hpp>

template <typename F>
struct aabb3 {
    vec3<F> min;
    vec3<F> max;

    constexpr aabb3() noexcept
	: min(std::numeric_limits<F>::max(), std::numeric_limits<F>::max(), std::numeric_limits<F>::max()),
	  max(std::numeric_limits<F>::min(), std::numeric_limits<F>::min(), std::numeric_limits<F>::min()) {}

    constexpr void expand(const vec3<F>& point) noexcept {
	min.x = std::min(min.x, point.x);
	min.y = std::min(min.y, point.y);
	min.z = std::min(min.z, point.z);
	max.x = std::max(max.x, point.x);
	max.y = std::max(max.y, point.y);
	max.z = std::max(max.z, point.z);
    }

    constexpr void unite(const aabb3<F>& other) noexcept {
	min.x = std::min(min.x, other.min.x);
	min.y = std::min(min.y, other.min.y);
	min.z = std::min(min.z, other.min.z);
	max.x = std::max(max.x, other.max.x);
	max.y = std::max(max.y, other.max.y);
	max.z = std::max(max.z, other.max.z);
    }

    [[nodiscard]] constexpr std::pair<aabb3<F>, aabb3<F>> split(const uint32_t axis) const noexcept {
	assert(axis == 0 || axis == 1 || axis == 2);
	[[assume(axis == 0 || axis == 1 || axis == 2)]];

	if (min[axis] == max[axis]) {
	    return split((axis + 1u) % 3u);
	}

	const F mid = min[axis] + ((max[axis] - min[axis]) / static_cast<F>(2.));

	aabb3<F> aabb0(*this);
	aabb3<F> aabb1(*this);

	aabb0.max[axis] = mid;
	aabb1.min[axis] = mid;

	return std::make_pair(aabb0, aabb1);
    }

    [[nodiscard]] constexpr bool contains(const vec3<F>& point) const noexcept {
	return (min.x <= point.x && point.x <= max.x) ||
	       (min.y <= point.y && point.y <= max.y) ||
	       (min.z <= point.z && point.z <= max.z);
    }

    [[nodiscard]] constexpr bool intersect(const aabb3<F>& other) const noexcept {
	return (other.min.x < max.x && min.x <= other.max.x) &&
	       (other.min.y < max.y && min.y <= other.max.y) &&
	       (other.min.z < max.z && min.z <= other.max.z);
    }

    [[nodiscard]] constexpr bool intersect(const ray3<F>& ray) const noexcept {
	F t_min = static_cast<F>(0.);
	F t_max = std::numeric_limits<F>::max();

	for (std::size_t axis = 0; axis < 3; ++axis) {
	    const F t1 = (min[axis] - ray.origin[axis]) * ray.inv_direction[axis];
	    const F t2 = (max[axis] - ray.origin[axis]) * ray.inv_direction[axis];

	    t_min = std::min(std::max(t1, t_min), std::max(t2, t_min));
	    t_max = std::max(std::min(t1, t_max), std::min(t2, t_max));
	}

	return t_min <= t_max;
    }

    [[nodiscard]] constexpr std::optional<F> intersect(const ray3<F>& ray, F t_max) const noexcept {
	F t_min = static_cast<F>(0.);

	for (std::size_t axis = 0; axis < 3; ++axis) {
	    const F t1 = (min[axis] - ray.origin[axis]) * ray.inv_direction[axis];
	    const F t2 = (max[axis] - ray.origin[axis]) * ray.inv_direction[axis];

	    t_min = std::min(std::max(t1, t_min), std::max(t2, t_min));
	    t_max = std::max(std::min(t1, t_max), std::min(t2, t_max));
	}

	if (t_min <= t_max) {
	    return t_min;
	}

	return std::nullopt;
    }
};
