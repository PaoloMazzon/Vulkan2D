cmake_minimum_required(VERSION 3.10)
project(retrolook)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

# Vulkan and SDL2 (latest, since Vulkan support is fairly recent) are all that needed
find_package(Vulkan)
find_package(SDL2 REQUIRED)

# All source files are located in the VK2D folder
file(GLOB C_FILES ../../VK2D/*.c)
file(GLOB H_FILES ../../VK2D/*.h)
set(VMA_FILES ../../VulkanMemoryAllocator/src/VmaUsage.cpp)
if (${CMAKE_C_COMPILER_ID} STREQUAL "GNU" AND WIN32)
	set(SDL2_LIBRARIES mingw32 SDL2main SDL2)
endif()

# We don't build a library because this engine is intended to have its source
# dropped into a host project (in this case JamEngine) and this whole cmake file
# is just for testing purposes.
include_directories(../../ ${SDL2_INCLUDE_DIR} ${Vulkan_INCLUDE_DIRS})
add_executable(${PROJECT_NAME} main.c ${VMA_FILES} ${C_FILES} ${H_FILES})
target_link_libraries(${PROJECT_NAME} m ${SDL2_LIBRARIES} ${Vulkan_LIBRARIES})