#pragma once

#include <array>

#include <raytracer/core/math/vec3.hpp>

template <typename F>
struct mat3 {
    std::array<F, 9> m;

    constexpr mat3() noexcept {}
    constexpr mat3(const std::array<F, 9>& m) noexcept : m(m) {}
    constexpr mat3(std::array<F, 9>&& m) noexcept : m(m) {}
    constexpr mat3(const vec3<F>& x, const vec3<F>& y, const vec3<F>& z) noexcept
        : m({x.x, x.y, x.z,
             y.x, y.y, y.z,
             z.x, z.y, z.z})
    {}

    F& operator[](size_t row, size_t column) {
        return m[row * 3 + column];
    }

    const F& operator[](size_t row, size_t column) const {
        return m[row * 3 + column];
    }
};

template <typename F>
mat3<F> transpose(const mat3<F>& m) noexcept {
    return {{
        m[0, 0], m[1, 0], m[2, 0],
        m[0, 1], m[1, 1], m[2, 1],
        m[0, 2], m[1, 2], m[2, 2]
    }};
}

template <typename F>
mat3<F> operator*(const mat3<F>& lhs, const mat3<F>& rhs) {
    mat3<F> res{};

    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            for (int k = 0; k < 3; ++k) {
                res[i, j] += lhs[i, k] * rhs[k, j];
            }
        }
    }

    return res;
}

template <typename F>
vec3<F> operator*(const mat3<F>& lhs, const vec3<F>& rhs) {
    return {
        lhs[0, 0] * rhs.x + lhs[0, 1] * rhs.y + lhs[0, 2] * rhs.z,
        lhs[1, 0] * rhs.x + lhs[1, 1] * rhs.y + lhs[1, 2] * rhs.z,
        lhs[2, 0] * rhs.x + lhs[2, 1] * rhs.y + lhs[2, 2] * rhs.z
    };
}
