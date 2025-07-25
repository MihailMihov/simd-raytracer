#pragma once

#include <cassert>
#include <cmath>
#include <utility>

template <typename F>
struct vec3 {
    F x, y, z;

    [[nodiscard]] constexpr F operator[](const std::size_t idx) const noexcept {
	assert(idx == 0 || idx == 1 || idx == 2);
	[[assume(idx == 0 || idx == 1 || idx == 2)]];

	switch (idx) {
	    case 0:
		return x;
	    case 1:
		return y;
	    case 2:
		return z;
	    default:
		std::unreachable();
	};
    }

    [[nodiscard]] constexpr F& operator[](const std::size_t idx) noexcept {
	assert(idx == 0 || idx == 1 || idx == 2);
	[[assume(idx == 0 || idx == 1 || idx == 2)]];

	switch (idx) {
	    case 0:
		return x;
	    case 1:
		return y;
	    case 2:
		return z;
	    default:
		std::unreachable();
	};
    }

    [[nodiscard]] constexpr vec3<F> operator-() const noexcept {
	return {-x, -y, -z};
    }

    constexpr vec3<F>& operator+=(const vec3<F>& other) noexcept {
	x += other.x;
	y += other.y;
	z += other.z;
	return *this;
    }

    constexpr vec3<F>& operator-=(const vec3<F>& other) noexcept {
	x -= other.x;
	y -= other.y;
	z -= other.z;
	return *this;
    }

    constexpr vec3<F>& operator*=(const F scalar) noexcept {
	x *= scalar;
	y *= scalar;
	z *= scalar;
	return *this;
    }

    constexpr vec3<F>& operator/=(const F scalar) noexcept {
	assert(scalar != 0);
	[[assume(scalar != 0)]];
	x /= scalar;
	y /= scalar;
	z /= scalar;
	return *this;
    }

    [[nodiscard]] constexpr vec3<F> operator+(const vec3<F>& rhs) const noexcept {
	return {x + rhs.x, y + rhs.y, z + rhs.z};
    }

    [[nodiscard]] constexpr vec3<F> operator-(const vec3<F>& rhs) const noexcept {
	return {x - rhs.x, y - rhs.y, z - rhs.z};
    }

    [[nodiscard]] constexpr F len_squared() const noexcept {
	return x * x + y * y + z * z;
    }

    [[nodiscard]] constexpr F len() const noexcept {
	return std::sqrt(len_squared());
    }
};

template <typename F>
[[nodiscard]] constexpr vec3<F> operator*(const F lhs, const vec3<F>& rhs) noexcept {
    return {lhs * rhs.x, lhs * rhs.y, lhs * rhs.z};
}

template <typename F>
[[nodiscard]] constexpr vec3<F> operator/(const F lhs, const vec3<F>& rhs) noexcept {
    return {lhs / rhs.x, lhs / rhs.y, lhs / rhs.z};
}

template <typename F>
[[nodiscard]] constexpr vec3<F> normalized(const vec3<F>& v) noexcept {
    const F inv_length = static_cast<F>(1.) / v.len();
    return {v.x * inv_length, v.y * inv_length, v.z * inv_length};
}

template <typename F>
[[nodiscard]] constexpr vec3<F> normalized(vec3<F>&& v) noexcept {
    const F inv_length = static_cast<F>(1.) / v.len();
    v.x *= inv_length;
    v.y *= inv_length;
    v.z *= inv_length;
    return std::move(v);
}

template <typename F>
[[nodiscard]] constexpr F dot(const vec3<F>& lhs, const vec3<F>& rhs) noexcept {
    return (lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z * rhs.z);
}

template <typename F>
[[nodiscard]] constexpr vec3<F> cross(const vec3<F>& lhs, const vec3<F>& rhs) noexcept {
    return {
	lhs.y * rhs.z - lhs.z * rhs.y,
	lhs.z * rhs.x - lhs.x * rhs.z,
	lhs.x * rhs.y - lhs.y * rhs.x
    };
}
