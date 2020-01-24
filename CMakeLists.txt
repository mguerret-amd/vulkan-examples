cmake_minimum_required(VERSION 3.15)
project(VulkanExamples)

set(CMAKE_C_STANDARD 90)

find_package(PkgConfig REQUIRED)
pkg_search_module(GLIB REQUIRED glib-2.0)

find_package(SDL2 REQUIRED)
find_package(Vulkan REQUIRED)

add_executable(triangle main.c)

target_include_directories(triangle PRIVATE ${Vulkan_INCLUDE_DIR} ${GLIB_INCLUDE_DIRS})
target_link_libraries(triangle ${GLIB_LDFLAGS} ${Vulkan_LIBRARY} ${SDL2_LIBRARIES})