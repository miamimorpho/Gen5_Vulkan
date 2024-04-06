/* We define this to get clock_gettime(CLOCK_MONOTONIC) */
#define _POSIX_C_SOURCE 200809L

#ifndef RENDER_H
#define RENDER_H

/* External Dependacies */
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
  VkExtent2D extent; /* framebuffer dimensions */
  VkPhysicalDevice p_dev;     /* Physical Device */
  VkDevice l_dev;             /* Logical Device */
  VkQueue queue;
  uint32_t frame_c;
  VkCommandPool command_pool;

  /* Swapchain Data */
  VkSwapchainKHR swapchain_handle;
  uint32_t frame_index;
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

  VkDescriptorPool descriptor_pool;
  VkDescriptorSetLayout slow_layout; // rare frequency
  VkDescriptorSet slow_set;
  
  VkDescriptorSetLayout rapid_layout; // every frame
  VkDescriptorSet* rapid_sets;

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
  void* *p_next;
} GfxBuffer;

typedef struct {
  VkImage handle;
  VkDeviceMemory memory;
  VkImageView view;
  VkSampler sampler;
} ImageData;

typedef struct {
  vec3 pos;

  vec3 up;
  vec3 front;
  vec3 right;
  
  float fov;
  float aspect_ratio;
  
  float near;
  float far;
} Camera;

/** Maths */
typedef struct{
  vec3 pos;
  vec3 normal;
  vec2 tex;
} vertex;

/* Stage 1 [Context + Swapchain] */
int stage1_create(GfxContext*);
int stage1_destroy(GfxContext);

/* Stage 2 [Pipeline + Descriptor Sets] */
int stage2_create(GfxContext, GfxPipeline*);
int stage2_destroy(GfxContext, GfxPipeline);

/* VKBuffer Memory Management memory.c */
int buffer_create(GfxContext, VkDeviceSize, VkBufferUsageFlags, GfxBuffer* );
int buffer_append(const void*, GfxBuffer*, size_t);
void buffers_destroy(GfxContext, GfxBuffer*, int);

/* Draw Commands draw.c */
VkCommandBuffer command_single_begin(GfxContext);
int command_single_end(GfxContext, VkCommandBuffer*);
int draw_indirect_create(GfxContext, GfxBuffer*, int);
int draw_start(GfxContext*, GfxPipeline);
int draw_end(GfxContext, GfxPipeline);

/* camera.c */
int camera_rotate(Camera*, float, float);
Camera camera_create(int, int);

/* images.c */
int  image_view_create(VkDevice, VkImage,
		  VkImageView*, VkFormat, VkImageAspectFlags);
int  image_create(GfxContext, VkImage*, VkDeviceMemory*, VkFormat,
		 VkImageUsageFlags, uint32_t, uint32_t);
void image_destroy(GfxContext, ImageData* );
int  image_file_load(GfxContext, unsigned char*,
		    size_t, size_t, size_t, ImageData* );
int  slow_descriptors_update(GfxContext, uint32_t, ImageData*, GfxPipeline*);
int  slow_descriptors_alloc(VkDevice, GfxPipeline* );
int  rapid_descriptors_alloc(GfxContext, GfxPipeline* );
void rapid_descriptors_update(GfxContext, GfxPipeline, GfxBuffer* );

#endif /* RENDER_H */
