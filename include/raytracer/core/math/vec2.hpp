#pragma once

#include <cmath>

template <typename F>
struct vec2 {
    F x, y;

    constexpr vec2<F> operator-() const {
	return vec2<F>{-x, -y};
    }

    constexpr vec2<F>& operator+=(const vec2<F>& other) {
	x += other.x;
	y += other.y;

	return *this;
    }

    constexpr vec2<F>& operator-=(const vec2<F>& other) {
	x -= other.x;
	y -= other.y;

	return *this;
    }

    constexpr vec2<F> operator+(const vec2<F>& rhs) const {
	return {x + rhs.x, y + rhs.y};
    }

    constexpr vec2<F> operator-(const vec2<F>& rhs) const {
	return {x - rhs.x, y - rhs.y};
    }

    constexpr F len_squared() const {
	return x * x + y * y;
    }

    constexpr F len() const {
	return std::sqrt(len_squared());
    }

    constexpr vec2<F> norm() const {
	return vec2<F>{x / len(), y / len()};
    }
};

template <typename F>
constexpr vec2<F> operator*(const F& lhs, const vec2<F>& rhs) {
    return {
	lhs * rhs.x,
	lhs * rhs.y
    };
}

template <typename F>
constexpr vec2<F> operator*(const vec2<F>& lhs, const F& rhs) {
    return {
	lhs.x * rhs,
	lhs.y * rhs
    };
}

template <typename F>
constexpr F dot(const vec2<F>& lhs, const vec2<F>& rhs) {
    return (lhs.x * rhs.x) + (lhs.y * rhs.y);
}
