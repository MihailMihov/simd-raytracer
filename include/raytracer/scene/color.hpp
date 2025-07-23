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
	std::min(lhs.red + rhs.red, static_cast<F>(1.)),
	std::min(lhs.green + rhs.green, static_cast<F>(1.)),
	std::min(lhs.blue + rhs.blue, static_cast<F>(1.))
    );
}

template <typename F>
color<F> operator*(const color<F>& lhs, const color<F>& rhs) {
    return color<F>(
	std::min(lhs.red * rhs.red, static_cast<F>(1.)),
	std::min(lhs.green * rhs.green, static_cast<F>(1.)),
	std::min(lhs.blue * rhs.blue, static_cast<F>(1.))
    );
}

template <typename F>
color<F> operator*(const F& lhs, const color<F>& rhs) {
    return color<F>(
	std::min(lhs * rhs.red, static_cast<F>(1.)),
	std::min(lhs * rhs.green, static_cast<F>(1.)),
	std::min(lhs * rhs.blue, static_cast<F>(1.))
    );
}

template <typename F>
color<F> operator*(const color<F>& lhs, const F& rhs) {
    return color<F>(
	std::min(lhs.red * rhs, static_cast<F>(1.)),
	std::min(lhs.green * rhs, static_cast<F>(1.)),
	std::min(lhs.blue * rhs, static_cast<F>(1.))
    );
}
