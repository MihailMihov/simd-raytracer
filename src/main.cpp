#include <filesystem>
#include <fstream>
#include <print>
#include <iostream>

#include <raytracer/io/image/ppm.hpp>
#include <raytracer/io/json/loader.hpp>
#include <raytracer/scene/scene.hpp>
#include <raytracer/render/render.hpp>
#include <raytracer/render/accel/accel.hpp>

int main(int argc, char **argv) {
    if(argc != 2) {
	std::println("Usage: ./raytracer FILE");

	return 1;
    }

    const std::filesystem::path& scene_file_path = argv[1];

    const auto scene = parse_scene_file<double>(scene_file_path);

    auto accel = build_accel<kd_tree_accel<double>, double>(std::make_shared<decltype(scene)>(scene));

    auto image = render_scene(accel, scheduling_type::BUCKET_TILES);

    std::ofstream output_file_stream("image.ppm", std::ios::out | std::ios::binary);
    write_ppm(image, output_file_stream);

    return 0;
}
