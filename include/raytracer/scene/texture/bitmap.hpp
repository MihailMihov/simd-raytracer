#pragma once

#include <string>

#include <stb_image.h>

#include <raytracer/scene/color.hpp>
#include <raytracer/scene/image.hpp>
#include <raytracer/render/hit.hpp>

template <typename F>
image<F> load_bitmap(const std::string& file_path) {
    std::vector<std::vector<color<F>>> buffer;
    int32_t width, height, channels_in_file;
    unsigned char* data = stbi_load(file_path.c_str(), &width, &height, &channels_in_file, 0);

    buffer.resize(height);

    const auto color_scale = F(1.0 / 255.0);

    for (int j = 0; j < height; ++j) {
        buffer[j].resize(width);
        for (int i = 0; i < width; ++i) {
            const auto pixel_offset = (j * width + i) * channels_in_file;

            auto r = static_cast<F>(data[pixel_offset + 0]) * color_scale;
            auto g = static_cast<F>(data[pixel_offset + 1]) * color_scale;
            auto b = static_cast<F>(data[pixel_offset + 2]) * color_scale;

            buffer[j][i] = color<F>(r, g, b);
        }
    }

    stbi_image_free(data);

    return image<F>(height, width, buffer);
}

template <typename F>
struct bitmap_texture {
    image<F> texture;
    
    bitmap_texture(const std::string& file_path)
        : texture(load_bitmap<F>(file_path)) {}

    constexpr color<F> sample(const hit<F>& hit, const vec3<vec2<F>>& uvs) const {
        F hit_u = hit.u;
        F hit_v = hit.v;
        F hit_w = 1. - hit_u - hit_v;

        vec2<F> final_uv = hit_w * uvs.x + hit_u * uvs.y + hit_v * uvs.z;

        std::size_t row = (1. - final_uv.y) * texture.get_height();
        std::size_t column = final_uv.x * texture.get_width();

        row = std::clamp(row, 0uz, texture.get_height() - 1);
        column = std::clamp(column, 0uz, texture.get_width() - 1);

        return texture.get_pixel(row, column);
    }
};
