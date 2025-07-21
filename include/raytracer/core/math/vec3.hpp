#pragma once

#include <cmath>

template <typename F>
struct vec3 {
    F x, y, z;

    constexpr vec3<F> operator-() const {
	return vec3<F>{-x, -y, -z};
    }

    constexpr vec3<F>& operator+=(const vec3<F>& other) {
	x += other.x;
	y += other.y;
	z += other.z;

	return *this;
    }

    constexpr vec3<F>& operator-=(const vec3<F>& other) {
	x -= other.x;
	y -= other.y;
	z -= other.z;

	return *this;
    }

    constexpr vec3<F> operator+(const vec3<F>& rhs) const {
	return {x + rhs.x, y + rhs.y, z + rhs.z};
    }

    constexpr vec3<F> operator-(const vec3<F>& rhs) const {
	return {x - rhs.x, y - rhs.y, z - rhs.z};
    }

    constexpr F len_squared() const {
	return x * x + y * y + z * z;
    }

    constexpr F len() const {
	return std::sqrt(len_squared());
    }

    constexpr vec3<F> norm() const {
	return vec3<F>{x / len(), y / len(), z / len()};
    }
};

template <typename F>
constexpr vec3<F> operator*(const F& lhs, const vec3<F>& rhs) {
    return {
	lhs * rhs.x,
	lhs * rhs.y,
	lhs * rhs.z
    };
}

template <typename F>
constexpr vec3<F> operator*(const vec3<F>& lhs, const F& rhs) {
    return {
	lhs.x * rhs,
	lhs.y * rhs,
	lhs.z * rhs
    };
}

template <typename F>
constexpr vec3<F> cross(const vec3<F>& lhs, const vec3<F>& rhs) {
    return {
	lhs.y * rhs.z - lhs.z * rhs.y,
	lhs.z * rhs.x - lhs.x * rhs.z,
	lhs.x * rhs.y - lhs.y * rhs.x
    };
}

template <typename F>
constexpr F dot(const vec3<F>& lhs, const vec3<F>& rhs) {
    return (lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z * rhs.z);
}
