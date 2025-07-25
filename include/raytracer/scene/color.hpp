#pragma once

template <typename F>
struct color {
    F red;
    F green;
    F blue;

    constexpr color<F>& operator+=(const color<F>& rhs) noexcept {
	red += rhs.red;
	green += rhs.green;
	blue += rhs.blue;
	return *this;
    }

    constexpr color<F>& operator/=(const F& rhs) noexcept {
	red /= rhs;
	green /= rhs;
	blue /= rhs;
	return *this;
    }
};

template <typename F>
color<F> operator+(const color<F>& lhs, const color<F>& rhs) noexcept {
    return {
	lhs.red + rhs.red,
	lhs.green + rhs.green,
	lhs.blue + rhs.blue
    };
}

template <typename F>
color<F> operator*(const F& lhs, const color<F>& rhs) noexcept {
    return {
	lhs * rhs.red,
	lhs * rhs.green,
	lhs * rhs.blue
    };
}

template <typename F>
color<F> operator/(const color<F>& lhs, const F& rhs) noexcept {
    return {
	lhs.red / rhs,
	lhs.green / rhs,
	lhs.blue / rhs
    };
}
