#pragma once

#include <memory>
#include <stack>
#include <optional>
#include <cmath>

#include <experimental/simd>

#include <raytracer/core/math/aabb3.hpp>
#include <raytracer/scene/scene.hpp>

namespace stdx = std::experimental;

constexpr std::size_t max_depth_simd = 8;
constexpr std::size_t max_primitive_count_simd = 16;

template <typename F, std::size_t W>
struct triangle_pack {
    using simd_f = stdx::native_simd<F>;
    using simd_f_mask = typename simd_f::mask_type;
    
    simd_f v0x, v0y, v0z;
    simd_f e1x, e1y, e1z;
    simd_f e2x, e2y, e2z;
    std::array<std::size_t, W> triangle_indices;

    template <bool backface_culling>
    constexpr simd_f_mask intersect(const ray3<F>& ray, simd_f& u, simd_f& v, simd_f& t) const noexcept {
	const simd_f pvec_x = ray.direction.y * e2z - ray.direction.z * e2y;
	const simd_f pvec_y = ray.direction.z * e2x - ray.direction.x * e2z;
	const simd_f pvec_z = ray.direction.x * e2y - ray.direction.y * e2x;

	const simd_f det = e1x * pvec_x + e1y * pvec_y + e1z * pvec_z;

	simd_f_mask mask{true};
	if constexpr (backface_culling) {
	    mask = std::numeric_limits<F>::epsilon() <= det;
	} else {
	    mask = std::numeric_limits<F>::epsilon() <= stdx::abs(det);
	}

	const simd_f inv_det = static_cast<F>(1.) / det;

	const simd_f tvec_x{ray.origin.x - v0x};
	const simd_f tvec_y{ray.origin.y - v0y};
	const simd_f tvec_z{ray.origin.z - v0z};

	u = (tvec_x * pvec_x + tvec_y * pvec_y + tvec_z * pvec_z) * inv_det;
	mask &= (static_cast<F>(0.) <= u);
	mask &= (u <= static_cast<F>(1.));

	const simd_f qvec_x = tvec_y * e1z - tvec_z * e1y;
	const simd_f qvec_y = tvec_z * e1x - tvec_x * e1z;
	const simd_f qvec_z = tvec_x * e1y - tvec_y * e1x;

	v = (ray.direction.x * qvec_x + ray.direction.y * qvec_y + ray.direction.z * qvec_z) * inv_det;
	mask &= (static_cast<F>(0.) <= v);
	mask &= (u + v <= static_cast<F>(1.));

	t = (e2x * qvec_x + e2y * qvec_y + e2z * qvec_z) * inv_det;
	mask &= (std::numeric_limits<F>::epsilon() < t);

	return mask;
    }
};

template <typename F, std::size_t W = stdx::native_simd<F>::size()>
struct kd_tree_simd_accel {
    using simd_f = stdx::native_simd<F>;
    using simd_f_mask = typename simd_f::mask_type;

    struct kd_tree_node {
	aabb3<F> box;
	int32_t parent;
	int32_t child0;
	int32_t child1;
	std::size_t start_idx;
	std::size_t pack_count;
    };

    std::shared_ptr<const scene<F>> scene_ptr;
    std::vector<triangle<F>> triangles;
    std::vector<kd_tree_node> tree;
    std::vector<triangle_pack<F, W>> triangle_packs;

    kd_tree_simd_accel(std::shared_ptr<const scene<F>> scene_ptr) : scene_ptr(std::move(scene_ptr)) {
	aabb3<F> root_box;
	std::vector<std::size_t> triangle_indices;
	for (const auto& mesh : this->scene_ptr->meshes) {
	    root_box.unite(mesh.box);

	    std::size_t start_idx = triangles.size();
	    triangles.insert(triangles.end(), mesh.triangles.begin(), mesh.triangles.end());
	    for (std::size_t i = 0; i < mesh.triangles.size(); ++i) {
		triangle_indices.push_back(start_idx + i);
	    }
	}

	tree.emplace_back(root_box, -1, -1, -1, std::numeric_limits<std::size_t>::max(), 0);
	build_tree(0, 0, triangle_indices);
    }

