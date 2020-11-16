cmake_minimum_required(VERSION 3.14)
project(Game_Engine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(extern/glfw)
add_subdirectory(extern/spirv-cross)
add_subdirectory(extern/assimp)

include_directories(extern/vulkan/Include)
include_directories(extern/glfw/include)
include_directories(extern/spirv-cross/include)
include_directories(extern/assimp/include)
include_directories(extern/imgui)
include_directories(extern/stb)
include_directories(extern/tinyobjloader)
include_directories(extern/glm)
include_directories(extern/mathlib)
include_directories(src)
#include_directories(D:/VulkanSDK/1.1.106.0/Include)

file(GLOB_RECURSE HEADERS_FILES "src/*.h")
file(GLOB_RECURSE SOURCE_FILES "src/*.cpp")


set(SOURCES
        ${SOURCE_FILES}

        extern/mathlib/mathlib.cpp
        extern/imgui/imgui.cpp
        extern/imgui/imgui_draw.cpp
        extern/imgui/imgui_widgets.cpp
        extern/imgui/imgui_demo.cpp)

set(HEADERS
        ${HEADER_FILES}

        extern/mathlib/mathlib.h
        extern/imgui/imgui.h)


find_package(Vulkan REQUIRED)

#add_library(vulkan STATIC IMPORTED GLOBAL)
#set_target_properties(vulkan PROPERTIES IMPORTED_LOCATION "D:/VulkanSDK/1.1.106.0/Lib/vulkan-1.lib")

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan glfw ${GLFW_LIBRARIES} spirv-cross-cpp assimp)
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
target_compile_definitions(${PROJECT_NAME} PRIVATE BASE_DIR="${PROJECT_SOURCE_DIR}")
target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS}/vulkan)

#message(${Vulkan_INCLUDE_DIRS})



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

add_dependencies(${PROJECT_NAME} Shaders)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders/"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_BINARY_DIR}/shaders"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders"
        )