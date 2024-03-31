/* We define this to get clock_gettime(CLOCK_MONOTONIC) */
#define _POSIX_C_SOURCE 200809L

/* Global Libraries */
#include <vulkan/vulkan.h>
#include <cglm/cglm.h>

extern const int WIDTH;
extern const int HEIGHT;
extern const int DEBUG;
//extern const char *cfg_validation_layers[];
extern const VkFormat cfg_format;

extern const vec3 CAMERA_UP;

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

typedef struct {
  VkDeviceSize size;
  VkBuffer handle;
  VkDeviceMemory memory;
  void* first_index;
} DeviceBuffer;

typedef struct {
  DeviceBuffer indices;
  uint32_t indices_used;
  DeviceBuffer vertices;
  uint32_t vertices_used;
  
  uint32_t texture_count;
} GeometryData;

typedef struct {
  VkImage handle;
  VkDeviceMemory memory;
  VkImageView view;
  VkSampler sampler;
} ImageData;

typedef struct {
  vec3 pos;
  uint32_t texture_index;
  
  uint32_t indices_len;
  uint32_t first_index;
  uint32_t vertex_offset;
} Entity;

typedef struct{
  uint32_t texture_index;
  mat4 transform;
} drawArgs;

/** Maths */
typedef struct{
  vec3 pos;
  vec3 normal;
  vec2 tex;
} vertex;

/* Stage 1 [Context + Swapchain] */
int stage1_create(GfxContext* context);
int stage1_destroy(GfxContext context);

/* Stage 2 [Pipeline + Descriptor Sets] */
int stage2_create(GfxContext context, GfxPipeline* pipeline);
int stage2_destroy(GfxContext context, GfxPipeline pipeline);

/* pipeline.c */
int descriptor_pool_create(GfxContext context, VkDescriptorPool* descriptor_pool);
int slow_descriptors_alloc(VkDevice l_dev, GfxPipeline *pipeline);
int slow_descriptors_update(GfxContext context, uint32_t count,
			    ImageData *textures, GfxPipeline *pipeline);
int rapid_descriptors_alloc(GfxContext context, GfxPipeline *pipeline);
void rapid_descriptors_update(GfxContext context, GfxPipeline pipeline,
			      DeviceBuffer *transforms);
/* Camera  */
Camera camera_create(GfxContext context);
int fps_mvp_calc(Camera cam, Entity entity, mat4 *dest);
int camera_rotate(Camera *cam, float x_vel, float y_vel);

/* memory.c */
int buffer_create(GfxContext context, VkDeviceSize size,
		  VkBufferUsageFlags usage, DeviceBuffer *buffer);
void buffers_destroy(GfxContext context, int count, DeviceBuffer *buffers);

int image_view_create(VkDevice l_dev, VkImage image,
		  VkImageView *view, VkFormat format,
		  VkImageAspectFlags aspect_flags);
int image_create(GfxContext context, VkImage *image,
		 VkDeviceMemory *image_memory, VkFormat format,
		 VkImageUsageFlags usage, uint32_t width, uint32_t height);
void image_destroy(GfxContext context, ImageData *image);

/* Drawing */
VkCommandBuffer command_single_begin(GfxContext context);
int command_single_end(GfxContext context, VkCommandBuffer *command_buffer);

int indirect_buffer_create(GfxContext context, int entity_c, DeviceBuffer *draw);
int indirect_buffer_update(DeviceBuffer buffer, Entity *entities);

void draw_args_create(GfxContext context, GfxPipeline pipeline,
		       int count, DeviceBuffer *buffers);
void draw_args_update(Camera cam, Entity *entities, DeviceBuffer buffer);


uint32_t draw_start(GfxContext *profile, GfxPipeline pipeline, GeometryData geo);
int draw_end(GfxContext profile, GfxPipeline pipeline);

/* vk_model.c
 */
GeometryData geometry_buffer_create(GfxContext context);
int gltf_load(GfxContext context, const char* filename,
	      GeometryData *geo, ImageData *image, Entity* components);
int models_destroy(GfxContext context, uint32_t count,
		   GeometryData *geometry, ImageData *textures);

