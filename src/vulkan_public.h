/* We define this to get clock_gettime(CLOCK_MONOTONIC) */
#define _POSIX_C_SOURCE 200809L

#ifndef RENDER_H
#define RENDER_H

/* External Dependacies */
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <cglm/cglm.h>

extern const int WIDTH;
extern const int HEIGHT;
extern const int DEBUG;
extern const uint32_t MAX_SAMPLERS;

//extern const char *cfg_validation_layers[];
extern const VkFormat cfg_format;

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
  VkCommandPool command_pool;

  /* Swapchain Data */
  VkSwapchainKHR swapchain_handle;
  uint32_t current_frame_index;
  VkImage *frames;
  VkImageView *views;
} GfxContext;

/* All the information required for a unique vulkan pipeline */
typedef struct {
  VkRenderPass render_pass;
  VkImage depth;
  VkDeviceMemory depth_memory;
  VkImageView depth_view;
  
  VkPipelineLayout layout;
  VkPipeline pipeline;
  
  VkCommandBuffer command_buffer;
  /* Sync Objects */
  VkSemaphore image_available;
  VkSemaphore render_finished;
  VkFence in_flight;
  
  VkFramebuffer *framebuffers;
  
} GfxPipeline;

/* Device Buffer contains opaque handles to Vulkan buffers
 * its size is static even if the data is represent changes
 */
typedef struct {
  VkDeviceSize total_size;
  VkDeviceSize used_size;
  VkBuffer handle;
  VkDeviceMemory memory;
  void* first_ptr;
  void* p_next;
} GfxBuffer;

typedef struct {
  VkImage handle;
  VkDeviceMemory memory;
  VkImageView view;
  VkSampler sampler;
} ImageData;

typedef struct {

  VkDescriptorPool descriptor_pool;
  VkDescriptorSetLayout slow_layout; // rare frequency
  VkDescriptorSet slow_sets;
  GfxBuffer geometry;

  VkDescriptorSetLayout rapid_layout; // every frame
  VkDescriptorSet* rapid_sets;
  ImageData textures[128];
} GfxResources;

/** Maths */
typedef struct{
  vec3 pos;
  vec3 normal;
  vec2 tex;
} vertex;

/* Stage 1 [Context + Swapchain] */
int context_create(GfxContext*);
int context_destroy(GfxContext);

/* Stage 2 [Pipeline + Descriptor Sets] */
int pipeline_create(GfxContext, GfxResources, GfxPipeline*);
int pipeline_destroy(GfxContext, GfxPipeline);

/* VKBuffer Memory Management memory.c */
int buffer_create(GfxContext, VkDeviceSize, VkBufferUsageFlags, GfxBuffer* );
int buffer_append(const void*, GfxBuffer*, size_t);
void buffers_destroy(GfxContext, GfxBuffer*, int);

void image_destroy(GfxContext, ImageData*);
int image_create(GfxContext, VkImage*, VkDeviceMemory*,
		 VkFormat, VkImageUsageFlags, uint32_t, uint32_t);
int image_view_create(VkDevice, VkImage, VkImageView*, VkFormat,
		      VkImageAspectFlags);

#endif /* RENDER_H */
