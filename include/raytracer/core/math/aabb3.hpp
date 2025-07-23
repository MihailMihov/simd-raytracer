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
	F tmin = (min[0] - ray.origin[0]) / ray.direction[0];
	F tmax = (max[0] - ray.origin[0]) / ray.direction[0];
	if(tmax < tmin) {
	    std::swap(tmin, tmax);
	}

	F tymin = (min[1] - ray.origin[1]) / ray.direction[1];
	F tymax = (max[1] - ray.origin[1]) / ray.direction[1];
	if(tymax < tymin) {
	    std::swap(tymin, tymax);
	}

	if((tymax < tmin) || (tmax < tymin)) {
	    return false;
	}

	tmin = std::max(tmin, tymin);
	tmax = std::min(tmax, tymax);

	F tzmin = (min[2] - ray.origin[2]) / ray.direction[2];
	F tzmax = (max[2] - ray.origin[2]) / ray.direction[2];
	if(tzmax < tzmin) {
	    std::swap(tzmin, tzmax);
	}

	if((tzmax < tmin) || (tmax < tzmin)) {
	    return false;
	}

	tmin = std::max(tmin, tzmin);
	tmax = std::min(tmax, tzmax);

	return true;
    }
};
