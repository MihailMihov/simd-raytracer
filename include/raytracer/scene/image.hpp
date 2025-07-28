#pragma once

#include <vector>

#include <raytracer/scene/color.hpp>

template <typename F>
struct image {
private:
    std::size_t height;
    std::size_t width;
    std::vector<std::vector<color<F>>> pixels;
public:
    constexpr image(std::size_t height, std::size_t width, std::vector<std::vector<color<F>>>&& pixels)
        : height(height), width(width), pixels(std::move(pixels)) {}

    constexpr image(std::size_t height, std::size_t width, const std::vector<std::vector<color<F>>>& pixels)
        : height(height), width(width), pixels(pixels) {}

    constexpr size_t get_height() const {
        return height;
    }

    constexpr size_t get_width() const {
        return width;
    }

    constexpr const color<F>& get_pixel(std::size_t row, std::size_t column) const {
        return pixels[row][column];
    }
};
