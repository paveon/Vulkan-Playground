cmake_minimum_required(VERSION 3.14)
project(Vulkan)

set(CMAKE_CXX_STANDARD 17)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(extern/glfw)
include_directories(extern/glfw/include)


set(SOURCES
        src/renderer.cpp
        src/mathlib.cpp
        src/utils.cpp src/utils.h)

set(HEADERS
        src/mathlib.h
        src/utils.cpp src/utils.h)


add_executable(renderer ${SOURCES} ${HEADERS})
target_link_libraries(renderer vulkan glfw ${GLFW_LIBRARIES})
target_compile_options(renderer PRIVATE -Wall -Wextra)
target_compile_definitions(renderer PRIVATE)




file(GLOB_RECURSE GLSL_SOURCE_FILES
        "shaders/*.frag.glsl"
        "shaders/*.vert.glsl"
        )

foreach(GLSL ${GLSL_SOURCE_FILES})
    get_filename_component(FILE_NAME ${GLSL} NAME)
    string(REGEX REPLACE "\\.[^.]*$" "" FILE_NAME ${FILE_NAME})
    set(SPIRV "${PROJECT_BINARY_DIR}/shaders/${FILE_NAME}.spv")
    add_custom_command(
            OUTPUT ${SPIRV}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/shaders/"
            COMMAND glslangValidator -V ${GLSL} -o ${SPIRV}
            DEPENDS ${GLSL})
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
        Shaders
        DEPENDS ${SPIRV_BINARY_FILES}
)

add_dependencies(renderer Shaders)

add_custom_command(TARGET renderer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:renderer>/shaders/"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_BINARY_DIR}/shaders"
        "$<TARGET_FILE_DIR:renderer>/shaders"
        )