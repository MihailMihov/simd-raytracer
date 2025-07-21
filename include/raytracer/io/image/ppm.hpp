#pragma once

#include <ostream>

#include <raytracer/scene/image.hpp>

template <typename F>
void write_ppm(const image<F>& img, std::ostream& out) {
    out << "P3\n";
    out << img.get_width() << " " << img.get_height() << "\n";
    out << "255\n";

    for(int row_idx = 0; row_idx < img.get_height(); ++row_idx) {
	for(int col_idx = 0; col_idx < img.get_width(); ++col_idx) {
	    auto color = img.get_pixel(row_idx, col_idx);

	    int red = static_cast<uint8_t>(255.999 * color.red);
	    int green = static_cast<uint8_t>(255.999 * color.green);
	    int blue = static_cast<uint8_t>(255.999 * color.blue);

	    out << red << ' ' << green << ' ' << blue << '\t';
	}
	out << '\n';
    }
}
