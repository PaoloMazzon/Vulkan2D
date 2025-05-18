cmake_minimum_required(VERSION 3.10)
project(gui)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

# Vulkan and SDL3 (latest, since Vulkan support is fairly recent) are all that needed
find_package(Vulkan)

# All source files are located in the VK2D folder
file(GLOB C_FILES ../../VK2D/*.c)
file(GLOB H_FILES ../../VK2D/*.h)
set(VMA_FILES ../../VulkanMemoryAllocator/src/VmaUsage.cpp)
set(REQUIRED_EXTRA_LIBS "")
set(SDL3_INCLUDE "../../SDL/include")
if (${CMAKE_C_COMPILER_ID} STREQUAL "GNU" AND WIN32)
	set(REQUIRED_EXTRA_LIBS m mingw32)
endif()

# We don't build a library because this engine is intended to have its source
# dropped into a host project (in this case JamEngine) and this whole cmake file
# is just for testing purposes.
include_directories(../../ ${SDL3_INCLUDE} ${Vulkan_INCLUDE_DIRS} ../../VulkanMemoryAllocator/include)
add_executable(${PROJECT_NAME} main.c ${VMA_FILES} ${C_FILES} ${H_FILES})
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3 ${REQUIRED_EXTRA_LIBS} ${Vulkan_LIBRARIES})