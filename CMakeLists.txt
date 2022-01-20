cmake_minimum_required(VERSION 3.14)
project(Game_Engine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpessimizing-move -Wredundant-move -std=c++17")


set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(extern/glfw)
add_subdirectory(extern/spirv-cross)
add_subdirectory(extern/assimp)

include_directories(extern/glfw/include)
include_directories(extern/spirv-cross/include)
include_directories(extern/assimp/include)
include_directories(extern/imgui)
include_directories(extern/stb)
include_directories(extern/tinyobjloader)
include_directories(extern/glm)
include_directories(extern/mathlib)
include_directories(src)

file(GLOB_RECURSE HEADERS_FILES "src/*.h")
file(GLOB_RECURSE SOURCE_FILES "src/*.cpp")

set(SOURCES
        ${SOURCE_FILES}

        extern/mathlib/mathlib.cpp
        extern/imgui/imgui.cpp
        extern/imgui/imgui_draw.cpp
        extern/imgui/imgui_widgets.cpp
        extern/imgui/imgui_demo.cpp
        extern/imgui/imgui_tables.cpp
        extern/imgui/misc/cpp/imgui_stdlib.cpp
        src/Platform/MacOS/DialogsMacOS.mm)

set(HEADERS
        ${HEADER_FILES}

        extern/mathlib/mathlib.h
        extern/imgui/imgui.h
        extern/imgui/misc/cpp/imgui_stdlib.h)

set_property(SOURCE src/Platform/MacOS/DialogsMacOS.mm PROPERTY COMPILE_FLAGS "-x objective-c++")

find_package(Vulkan REQUIRED)

find_package(PkgConfig REQUIRED)

pkg_check_modules(GTKMM gtkmm-3.0)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
target_link_libraries(${PROJECT_NAME} ${Vulkan_LIBRARIES} glfw ${GLFW_LIBRARIES} ${GTKMM_LIBRARIES} spirv-cross-cpp assimp)
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
target_compile_definitions(${PROJECT_NAME} PRIVATE BASE_DIR="${PROJECT_SOURCE_DIR}")
target_include_directories(${PROJECT_NAME} PUBLIC ${GTKMM_INCLUDE_DIRS} ${Vulkan_INCLUDE_DIRS})

add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_SOURCE_DIR}/bin/
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_SOURCE_DIR}/bin/sandbox
)


file(GLOB_RECURSE GLSL_SOURCE_FILES
        "shaders/*.frag.glsl"
        "shaders/*.vert.glsl"
        "shaders/*.geom.glsl"
        )

foreach (GLSL ${GLSL_SOURCE_FILES})
    get_filename_component(FILE_NAME ${GLSL} NAME)
    string(REGEX REPLACE "\\.[^.]*$" "" FILE_NAME ${FILE_NAME})
    set(SPIRV "${PROJECT_SOURCE_DIR}/shaders/${FILE_NAME}.spv")
    add_custom_command(
            OUTPUT ${SPIRV}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_SOURCE_DIR}/shaders/"
            COMMAND glslangValidator -V ${GLSL} -o ${SPIRV}
            DEPENDS ${GLSL}
    )
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach (GLSL)

add_custom_target(
        Shaders
        DEPENDS ${SPIRV_BINARY_FILES}
)

add_dependencies(${PROJECT_NAME} Shaders)

add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders/"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_BINARY_DIR}/shaders" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders"
)
