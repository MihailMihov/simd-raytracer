#include <filesystem>
#include <fstream>
#include <print>
#include <iostream>

#include <raytracer/io/image/ppm.hpp>
#include <raytracer/io/json/loader.hpp>
#include <raytracer/scene/scene.hpp>
#include <raytracer/render/render.hpp>
#include <raytracer/render/accel/accel.hpp>

constexpr std::size_t total_frames = 200;

int main(int argc, char **argv) {
    if(argc != 2) {
	std::println("Usage: ./raytracer FILE");

	return 1;
    }

    const std::filesystem::path& scene_file_path = argv[1];

    const auto scene = parse_scene_file<double>(scene_file_path);

    auto accel_variant = build_accel<kd_tree_accel<double>, double>(std::make_shared<decltype(scene)>(scene));

    for (std::size_t frame = 1; frame <= total_frames; ++frame) {
	std::print("\rGenerating frame {} out of {}...", frame, total_frames);
	std::flush(std::cout);

	if (auto* accel = std::get_if<kd_tree_accel<double>>(&accel_variant)) {
	    accel->set_primitives_limit(frame * 25);
	};

	auto image = render_frame(accel_variant, scheduling_type::BUCKET_TILES);

	std::ofstream output_file_stream(std::format("output/frame_{:04d}.ppm", frame), std::ios::out | std::ios::binary);
	write_ppm(image, output_file_stream);
    }

    std::println("");

    return 0;
}
