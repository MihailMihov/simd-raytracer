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

    constexpr aabb3() :
	min(std::numeric_limits<F>::max(), std::numeric_limits<F>::max(), std::numeric_limits<F>::max()),
	max(std::numeric_limits<F>::min(), std::numeric_limits<F>::min(), std::numeric_limits<F>::min()) {}

    constexpr void expand(const vec3<F>& point) noexcept {
	min = { std::min(min.x, point.x), std::min(min.y, point.y), std::min(min.z, point.z) };
	max = { std::max(max.x, point.x), std::max(max.y, point.y), std::max(max.z, point.z) };
    }

    constexpr void unite(const aabb3<F>& other) noexcept {
	min = { std::min(min.x, other.min.x), std::min(min.y, other.min.y), std::min(min.z, other.min.z) };
	max = { std::max(max.x, other.max.x), std::max(max.y, other.max.y), std::max(max.z, other.max.z) };
    }

    [[nodiscard]] constexpr std::pair<aabb3<F>, aabb3<F>> split(const uint32_t axis) const noexcept {
	if (axis == 0) {
	    if (min.x == max.x) {
		return split(1);
	    }

	    const F mid = min.x + ((max.x - min.x) / 2.);

	    aabb3<F> aabb0(*this);
	    aabb3<F> aabb1(*this);

	    aabb0.max.x = mid;
	    aabb1.min.x = mid;

	    return std::make_pair(aabb0, aabb1);
	} else if (axis == 1) {
	    if (min.y == max.y) {
		return split(2);
	    }

	    const F mid = min.y + ((max.y - min.y) / 2.);

	    aabb3<F> aabb0(*this);
	    aabb3<F> aabb1(*this);

	    aabb0.max.y = mid;
	    aabb1.min.y = mid;

	    return std::make_pair(aabb0, aabb1);
	} else {
	    if (min.z == max.z) {
		return split(0);
	    }

	    const F mid = min.z + ((max.z - min.z) / 2.);

	    aabb3<F> aabb0(*this);
	    aabb3<F> aabb1(*this);

	    aabb0.max.z = mid;
	    aabb1.min.z = mid;

	    return std::make_pair(aabb0, aabb1);
	}
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
	F tmin = (min.x - ray.origin.x) / ray.direction.x;
	F tmax = (max.x - ray.origin.x) / ray.direction.x;
	if(tmax < tmin) {
	    std::swap(tmin, tmax);
	}

	F tymin = (min.y - ray.origin.y) / ray.direction.y;
	F tymax = (max.y - ray.origin.y) / ray.direction.y;
	if(tymax < tymin) {
	    std::swap(tymin, tymax);
	}

	if((tymax < tmin) || (tmax < tymin)) {
	    return false;
	}

	tmin = std::max(tmin, tymin);
	tmax = std::min(tmax, tymax);

	F tzmin = (min.z - ray.origin.z) / ray.direction.z;
	F tzmax = (max.z - ray.origin.z) / ray.direction.z;
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
