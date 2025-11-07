// Special thanks to Github user @cmburn for the modified version of this header.
/*
 * Nuklear - 1.32.0 - public domain
 * no warranty implied; use at your own risk.
 * authored from 2015-2016 by Micha Mettke
 */
/*
 * ==============================================================
 *
 *                              API
 *
 * ===============================================================
 */
#ifndef NK_SDL_VULKAN_H_
#define NK_SDL_VULKAN_H_

#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <SDL3/SDL.h>

enum nk_sdl_init_state { NK_SDL_DEFAULT = 0 };

NK_API struct nk_context *nk_sdl_init(SDL_Window *win, VkDevice logical_device,
                                      VkPhysicalDevice physical_device, uint32_t graphics_queue_family_index,
                                      VkImageView *image_views, uint32_t image_views_len, VkFormat color_format,
                                      enum nk_sdl_init_state init_state, VkDeviceSize max_vertex_buffer,
                                      VkDeviceSize max_element_buffer);
NK_API void nk_sdl_shutdown(void);
NK_API void nk_sdl_font_stash_begin(struct nk_font_atlas **atlas);
NK_API void nk_sdl_font_stash_end(VkQueue graphics_queue);
NK_API int nk_sdl_handle_event(SDL_Event *evt);
NK_API VkSemaphore nk_sdl_render(VkQueue graphics_queue, uint32_t buffer_index,
        VkSemaphore wait_semaphore, enum nk_anti_aliasing AA);
NK_API void nk_sdl_resize(VkImageView *imageViews, uint32_t imageViewCount, uint32_t framebuffer_width,
                          uint32_t framebuffer_height);
NK_API void nk_sdl_device_destroy(void);
NK_API void nk_sdl_device_create(VkDevice logical_device,
                                 VkPhysicalDevice physical_device, uint32_t graphics_queue_family_index,
                                 VkImageView *image_views, uint32_t image_views_len, VkFormat color_format,
                                 VkDeviceSize max_vertex_buffer, VkDeviceSize max_element_buffer,
                                 uint32_t framebuffer_width, uint32_t framebuffer_height);
NK_API void nk_sdl_handle_grab(void);

#endif
/*
 * ==============================================================
 *
 *                          IMPLEMENTATION
 *
 * ===============================================================
 */
#define NK_SDL_VULKAN_IMPLEMENTATION
#ifdef NK_SDL_VULKAN_IMPLEMENTATION
#undef NK_SDL_VULKAN_IMPLEMENTATION


#endif