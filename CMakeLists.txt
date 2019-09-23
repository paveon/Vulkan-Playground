cmake_minimum_required(VERSION 3.14)
project(Vulkan)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(extern/glfw)
include_directories(extern/glfw/include)
include_directories(extern/imgui)
include_directories(extern/stb)
include_directories(extern/tinyobjloader)
include_directories(extern/glm)
include_directories(extern/mathlib)
include_directories(src)
include_directories(D:/VulkanSDK/1.1.106.0/Include)


set(SOURCES
        src/SandboxApp.cpp

        src/Engine/Renderer/renderer.cpp
        src/Engine/Renderer/utils.cpp
        src/Engine/Renderer/Device.cpp
        src/Engine/Renderer/vulkan_wrappers.cpp

        src/Engine/Window.cpp
        src/Engine/Application.cpp
        src/Engine/EntryPoint.cpp
        src/Engine/LayerStack.cpp
        src/Engine/ImGuiLayer.cpp
        src/Engine/Renderer/Texture.cpp
        src/Engine/Renderer/GraphicsContext.cpp

        src/Platform/Windows/WindowWin32.cpp
        src/Platform/Windows/InputWin32.cpp
        src/Platform/Vulkan/TextureVk.cpp
        src/Platform/Vulkan/GraphicsContextVk.cpp

        extern/mathlib/mathlib.cpp
        extern/imgui/imgui.cpp
        extern/imgui/imgui_draw.cpp
        extern/imgui/imgui_widgets.cpp
        extern/imgui/imgui_demo.cpp)

set(HEADERS
        src/Engine/Renderer/utils.h
        src/Engine/Renderer/Device.h
        src/Engine/Renderer/renderer.h
        src/Engine/Renderer/vulkan_wrappers.h
        src/Engine/Renderer/GraphicsContext.h
        src/Engine/Renderer/Texture.h

        src/Platform/Windows/WindowWin32.h
        src/Platform/Windows/InputWin32.h
        src/Platform/Vulkan/GraphicsContextVk.h
        src/Platform/Vulkan/TextureVk.h

        src/Engine/Window.h
        src/Engine/Events/Event.h
        src/Engine/Events/KeyEvents.h
        src/Engine/Events/MouseEvents.h
        src/Engine/Events/WindowEvents.h

        src/Engine/Core.h
        src/Engine/Application.h
        src/Engine/Include/Engine.h
        src/Engine/EntryPoint.h
        src/Engine/Layer.h
        src/Engine/LayerStack.h
        src/Engine/ImGuiLayer.h
        src/Engine/Input.h
        src/Engine/KeyCodes.h
        src/Engine/MouseButtonCodes.h

        extern/mathlib/mathlib.h
        extern/imgui/imgui.h)

add_library(vulkan STATIC IMPORTED GLOBAL)
set_target_properties(vulkan PROPERTIES IMPORTED_LOCATION "D:/VulkanSDK/1.1.106.0/Lib/vulkan-1.lib")

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