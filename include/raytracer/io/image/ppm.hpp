#pragma once

#include <ostream>

#include <raytracer/scene/image.hpp>

template <typename F>
void write_ppm(const image<F>& img, std::ostream& out) {
    out << "P3\n";
    out << img.get_width() << " " << img.get_height() << "\n";
    out << "255\n";

    for(std::size_t row_idx = 0; row_idx < img.get_height(); ++row_idx) {
	for(std::size_t col_idx = 0; col_idx < img.get_width(); ++col_idx) {
	    auto color = img.get_pixel(row_idx, col_idx);

	    uint16_t red = static_cast<uint8_t>(255.999 * std::clamp(color.red, static_cast<F>(0.), static_cast<F>(1.)));
	    uint16_t green = static_cast<uint8_t>(255.999 * std::clamp(color.green, static_cast<F>(0.), static_cast<F>(1.)));
	    uint16_t blue = static_cast<uint8_t>(255.999 * std::clamp(color.blue, static_cast<F>(0.), static_cast<F>(1.)));

	    out << red << ' ' << green << ' ' << blue << '\t';
	}
	out << '\n';
    }
}
