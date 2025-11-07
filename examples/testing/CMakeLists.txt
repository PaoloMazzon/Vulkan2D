add_executable(testing_suite main.c)
target_link_libraries(testing_suite PRIVATE Vulkan2D SDL3::SDL3)
