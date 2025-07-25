cmake_minimum_required(VERSION 3.20)
project(raytracer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_EXPORT_COMPILE_COMMANDS YES)
# set(CMAKE_CXX_CLANG_TIDY 
#     clang-tidy;
#     --use-color;
#     --header-filter=.*;
#     --checks=*;
#     --warnings-as-errors=*;)

add_compile_options(-Wall -Wextra -pedantic -Werror)

if (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo" OR CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-ggdb -fno-omit-frame-pointer)
endif()

include(FetchContent)

FetchContent_Declare(
    simdjson
    GIT_REPOSITORY  https://github.com/simdjson/simdjson.git
    GIT_TAG	    tags/v3.13.0
    GIT_SHALLOW	    TRUE
)
FetchContent_Declare(
    stb
    GIT_REPOSITORY  https://github.com/nothings/stb.git
    GIT_TAG	    master
)

FetchContent_MakeAvailable(simdjson)
FetchContent_MakeAvailable(stb)

add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})

add_executable(raytracer
    src/main.cpp
    src/stb_implementation.cpp
)

target_include_directories(raytracer
    PRIVATE	${CMAKE_CURRENT_SOURCE_DIR}/src
    PUBLIC	${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(raytracer simdjson stb)
