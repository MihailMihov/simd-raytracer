simd-raytracer
---
`simd-raytracer` is a portable high-performance raytracer, written in C++23,
with support for reading `.crtscene` JSON scene files using `simdjson`, support
for different material and texture kinds and a basic global illumination
algorithm.

### Quick links

* [Quick start](#quick-start)
* [Examples](#examples)
* [Configuration](#configuration)
* [Acceleration structures](#acceleration-structures)
* [Materials](#materials)
* [Textures](#textures)

## Quick start

The project uses CMake as a build configuration system. The `CMakeLists.txt` in
the root directory of the project contains all the needed setup and it uses the
`FetchContent` module to fetch and setup the necessary libraries, `simdjson`
for JSON parsing and `stb` for reading the texture files.

On Linux setting up and running the project can be done as follows:
1. `git clone https://github.com/mihailmihov/simd-raytracer.git`
2. `cd raytracer`
3. `mkdir build`
4. `cmake -S . -B build -DCMAKE_BUILD_TYPE="Release"` [^1]
5. `cmake --build build`
6. `./build/raytracer scenes/hw15/scene2.crtscene`

The scenes in `scenes/`, which depend on texture files (currently only
`scenes/hw12/`) use relative paths starting from the project root directory, so
the raytracer should be run from the project root when those are used, or the
directories need to be changed accordingly.

[^1]: It is also strongly recommended to add
    `-DCMAKE_CXX_FLAGS="-march=native"` for GCC/Clang or `/arch:AVX2` for MSVC,
    if the machine supports a vector instruction set.

## Examples

The `outputs/` directory features some of the images produces by the raytracer
with various scenes and options.

The
`gi_{samples_per_pixel}spp_{max_ray_depth}_{diffuse_reflection_rays_count}.png`
are all from the `scenes/hw15/scene2.crtscene` scene, which features a box with
a semi-transparent sphere, a mettalic ball, an opaque cube and an opaque
cylinder. The sphere's shadows highlight how the diffuse reflections cause even
transparent objects to cause some kind of shadow and caustics.

The `dragon_slow_load.mp4` shows the dragon from `scenes/hw09/scene5.crtscene`,
which is diffuse and on top of a reflective material. I also used some ugly
code to "unmask" 10 triangles per frame (which was removed in some cleanup) and
then I stitched the frames together with `ffmpeg`. It took ~4 minutes to render
then, but now on the latest version a single frame is rendered in <100ms, even
with all triangles.

`refractive_dragon.png` shows the refractive dragon from
`scenes/hw11/scene8.crtscene`, which is inside a box with differently colored
walls and shows how the refractions pick up different colors.

`textures.png` shows `scenes/hw12/scene4.crtscene` which highlights all the
different types of textures (albedo, edge, checker and bitmap).

As the raytracer only supports `.ppm`, `ffmpeg -i image.ppm output.png` can be
used to convert the PPM files to the superior PNG file format, if they should
be stored in git or sent anywhere.

## Configuration

Configuration is currently still done with the `constexpr` variables in the
`include/raytracer/config.hpp` header file. The currently available options
are:
- `fov_degrees` sets the field-of-view of the camera (in degrees).
- `epsilon` sets the epsilon for the scene, used to ignore rounding errors.
- `shadow_bias` bias to offset shadow rays with.
- `reflection_bias` bias to offset reflection rays with.
- `refraction_bias` bias to offset refraction rays with.
- `samples_per_pixel` how many rays to average for each pixel in the image.
- `max_ray_depth` maximum recursion when shooting reflections and refractions.
- `diffuse_reflection_ray_count` how many reflection rays to shoot when a
  diffuse texture is hit.
- `fixed_rng_seed` seed to use for the RNG engine (currently only used to
  generate random offsets, when more than one sample per pixel is requested).

## Acceleration structures

Currently the supported acceleration structures are either a list
(`list_accel`) or two variants of a kd-tree (`kd_tree_accel` and
`kd_tree_simd_accel`). The list data does use a single scene AABB to optimize
some of the rays that wouldn't have hit anything, but for most rays it iterates
over all meshes and triangles in them to check for a hit and its distance. The
kd-tree has two variants that are algorithmically equivalent. They build a
scene AABB and then recursively split the AABB at the mid point over one of the
axes, cycling over them based on the current depth. A leaf node is created when
either a max depth or minimum of contained triangles is reached. The difference
between the kd-tree variants is that the `_simd` variant stores packets of
triangles in the leaf nodes, so that a single ray can be intersected with W
triangles at once, where W is dependant on the platform's SIMD capabilities and
on the floating point data type that is used.

Below is a table showing the
theoretical possible speedups, relative to a legacy machine with 32-bit `float`s:

| Level                     | Instruction Set   | `float` (32-bit) speedup | `double` (64-bit) speedup |
|---------------------------|-------------------|--------------------------|---------------------------|
| **Legacy**                | (no SIMD)         | 1x                       | 0.5x                      |
| **Consumer**              | AVX2 (256-bit)    | 8x                       | 4x                        |
| **HEDT/Professional**     | AVX512 (512-bit)  | 16x                      | 8x                        |

These are only the theoretical possible speedups, which are not obtained in
practice. On my consumer laptop with AVX2 I observed a ~5x speedup when using
`float`s, which is quite a bit less than the 8x, but still a massive speedup.
This can be explained in many ways, but mostly comes down to only the leaf
checking of the kd-tree being vectorized, but not the actual traversal of the
tree, which are the two main contributing factors to render time. Next up on my
to-do will be implementing a BVH4/8 structure (e.g. the ones used in Intel's
Embree raytracer), where a node has 4/8 children, which leads to shallower
trees compared to binary structures like a kd-tree, and the traversal should be
vectorizable. An octree might also be a good candidate as it also has 8
children per node, when used in 3D, but according to my limited research a BVH
performs much better in most cases.

Additionally the SIMD implementation is fully portable, because it is based on
the experimental parallelism technical specification v2 (will become part of
ISO C++ with C++26). The data-parallel types used are based on
`std::experimental::native_simd`, which takes up the width that would be most
efficient for the given data type and platform. No CPU/architecture specific
intrinsics are used, so the project would work just as well on ARM or any other
exotic architecture and it would fully make use of its SIMD capabilities.

## Materials

Currently the supported materials are:
- diffuse: The diffuse material behaves like an opaque material. It has an
  `albedo` (color) and it can be shadowes by other non-transmissive materials
  on the path between it and the light source. If diffuse reflections are
  enabled (`diffuse_reflection_ray_count` > 0) then indirect lighting is
  simulated by further tracing rays along the hemisphere of the hit.
- reflective: The reflective material behaves like a perfect mirror, which
  reflects all rays that intersect with it. It has no configurable properties.
- refractive: The refractive material behaves like a semi-transparent material,
  which both reflects rays, but also refracts them internally. It has an `ior`
  (index of refraction), which controls the angle of the refraction rays. The
  value should be the fraction: $\dfrac{\text{speed of light in
  vacuum}}{\text{speed of light in material}}$.
- texture: The textured material behaves like the diffuse material, except that
  instead of an albedo, a texture can be used (see [Textures](#textures)). Also
  indirect lighting is not implemented for texture materials, although the
  implementations should be the same as the one for diffuse materials.
- constant: The constant material just has an `albedo`, which is it's color at
  any point. There are no shadows, reflections, refractions or textures applied
  to it.

## Textures

Currently the supported textures are:
- albedo: The albedo texture has just that, an `albedo` (color).
- edge: The edge texture has an `inner_color`, an `outer_color` and an
  `edge_width`, specifying how wide an edge of the triangle is.
- checker: The checker texture has two colors (`color_a` and `color_b`) and a
  `square_size`. It repeats the two colors in a checker pattern and it respects
  the `uv` coordinates of the vertices.
- bitmap: The bitmap texture is loaded from a file using `stb_image`'s
  `stbi_load`. It then maps the image onto the mesh, also respecting the
  vertices' `uv` coordinates to position the texture properly.
