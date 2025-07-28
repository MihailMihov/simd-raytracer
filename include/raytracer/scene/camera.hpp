#pragma once

#include <numbers>

#include <raytracer/core/math/vec3.hpp>
#include <raytracer/core/math/mat3.hpp>

template <typename F>
struct camera {
    vec3<F> position;
    mat3<F> matrix;

    void translate(const vec3<F>& translation) {
        position = position + (translation * matrix);
    }

    void truck(const F distance) {
        translate({distance, 0, 0});
    }

    void pedestal(const F distance) {
        translate({0, distance, 0});
    }

    void dolly(const F distance) {
        translate({0, 0, distance});
    }

    void pan(const F degrees) {
        const F radians = degrees * (std::numbers::pi_v<F> / 180.);
        const F c = static_cast<F>(std::cos(radians));
        const F s = static_cast<F>(std::sin(radians));

        const mat3<F> rotate_around_y({
            { c, 0, s},
            { 0, 1, 0},
            {-s, 0, c}
        });
        matrix = rotate_around_y * matrix;
    }

    void tilt(const F degrees) {
        const F radians = degrees * (std::numbers::pi_v<F> / 180.);
        const F c = static_cast<F>(std::cos(radians));
        const F s = static_cast<F>(std::sin(radians));

        const mat3<F> rotate_around_x({
            {1, 0,  0},
            {0, c, -s},
            {0, s,  c}
        });
        matrix = rotate_around_x * matrix;
    }

    void roll(const F degrees) {
        const F radians = degrees * (std::numbers::pi_v<F> / 180.);
        const F c = static_cast<F>(std::cos(radians));
        const F s = static_cast<F>(std::sin(radians));

        const mat3<F> rotate_around_z({
            {c, -s, 0},
            {s,  c, 0},
            {0,  0, 1}
        });
        matrix = rotate_around_z * matrix;
    }
};
