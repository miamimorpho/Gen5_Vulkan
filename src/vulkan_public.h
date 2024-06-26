/* We define this to get clock_gettime(CLOCK_MONOTONIC) */
#define _POSIX_C_SOURCE 200809L

#ifndef RENDER_H
#define RENDER_H

/* External Dependacies */
#include "../extern/vk_mem_alloc.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <cglm/cglm.h>

#define MAX_SAMPLERS 128

extern const int WIDTH;
extern const int HEIGHT;
extern const int DEBUG;

extern const char *cfg_validation_layers[];
extern const VkFormat cfg_format;

typedef struct {
  VkBuffer handle;
  VmaAllocation allocation;
  VkDeviceSize used_size;
} GfxBuffer;

typedef struct {
  VkImage handle;
  VmaAllocation allocation;
  VkImageView view;
  VkSampler sampler;
} GfxImage;

/**
 * Configuration settings and states
 */
typedef struct {
  /* Device Data */
  GLFWwindow* window;
  VkExtent2D extent; /* framebuffer dimensions */
  VkPhysicalDevice p_dev;     /* Physical Device */
  VkDevice l_dev;             /* Logical Device */
  VkQueue queue;
  uint32_t frame_c;
  VmaAllocator allocator;

  VkCommandPool command_pool;
  VkCommandBuffer command_buffer;

  /* Global Descriptor Sets */
  VkDescriptorPool descriptor_pool;
  
  /* Swapchain Data */
  VkSwapchainKHR swapchain_handle;
  VkImage *frames;
  VkImageView *views;
  VkRenderPass render_pass;
  GfxImage depth;
  VkFramebuffer *framebuffers;

  /* Timers */
  VkSemaphore image_available;
  VkSemaphore render_finished;
  VkFence in_flight;
  uint32_t current_frame_index;

  /* Global Texture Array */
  VkDescriptorSetLayout texture_descriptors_layout;
  VkDescriptorSet texture_descriptors;
  
} GfxContext;

typedef struct {
 
  uint32_t firstIndex;
  uint32_t indexCount;
  uint32_t vertexOffset;
  uint32_t textureIndex;
} GfxModelOffsets;

/** Maths */
typedef struct{
  vec3 pos;
  vec3 normal;
  vec2 uv;
} vertex3;

/* Stage 1 [Context + Swapchain] */
int context_create(GfxContext*);
int context_destroy(GfxContext);

/* VKBuffer Memory Management memory.c */
int buffer_create(GfxContext context, GfxBuffer* buffer,
		  VkBufferUsageFlags usage,
		  VmaAllocatorCreateFlags flags,
		  VkDeviceSize size);
int buffer_append(GfxContext context, GfxBuffer *dest,
		  const void* src, VkDeviceSize src_size);
GfxBuffer* buffer_next(GfxContext, GfxBuffer);
int buffer_destroy(GfxContext, GfxBuffer);
int buffers_destroy(GfxContext context, GfxBuffer* buffers, int count);
void image_destroy(GfxContext, GfxImage);
int image_create(GfxContext context,
	     VkImage *image, VmaAllocation *image_alloc,
	     VkImageUsageFlags usage, VkFormat format,
	     uint32_t width, uint32_t height);
int image_view_create(VkDevice, VkImage, VkImageView*, VkFormat,
		      VkImageAspectFlags);


#endif /* RENDER_H */
