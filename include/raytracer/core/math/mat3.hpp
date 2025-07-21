#pragma once

#include <array>

#include <raytracer/core/math/vec3.hpp>

template<typename F>
struct mat3 {
    std::array<F, 9> m;

    F& operator[](size_t row, size_t column) {
	return m[row * 3 + column];
    }

    const F& operator[](size_t row, size_t column) const {
	return m[row * 3 + column];
    }
};

template<typename F>
mat3<F> operator*(const mat3<F>& lhs, const mat3<F>& rhs) {
    mat3<F> res{};

    for(int i = 0; i < 3; ++i) {
	for(int j = 0; j < 3; ++j) {
	    for(int k = 0; k < 3; ++k) {
		res[i, j] += lhs[i, k] * rhs[k, j];
	    }
	}
    }

    return res;
}

template<typename F>
vec3<F> operator*(const vec3<F>& lhs, const mat3<F>& rhs) {
    return vec3<F>{
	lhs.x * rhs[0, 0] + lhs.y * rhs[1, 0] + lhs.z * rhs[2, 0],
	lhs.x * rhs[0, 1] + lhs.y * rhs[1, 1] + lhs.z * rhs[2, 1],
	lhs.x * rhs[0, 2] + lhs.y * rhs[1, 2] + lhs.z * rhs[2, 2],
    };
}
