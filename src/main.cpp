#include <filesystem>
#include <fstream>
#include <print>
#include <iostream>

#include <raytracer/config.hpp>
#include <raytracer/io/image/ppm.hpp>
#include <raytracer/io/json/loader.hpp>
#include <raytracer/scene/scene.hpp>
#include <raytracer/render/render.hpp>
#include <raytracer/render/accel/kd_tree_simd.hpp>

template <typename A, typename F>
void render_still(const A& accel)
requires accelerator<A, F> {
    auto render_start = std::chrono::high_resolution_clock::now();
    auto image = render_frame<A, F>(accel, scheduling_type::BUCKET_TILES);
    auto render_end = std::chrono::high_resolution_clock::now();

    auto duration = duration_cast<std::chrono::milliseconds>(render_end - render_start);
    std::println("Rendering took {} seconds.", duration.count() / 1'000.);

    std::ofstream output_file_stream("image.ppm", std::ios::out | std::ios::binary);
    write_ppm(image, output_file_stream);
}

int main(int argc, char **argv) {
    if (argc != 2) {
	std::println("Usage: ./raytracer FILE");

	return 1;
    }

    const std::filesystem::path& scene_file_path = argv[1];

    using F = float;
    using accel_t = kd_tree_simd_accel<float_t, static_cast<F>(epsilon)>;

    const auto scene = parse_scene_file<F>(scene_file_path);

    auto accelerator = accel_t(std::make_shared<decltype(scene)>(scene));

    render_still<decltype(accelerator), float_t>(accelerator);

    return 0;
}
