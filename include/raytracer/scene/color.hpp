#pragma once

#include <algorithm>
template <typename F>
struct color {
    F red;
    F green;
    F blue;
};

template <typename F>
color<F> operator+(const color<F>& lhs, const color<F>& rhs) {
    return color<F>(
	std::min(lhs.red + rhs.red, 1.),
	std::min(lhs.green + rhs.green, 1.),
	std::min(lhs.blue + rhs.blue, 1.)
    );
}

template <typename F>
color<F> operator*(const color<F>& lhs, const color<F>& rhs) {
    return color<F>(
	std::min(lhs.red * rhs.red, 1.),
	std::min(lhs.green * rhs.green, 1.),
	std::min(lhs.blue * rhs.blue, 1.)
    );
}

template <typename F>
color<F> operator*(const F& lhs, const color<F>& rhs) {
    return color<F>(
	std::min(lhs * rhs.red, 1.),
	std::min(lhs * rhs.green, 1.),
	std::min(lhs * rhs.blue, 1.)
    );
}

template <typename F>
color<F> operator*(const color<F>& lhs, const F& rhs) {
    return color<F>(
	std::min(lhs.red * rhs, 1.),
	std::min(lhs.green * rhs, 1.),
	std::min(lhs.blue * rhs, 1.)
    );
}