    constexpr void build_tree(const int32_t parent_idx, const std::size_t depth, const std::vector<std::size_t>& triangle_indices) {
	if (depth == max_depth_simd || triangle_indices.size() <= max_primitive_count_simd) {
	    const std::size_t first_pack = triangle_packs.size();
	    
	    for (std::size_t i = 0; i < triangle_indices.size(); i += W) {
		triangle_pack<F, W> pack{};
		for (std::size_t lane = 0; lane < W; ++lane) {
		    const std::size_t triangle_idx = triangle_indices[std::min(i + lane, triangle_indices.size() - 1)];

		    const auto& triangle = triangles[triangle_idx];

		    pack.v0x[lane] = triangle.v0.x;
		    pack.v0y[lane] = triangle.v0.y;
		    pack.v0z[lane] = triangle.v0.z;
		    pack.e1x[lane] = triangle.e1.x;
		    pack.e1y[lane] = triangle.e1.y;
		    pack.e1z[lane] = triangle.e1.z;
		    pack.e2x[lane] = triangle.e2.x;
		    pack.e2y[lane] = triangle.e2.y;
		    pack.e2z[lane] = triangle.e2.z;
		    pack.triangle_indices[lane] = triangle_idx;
		}

		triangle_packs.push_back(pack);
	    }
	    
	    tree[parent_idx].start_idx = first_pack;
	    tree[parent_idx].pack_count = triangle_packs.size() - first_pack;

	    return;
	}

	auto [aabb0, aabb1] = tree[parent_idx].box.split(depth % 3);

	std::vector<std::size_t> child0_triangle_indices;
	child0_triangle_indices.reserve(triangle_indices.size());

	std::vector<std::size_t> child1_triangle_indices;
	child1_triangle_indices.reserve(triangle_indices.size());

	for (const auto& triangle_idx : triangle_indices) {
	    const auto& triangle = triangles[triangle_idx];

	    if (aabb0.intersect(triangle.box)) {
		child0_triangle_indices.push_back(triangle_idx);
	    }

	    if (aabb1.intersect(triangle.box)) {
		child1_triangle_indices.push_back(triangle_idx);
	    }
	}

	if (!child0_triangle_indices.empty()) {
	    std::size_t child0_idx = tree.size();
	    tree.emplace_back(aabb0, parent_idx, -1, -1, std::numeric_limits<std::size_t>::max(), 0);
	    tree[parent_idx].child0 = child0_idx;
	    build_tree(child0_idx, depth + 1, child0_triangle_indices);
	}

	if (!child1_triangle_indices.empty()) {
	    std::size_t child1_idx = tree.size();
	    tree.emplace_back(aabb1, parent_idx, -1, -1, std::numeric_limits<std::size_t>::max(), 0);
	    tree[parent_idx].child1 = child1_idx;
	    build_tree(child1_idx, depth + 1, child1_triangle_indices);
	}
    }

    template <bool backface_culling>
    constexpr std::optional<hit_record<F>> trace(const ray3<F>& ray) const {
	F best_t = std::numeric_limits<F>::max();
	F best_u = std::numeric_limits<F>::max();
	F best_v = std::numeric_limits<F>::max();
	std::size_t best_pack = std::numeric_limits<std::size_t>::max();
	std::size_t best_lane = std::numeric_limits<std::size_t>::max();

	std::stack<std::size_t, std::vector<std::size_t>> nodes_to_check;
	nodes_to_check.push(0);

	while (!nodes_to_check.empty()) {
	    const auto node_idx = nodes_to_check.top();
	    nodes_to_check.pop();

	    const auto& node = tree[node_idx];

	    auto maybe_box_hit = node.box.intersect(ray, best_t);
	    if (!maybe_box_hit || best_t <= maybe_box_hit.value()) {
		continue;
	    }

	    if (node.start_idx == std::numeric_limits<std::size_t>::max()) {
		if (node.child0 != -1) {
		    nodes_to_check.push(node.child0);
		}

		if (node.child1 != -1) {
		    nodes_to_check.push(node.child1);
		}
	    } else {
		for (std::size_t pack_idx = node.start_idx; pack_idx < node.start_idx + node.pack_count; ++pack_idx) {
		    const auto& pack = triangle_packs[pack_idx];

		    simd_f u, v, t;
		    simd_f_mask mask = pack.template intersect<backface_culling>(ray, u, v, t);

		    if (stdx::none_of(mask)) {
			continue;
		    }

		    stdx::where(!mask, t) = best_t;

		    const F t_min = stdx::hmin(t);
		    if (best_t <= t_min) {
			continue;
		    }

		    mask &= (t == t_min);

		    best_pack = pack_idx;
		    best_lane = stdx::find_first_set(mask);
		    best_u = u[best_lane];
		    best_v = v[best_lane];
		    best_t = t_min;
		}
	    }
	}

	if (best_t == std::numeric_limits<F>::max()) {
	    return std::nullopt;
	}

	const auto& pack = triangle_packs[best_pack];

	const F w = static_cast<F>(1.) - best_u - best_v;

	const std::size_t triangle_idx = pack.triangle_indices[best_lane];
	const auto& triangle = triangles[triangle_idx];

	const std::size_t mesh_idx = triangle.mesh_idx;
	const std::size_t v0_idx = triangle.vertex_indices[0];
	const std::size_t v1_idx = triangle.vertex_indices[1];
	const std::size_t v2_idx = triangle.vertex_indices[2];

	const auto& mesh = scene_ptr->meshes[mesh_idx];

	const vec3<F> hit_normal = norm(best_u * mesh.vertex_normals[v1_idx] + best_v * mesh.vertex_normals[v2_idx] + w * mesh.vertex_normals[v0_idx]);

	return hit_record<F>{
	    ray,
	    ray.origin + (best_t * ray.direction),
	    hit_normal,
	    triangle.normal,
	    triangle.uvs,
	    best_t,
	    best_u,
	    best_v,
	    w,
	    mesh_idx
	};
    }
};
