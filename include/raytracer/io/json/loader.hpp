#pragma once

#include <filesystem>

#include <simdjson.h>

#include <raytracer/scene/scene.hpp>

template <typename F>
vec3<F> load_vec3(simdjson::dom::array&& arr) {
    return vec3<F>{
	arr.at(0),
	arr.at(1),
	arr.at(2)
    };
}

template <typename F>
mat3<F> load_mat3(simdjson::dom::array&& arr) {
    return mat3<F>{
	arr.at(0), arr.at(1), arr.at(2),
	arr.at(3), arr.at(4), arr.at(5),
	arr.at(6), arr.at(7), arr.at(8)
    };
}

template <typename F>
color<F> load_color(simdjson::dom::array&& arr) {
    return color<F>{
	arr.at(0),
	arr.at(1),
	arr.at(2)
    };
}

template <typename F>
settings<F> load_settings(simdjson::dom::object&& obj) {
    std::size_t bucket_size = 64;

    if(auto bucket_size_json = obj["image_settings"]["bucket_size"].get_uint64(); !bucket_size_json.error()) {
	bucket_size = bucket_size_json.value();
    }

    return settings<F>{
	load_color<F>(obj["background_color"]),
	obj["image_settings"]["height"],
	obj["image_settings"]["width"],
	bucket_size
    };
}

template <typename F>
camera<F> load_camera(simdjson::dom::object&& obj) {
    return camera<F>{
	load_vec3<F>(obj["position"]),
	load_mat3<F>(obj["matrix"])
    };
}

template <typename F>
light<F> load_light(simdjson::dom::object&& obj) {
    return light<F>{
	load_vec3<F>(obj["position"]),
	obj["intensity"]
    };
}

template <typename F>
texture_variant<F> load_texture(simdjson::dom::object&& obj) {
    std::string_view type = obj["type"];

    if(type == "albedo") {
	return albedo_texture<F>{
	    load_color<F>(obj["albedo"])
	};
    } else if(type == "edges") {
	return edge_texture<F>{
	    load_color<F>(obj["edge_color"]),
	    load_color<F>(obj["inner_color"]),
	    obj["edge_width"]
	};
    } else if(type == "checker") {
	return checker_texture<F>{
	    load_color<F>(obj["color_A"]),
	    load_color<F>(obj["color_B"]),
	    obj["square_size"]
	};
    } else if(type == "bitmap") {
	std::string_view file_path = obj["file_path"];
	return bitmap_texture<F>{
	    std::string{file_path}
	};
    } else {
	throw std::invalid_argument("texture type unknown");
    }
}

template <typename F>
material_variant<F> load_material(simdjson::dom::object&& obj) {
    std::string_view type = obj["type"];

    if(type == "diffuse") {
	simdjson::dom::element albedo = obj["albedo"];

	if(albedo.type() == simdjson::dom::element_type::ARRAY) {
	    return diffuse_material<F>{
		load_color<F>(obj["albedo"]),
		obj["smooth_shading"]
	    };
	} else if(albedo.type() == simdjson::dom::element_type::STRING) {
	    std::string_view texture_name = obj["albedo"];
	    return texture_material<F>{
		std::string{texture_name},
		obj["smooth_shading"]
	    };
	} else {
	    throw std::invalid_argument("albedo neither array nor string");
	}
    } else if(type == "reflective") {
	return reflective_material<F>{
	    load_color<F>(obj["albedo"]),
	    obj["smooth_shading"]
	};
    } else if(type == "refractive") {
	return refractive_material<F>{
	    obj["ior"],
	    obj["smooth_shading"]
	};
    } else if(type == "constant") {
	return constant_material<F>{
	    load_color<F>(obj["albedo"]),
	    obj["smooth_shading"]
	};
    } else {
	throw std::invalid_argument("material type unknown");
    }
}

template <typename F>
mesh_object<F> load_mesh(simdjson::dom::object&& obj, std::size_t object_idx) {
    std::size_t material_index = obj["material_index"];

    std::vector<vec3<F>> vertices;
    std::vector<F> vertex_buffer;
    for(auto vertex : obj["vertices"]) {
	vertex_buffer.emplace_back(vertex);

	if(vertex_buffer.size() == 3) {
	    vertices.emplace_back(vec3<F>{
		vertex_buffer[0],
		vertex_buffer[1],
		vertex_buffer[2]
	    });

	    vertex_buffer.clear();
	}
    }

    if(!vertex_buffer.empty()) {
	throw std::invalid_argument("vertex coordinates not multiple of 3");
    }

    std::vector<vec2<F>> uvs;
    if(auto uv_array = obj["uvs"].get_array(); !uv_array.error()) {
	std::vector<F> uv_buffer;
	for(auto uv : uv_array) {
	    uv_buffer.emplace_back(uv);

	    if(uv_buffer.size() == 3) {
		uvs.emplace_back(vec2<F>{
		    uv_buffer[0],
		    uv_buffer[1]
		});

		uv_buffer.clear();
	    }
	}

	if(!uv_buffer.empty()) {
	    throw std::invalid_argument("uv coordinates not multiple of 3");
	}
    }

    std::vector<triangle<F>> triangles;
    std::vector<std::size_t> triangle_buffer;
    for(auto triangle_index : obj["triangles"]) {
	triangle_buffer.push_back(triangle_index);

	if(triangle_buffer.size() == 3) {
	    vec3<vec2<F>> triangle_uvs{};

	    if (!uvs.empty()) {
		triangle_uvs = vec3<vec2<F>>{
		    uvs[triangle_buffer[0]],
		    uvs[triangle_buffer[1]],
		    uvs[triangle_buffer[2]]
		};
	    }

	    triangles.push_back(triangle<F>{
		vertices[triangle_buffer[0]],
		vertices[triangle_buffer[1]],
		vertices[triangle_buffer[2]],
		{triangle_buffer[0], triangle_buffer[1], triangle_buffer[2]},
		object_idx,
		triangle_uvs 
	    });

	    triangle_buffer.clear();
	}
    }

    if(!triangle_buffer.empty()) {
	throw std::invalid_argument("triangle indices not multiple of 3");
    }

    return mesh_object<F>{
	material_index,
	vertices,
	uvs,
	triangles
    };
}

template <typename F>
scene<F> parse_scene_file(const std::filesystem::path& path) {
    simdjson::dom::parser parser;
    simdjson::padded_string json = simdjson::padded_string::load(path.string());
    auto doc = parser.parse(json);

    scene<F> scene{};

    scene.config = load_settings<F>(doc["settings"]);
    scene.viewpoint = load_camera<F>(doc["camera"]);

    for(auto light : doc["lights"]) {
	scene.lights.push_back(load_light<F>(light));
    }

    if(auto textures = doc["textures"].get_array(); !textures.error()) {
	for(auto texture : textures) {
	    scene.textures.emplace(texture["name"], load_texture<F>(texture));
	}
    }

    for(auto material : doc["materials"]) {
	scene.materials.emplace_back(load_material<F>(material));
    }

    for(auto [object_idx, object] : doc["objects"] | std::ranges::views::enumerate) {
	scene.meshes.emplace_back(load_mesh<F>(object, object_idx));
    }

    return scene;
}
