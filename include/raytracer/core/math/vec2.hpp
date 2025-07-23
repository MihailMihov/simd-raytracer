#pragma once

#include <cmath>

template <typename F>
struct vec2 {
    F x, y;

    constexpr vec2<F> operator-() const noexcept {
	return {-x, -y};
    }

    constexpr vec2<F>& operator+=(const vec2<F>& other) noexcept {
	x += other.x;
	y += other.y;
	return *this;
    }

    constexpr vec2<F>& operator-=(const vec2<F>& other) noexcept {
	x -= other.x;
	y -= other.y;
	return *this;
    }

    constexpr vec2<F>& operator*=(const F scalar) noexcept {
	x *= scalar;
	y *= scalar;
	return *this;
    }

    constexpr vec2<F>& operator/=(const F scalar) noexcept {
	x /= scalar;
	y /= scalar;
	return *this;
    }

    [[nodiscard]] constexpr vec2<F> operator+(const vec2<F>& rhs) const noexcept {
	return {x + rhs.x, y + rhs.y};
    }

    [[nodiscard]] constexpr vec2<F> operator-(const vec2<F>& rhs) const noexcept {
	return {x - rhs.x, y - rhs.y};
    }

    [[nodiscard]] constexpr F len_squared() const noexcept {
	return x * x + y * y;
    }

    [[nodiscard]] constexpr F len() const noexcept {
	return std::sqrt(len_squared());
    }

    constexpr void normalize() {
	const F inv_length = static_cast<F>(1.) / len();
	x *= inv_length;
	y *= inv_length;
    }
};

template <typename F>
[[nodiscard]] constexpr vec2<F> operator*(const F lhs, const vec2<F>& rhs) noexcept {
    return {lhs * rhs.x, lhs * rhs.y};
}

template <typename F>
[[nodiscard]] constexpr vec2<F> norm(const vec2<F>& v) {
    const F inv_length = static_cast<F>(1.) / v.len();
    return {v.x * inv_length, v.y * inv_length};
}

template <typename F>
[[nodiscard]] constexpr F dot(const vec2<F>& lhs, const vec2<F>& rhs) noexcept {
    return (lhs.x * rhs.x) + (lhs.y * rhs.y);
}
