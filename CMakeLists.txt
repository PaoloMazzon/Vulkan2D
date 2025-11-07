cmake_minimum_required(VERSION 3.17)
project(Vulkan2D LANGUAGES C CXX)

option(VK2D_BUILD_EXAMPLES "Build examples for Vulkan2D" OFF)
option(VK2D_BUILD_SDL "Build SDL3 with VK2D" ON)

# VK2D requires C11 and C++17
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Dependencies
find_package(Vulkan REQUIRED)
if(VK2D_BUILD_SDL)
    add_subdirectory(extern/SDL)
endif()

# Vulkan2D
add_library(Vulkan2D
        VK2D/src/Buffer.c
        VK2D/src/Camera.c
        VK2D/src/Constants.c
        VK2D/src/DescriptorBuffer.c
        VK2D/src/DescriptorControl.c
        VK2D/src/Gui.c
        VK2D/src/Image.c
        VK2D/src/Initializers.c
        VK2D/src/Logger.c
        VK2D/src/LogicalDevice.c
        VK2D/src/Model.c
        VK2D/src/nuklear.c
        VK2D/src/PhysicalDevice.c
        VK2D/src/Pipeline.c
        VK2D/src/Polygon.c
        VK2D/src/Renderer.c
        VK2D/src/RendererMeta.c
        VK2D/src/Shader.c
        VK2D/src/ShadowEnvironment.c
        VK2D/src/Texture.c
        VK2D/src/Util.c
        VK2D/src/Validation.c
        VK2D/src/VulkanInterface.c
        extern/VulkanMemoryAllocator/src/VmaUsage.cpp
)

target_include_directories(Vulkan2D
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/VK2D/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/extern/VulkanMemoryAllocator/include
)

target_compile_features(Vulkan2D PUBLIC cxx_std_17)
target_link_libraries(Vulkan2D
        PUBLIC
        Vulkan::Vulkan
        PRIVATE
        $<$<TARGET_EXISTS:SDL3::SDL3>:SDL3::SDL3>
        $<$<TARGET_EXISTS:SDL3::SDL3-shared>:SDL3::SDL3-shared>
        $<$<TARGET_EXISTS:SDL3::SDL3-static>:SDL3::SDL3-static>
)

if(VK2D_BUILD_EXAMPLES)
    add_subdirectory(examples/main)
    add_subdirectory(examples/gui)
    add_subdirectory(examples/retrolook)
    add_subdirectory(examples/shadows)
    add_subdirectory(examples/shadowsglsl)
    add_subdirectory(examples/splitscreen)
    add_subdirectory(examples/testing)
endif()
