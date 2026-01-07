cmake_minimum_required(VERSION 3.0)
project(final_proj)

find_package(OpenGL REQUIRED)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

add_subdirectory(external)

include_directories(
    external/glfw-3.1.2/include/
    external/glm-0.9.7.1/
    external/glad-3.3/include/
    external/tinygltf-2.9.3/
    external/
    project/
)

add_executable(main
    project/main.cpp
    project/render/shader.cpp
    project/stb_config.cpp 
)

target_link_libraries(main
    ${OPENGL_LIBRARY}
    glfw
    glad
)