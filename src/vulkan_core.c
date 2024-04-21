#include "vulkan_public.h"

#include <string.h>


const int WIDTH = 640;
const int HEIGHT = 480;
const uint32_t MAX_SAMPLERS = 128;

/* Device Extensions required to run */
const char *cfg_device_extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, };

/* Debug/Validation Layers */
const int enable_debug = 1;
const char *cfg_validation_layers[] = { "VK_LAYER_KHRONOS_validation", };

const VkFormat cfg_format = VK_FORMAT_B8G8R8A8_SRGB;

/* File Specific Globals
 * used to isolate the handling of platform specific data
 */
static SDL_Window* SDL_WINDOW;
static VkInstance VULKAN_INSTANCE;
static VkSurfaceKHR VULKAN_SURFACE;


int
queue_index(VkPhysicalDevice p_dev, VkQueueFlagBits required_flags)
{
  uint32_t q_fam_c;
  vkGetPhysicalDeviceQueueFamilyProperties(p_dev, &q_fam_c, NULL);
  VkQueueFamilyProperties q_fam[q_fam_c];
  vkGetPhysicalDeviceQueueFamilyProperties(p_dev, &q_fam_c, q_fam);

  for(uint32_t i = 0; i < q_fam_c; i++){
    if(q_fam[i].queueFlags & required_flags){
      return i;
    }
  }

  return -1;
}

/**
 * checks that colour format and presentation mode
 * are supported. returns 0 if all configs settings are supported
 * return 0 REQUIRED FOR BOOT
 */
int
surface_create(GfxContext *stage1)
{
  if(SDL_Vulkan_CreateSurface(SDL_WINDOW, VULKAN_INSTANCE,
			      &VULKAN_SURFACE)
     != SDL_TRUE){
    printf("failed to create SDL_WINDOW surface");
    SDL_DestroyWindow(SDL_WINDOW);
    SDL_Quit();
    return 1;
  }
  
  /* Test for queue presentation support */
  VkBool32 present_support = VK_FALSE;
  vkGetPhysicalDeviceSurfaceSupportKHR
    (stage1->p_dev, queue_index(stage1->p_dev, VK_QUEUE_GRAPHICS_BIT),
     VULKAN_SURFACE, &present_support);
  if(present_support == VK_FALSE){
    printf("!no device presentation support!\n");
    return -2;
  }
  
  /* Find colour format */
  uint32_t format_c;
  vkGetPhysicalDeviceSurfaceFormatsKHR(stage1->p_dev, VULKAN_SURFACE,
				       &format_c, NULL);
  VkSurfaceFormatKHR formats[format_c];
  vkGetPhysicalDeviceSurfaceFormatsKHR(stage1->p_dev, VULKAN_SURFACE,
				       &format_c, formats);

  VkBool32 supported = VK_FALSE;
  for (uint32_t i = 0; i < format_c; i++) {
    if(formats[i].format == cfg_format
       && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
      supported = VK_TRUE;
  }
  if (supported == VK_FALSE) {
    printf("!colour format not supported!\n");
    return 1;
  }
  
  /* Presentation mode */
  uint32_t mode_c;
  vkGetPhysicalDeviceSurfacePresentModesKHR(stage1->p_dev, VULKAN_SURFACE,
					    &mode_c, NULL);
  VkPresentModeKHR modes[mode_c];
  vkGetPhysicalDeviceSurfacePresentModesKHR(stage1->p_dev, VULKAN_SURFACE,
					    &mode_c, modes);

  supported = VK_FALSE;
  for (uint32_t i = 0; i < mode_c; i++) {
    if (modes[i]== VK_PRESENT_MODE_FIFO_KHR)
      supported = VK_TRUE;
  }
  if (supported == VK_FALSE) {
    printf("!presentation mode not supported!\n");
    return 2;
  }

  return 0;
}

int
swapchain_create(GfxContext *stage1)
{
  int width, height;
  SDL_Vulkan_GetDrawableSize(SDL_WINDOW, &width, &height);
  stage1->extent.width = (uint32_t)width;
  stage1->extent.height = (uint32_t)height;

  VkSurfaceCapabilitiesKHR capable;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR
    (stage1->p_dev, VULKAN_SURFACE, &capable);

  if (capable.currentExtent.width != stage1->extent.width
     || capable.currentExtent.height != stage1->extent.height) {
    stage1->extent.width =
      stage1->extent.width > capable.maxImageExtent.width ?
      capable.maxImageExtent.width : stage1->extent.width;

    stage1->extent.width =
      stage1->extent.width < capable.minImageExtent.width ?
      capable.minImageExtent.width : stage1->extent.width;
    
    stage1->extent.height =
      stage1->extent.height > capable.maxImageExtent.height ?
      capable.maxImageExtent.height : stage1->extent.height;

    stage1->extent.height =
      stage1->extent.height < capable.minImageExtent.height ?
      capable.minImageExtent.height : stage1->extent.height;
  }
   
  uint32_t max_frame_c = capable.minImageCount + 1;
  if(capable.maxImageCount > 0 && max_frame_c > capable.maxImageCount) {
    max_frame_c = capable.maxImageCount;
  }
  
  VkSwapchainCreateInfoKHR create_info = {
    .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .surface = VULKAN_SURFACE,
    .minImageCount = max_frame_c,
    .imageFormat = cfg_format,
    .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
    .imageExtent = stage1->extent,
    .imageArrayLayers = 1,
    .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    /* TODO: multiple queue indices ( current [0][0] ) */
    .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .queueFamilyIndexCount = 0,
    .pQueueFamilyIndices = NULL,
    .preTransform = capable.currentTransform,
    .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    .presentMode =  VK_PRESENT_MODE_FIFO_KHR,
    .clipped = VK_TRUE,
    .oldSwapchain = VK_NULL_HANDLE,
  };

  if (vkCreateSwapchainKHR(stage1->l_dev, &create_info, NULL,
			   &stage1->swapchain_handle)
     != VK_SUCCESS){
    printf("!failed to create swap chain!\n");
    return 1;
  }

  /* Create Image Views */
  vkGetSwapchainImagesKHR
    (stage1->l_dev, stage1->swapchain_handle, &stage1->frame_c, NULL);
  stage1->frames = malloc(stage1->frame_c * sizeof(VkImage));
  stage1->views = malloc(stage1->frame_c * sizeof(VkImageView));

  vkGetSwapchainImagesKHR
    (stage1->l_dev, stage1->swapchain_handle,
     &stage1->frame_c, stage1->frames);

  for (uint32_t i = 0; i < stage1->frame_c; i++) {
    image_view_create(stage1->l_dev, stage1->frames[i],
		      &stage1->views[i], cfg_format,
		      VK_IMAGE_ASPECT_COLOR_BIT);
  }

  return 0;
}

int
command_pool_create(GfxContext stage1, VkCommandPool *command_pool)
{
  VkCommandPoolCreateInfo pool_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    .queueFamilyIndex = queue_index(stage1.p_dev, VK_QUEUE_GRAPHICS_BIT),
  };

  if(vkCreateCommandPool(stage1.l_dev, &pool_info, NULL, command_pool)
     != VK_SUCCESS){
    printf("!failed to create command pool!");
    return 1;
  }

  return 0;
}

/* Returns size of *device_extensions[]
 * will return 0 unless every requested extension is available
 * positive int REQUIRED FOR BOOT
 */
int
device_ext_count(VkPhysicalDevice l_dev)
{
  uint32_t avail_ext_c;
  vkEnumerateDeviceExtensionProperties(l_dev, NULL, &avail_ext_c, NULL);
  VkExtensionProperties avail_ext[avail_ext_c];
  vkEnumerateDeviceExtensionProperties(l_dev, NULL, &avail_ext_c, avail_ext);

  uint32_t ext_c = sizeof(cfg_device_extensions) / sizeof(cfg_device_extensions[0]);
  int layer_found;
  printf("device_extensions [ ");
  for(uint32_t i = 0; i < ext_c; i++){
    layer_found = 0;
    for(uint32_t a = 0; a < avail_ext_c; a++){
      if(strcmp(cfg_device_extensions[i], avail_ext[a].extensionName) == 0){
	printf("%s, ", avail_ext[a].extensionName);
	layer_found = 1;
      }
    }
    if(!layer_found){
      printf("[MISSING %s]", cfg_device_extensions[i]);
      return 0;
    }
  }
  printf("]\n");
  
  return ext_c;
}

int
devices_create(GfxContext *stage1)
{
  uint32_t dev_c = 0;
  vkEnumeratePhysicalDevices(VULKAN_INSTANCE, &dev_c, NULL);
  if(dev_c > 1) return 1;
  VkPhysicalDevice devs[dev_c];
  vkEnumeratePhysicalDevices(VULKAN_INSTANCE, &dev_c, devs);
  VkPhysicalDevice p_dev = devs[0];

  /* Request Basic Features */
  VkPhysicalDeviceProperties dev_properties;
  vkGetPhysicalDeviceProperties(p_dev, &dev_properties);
  
  printf("GPU [ %s ]\n", dev_properties.deviceName);      
  printf("device features [ ");

  VkPhysicalDeviceFeatures dev_features;
  vkGetPhysicalDeviceFeatures(p_dev, &dev_features);  
  if(dev_features.geometryShader){
    // should be enabled by default
    printf("geometry sampler, ");
  }else{
    printf("[error] no geometry shader");
    return 1;
  }
  if(dev_features.samplerAnisotropy){
     printf("sampler anisotropy, ");
  }
  if(dev_features.multiDrawIndirect){
    printf("multidrawing, ");
  }

  /* Enable Descriptor Indexing */
  VkPhysicalDeviceDescriptorIndexingFeatures descriptor_indexing_features = {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT,
  };

  VkPhysicalDeviceFeatures2 dev_features2 = {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
    .pNext = &descriptor_indexing_features,
  };
  vkGetPhysicalDeviceFeatures2(p_dev, &dev_features2 );
 
  if(descriptor_indexing_features.descriptorBindingPartiallyBound)
    printf("descriptor Binding Partially Bound, ");
  if(descriptor_indexing_features.runtimeDescriptorArray)
    printf("runtime descriptor array, ");

  printf("]\n");
  /* Graphics Queue Init*/
  stage1->p_dev = p_dev;
  int graphics_queue_index = queue_index(stage1->p_dev, VK_QUEUE_GRAPHICS_BIT);
  
  float priority = 1.0f;
  VkDeviceQueueCreateInfo q_create_info = {
    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
    .queueFamilyIndex = graphics_queue_index,
    .queueCount = 1,
    .pQueuePriorities = &priority,
  };

  VkDeviceCreateInfo dev_create_info = {
    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .pQueueCreateInfos = & q_create_info,
    .queueCreateInfoCount = 1,
    //.pEnabledFeatures = &dev_features,
    .enabledExtensionCount = device_ext_count(stage1->p_dev),
    .ppEnabledExtensionNames = cfg_device_extensions,
    .enabledLayerCount = 0,
    .pNext = &dev_features2,
  };

  if(vkCreateDevice(stage1->p_dev, &dev_create_info, NULL, &stage1->l_dev)
     != VK_SUCCESS) {
    printf("!failed to create logical device!\n");
    return 2;
  }

  vkGetDeviceQueue(stage1->l_dev, graphics_queue_index, 0, &stage1->queue);

  return 0;
}

/* Returns size of *validation_layers[]
 * will return 0 unless every requested layer is available
 * REQUIRED FOR BOOT
 */
int
debug_layer_count(void)
{
  uint32_t avail_layer_c;
  vkEnumerateInstanceLayerProperties(&avail_layer_c, NULL);
  VkLayerProperties avail_layers[avail_layer_c];
  vkEnumerateInstanceLayerProperties(&avail_layer_c, avail_layers);

  int valid_layer_c = sizeof(cfg_validation_layers) / sizeof(cfg_validation_layers[0]);
  int layer_found;

  printf("validation_layers [ ");
  for (int i = 0; i < valid_layer_c; i++){
    layer_found = 0;
    for(uint32_t a = 0; a < avail_layer_c; a++){
      if(strcmp(cfg_validation_layers[i], avail_layers[a].layerName) == 0){
	printf("%s, ", avail_layers[a].layerName);
	layer_found = 1;
      }
    }
    if(!layer_found){
      printf("%s[error]", cfg_validation_layers[i]);
      return 0;
    }
  }
  printf("]\n");
  return valid_layer_c;
}

/**
 * checks required GFLW extensions are available
 * loads in validation layers if compiled with debug_enabled
 * returns true if there were errors
 */
int
instance_create()
{
  uint32_t vk_version;
  vkEnumerateInstanceVersion(&vk_version);
  printf("\n[VULKAN:%d.%d.%d]\n",
	 VK_VERSION_MAJOR(vk_version),
	 VK_VERSION_MINOR(vk_version),
	 VK_VERSION_PATCH(vk_version)
	 );

  VkApplicationInfo app_info = {
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pApplicationName = "Cars",
    .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
    .pEngineName = "Naomi Engine",
    .engineVersion = VK_MAKE_VERSION(1, 0, 0),
    .apiVersion = vk_version,//VK_API_VERSION_1_3,
  };

  uint32_t SDL_ext_c;
  SDL_Vulkan_GetInstanceExtensions(SDL_WINDOW, &SDL_ext_c, NULL);
  const char *SDL_ext[SDL_ext_c];
  SDL_Vulkan_GetInstanceExtensions(SDL_WINDOW, &SDL_ext_c, SDL_ext);

  uint32_t vk_ext_c = 0;
  vkEnumerateInstanceExtensionProperties(NULL, &vk_ext_c, NULL);
  VkExtensionProperties vk_ext[vk_ext_c+1];
  vkEnumerateInstanceExtensionProperties(NULL, &vk_ext_c, vk_ext);

  int ext_found;
  printf("instance extensions...\n");
  for (uint32_t g = 0; g < SDL_ext_c; g++){
    ext_found = 0;
    for (uint32_t i = 0; i < vk_ext_c; i++){
      if (strcmp(SDL_ext[g], vk_ext[i].extensionName) == 0){
	ext_found = 1;
	printf("[X]\t%s\n", vk_ext[i].extensionName);
      }
    }
    if(!ext_found){
      printf("!extension not found! %s\n", SDL_ext[g]);
      return 1;
    }
  }

  VkInstanceCreateInfo create_info = {
    .sType= VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pApplicationInfo = &app_info,
    .enabledExtensionCount = SDL_ext_c,
    .ppEnabledExtensionNames = SDL_ext,
    .enabledLayerCount = 0,
  };

  /* Enable validation_layers debugging */
  if(enable_debug){
    create_info.enabledLayerCount = debug_layer_count();
    create_info.ppEnabledLayerNames = cfg_validation_layers;
  }
    
  if(vkCreateInstance(&create_info, NULL, &VULKAN_INSTANCE) != VK_SUCCESS) {
    printf("failed to create VULKAN_INSTANCE!\n");
    return 1;
  }

 
  return 0;
}

int context_create(GfxContext *stage1)
{
  SDL_Init(SDL_INIT_VIDEO);
  SDL_WINDOW = SDL_CreateWindow("Vulkan", SDL_WINDOWPOS_UNDEFINED,
				 SDL_WINDOWPOS_UNDEFINED,
				 WIDTH, HEIGHT,
				 SDL_WINDOW_VULKAN);
  
  if(instance_create() > 0)
    return 1;

  if(devices_create(stage1) > 0) return 2;

  command_pool_create(*stage1, &stage1->command_pool);

  if(surface_create(stage1) > 0) return 3;

  if(swapchain_create(stage1) > 0) return 4;

  return 0;
}

int context_destroy(GfxContext stage1)
{
  for(uint32_t i = 0; i < stage1.frame_c; i++)
    vkDestroyImageView(stage1.l_dev, stage1.views[i], NULL);

  free(stage1.views);
  free(stage1.frames);
 
  vkDestroySwapchainKHR(stage1.l_dev, stage1.swapchain_handle, NULL);

  vkDestroyDevice(stage1.l_dev, NULL);
  vkDestroySurfaceKHR(VULKAN_INSTANCE, VULKAN_SURFACE, NULL);
  vkDestroyInstance(VULKAN_INSTANCE, NULL);

  SDL_DestroyWindow(SDL_WINDOW);
  SDL_Quit();
 
  return 0;
}

int
depth_format_check(VkPhysicalDevice p_dev, VkFormat *format)
{

  VkFormat depth_format = VK_FORMAT_UNDEFINED;
  VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
  VkFormatFeatureFlags features = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
  
  VkFormat candidates[3] = {
    //VK_FORMAT_D32_SFLOAT,
    //VK_FORMAT_D32_SFLOAT_S8_UINT,
    //VK_FORMAT_D24_UNORM_S8_UINT,
    VK_FORMAT_D16_UNORM,
  };

  /* cycle through format candidates for suitability */
  for(int i = 0; i < 3; i++){
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(p_dev, candidates[i], &props);

    if(tiling == VK_IMAGE_TILING_LINEAR
       && (props.linearTilingFeatures & features) == features ){
      depth_format = candidates[i];
      break;
    }
    if (tiling == VK_IMAGE_TILING_OPTIMAL
	&& (props.optimalTilingFeatures & features) == features){
      depth_format = candidates[i];
      break;
    }
  }

  if(depth_format != VK_FORMAT_UNDEFINED){
    *format = depth_format;
    return 0;
  }else {
    printf("no suitable depth format found!\n");
    return 1;
  }
}

VkShaderModule
shader_module_create(VkDevice l_dev, long size, const char* code)
{

  VkShaderModuleCreateInfo create_info = {
    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .codeSize = size,
    .pCode = (const uint32_t*)code
  };

  VkShaderModule shader_mod;
  if (vkCreateShaderModule(l_dev, &create_info, NULL, &shader_mod)
     != VK_SUCCESS) {
    printf("failed to create shader module!");
    return NULL;
  }
  
  return shader_mod;
}

char*
shader_file_read(const char* filename, long* buffer_size)
{
  
  FILE* file = fopen(filename, "rb");
  if(file == NULL) {
      printf("%s not found!", filename);
      return 0;
  }
  if(fseek(file, 0l, SEEK_END) != 0) {
    printf("failed to seek to end of file!");
    return 0;
  }
  *buffer_size = ftell(file);
  if (*buffer_size == -1){
    printf("failed to get file size!");
    return 0;
  }

  char *spv_code = (char *)malloc(*buffer_size * sizeof(char));
  //char spv_code[*buffer_size];
  rewind(file);
  fread(spv_code, *buffer_size, 1, file);
  
  fclose(file);
  
  return spv_code;
}

int
depth_create(GfxContext context, GfxPipeline *pipeline)
{
  VkFormat format;
  if(depth_format_check(context.p_dev, &format) == 1) return 1;
  
  image_create(context, &pipeline->depth, &pipeline->depth_memory,
	       format,
	       VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
	       context.extent.width, context.extent.height);

  image_view_create(context.l_dev, pipeline->depth, &pipeline->depth_view,
		    format, VK_IMAGE_ASPECT_DEPTH_BIT);
  
  return 0;
}

int
render_pass_create(GfxContext context, VkRenderPass *render_pass)
{
  VkAttachmentDescription color_attachment = {
    .format = cfg_format,
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
  };

  VkAttachmentReference color_attachment_ref = {
    .attachment = 0,
    .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
  };

  VkFormat depth_format;
  if(depth_format_check(context.p_dev, &depth_format)) return 1;
  
  VkAttachmentDescription depth_attachment = {
    .format = depth_format,
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
    .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
  };

  VkAttachmentReference depth_attachment_ref = {
    .attachment = 1,
    .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
  };

  VkSubpassDescription subpass = {
    .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
    .colorAttachmentCount = 1,
    .pColorAttachments = &color_attachment_ref,
    .pDepthStencilAttachment = &depth_attachment_ref,
  };
  
  VkSubpassDependency dependency = {
    .srcSubpass = VK_SUBPASS_EXTERNAL,
    .dstSubpass = 0,
    .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
    
    .srcAccessMask = 0,
    .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
    .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
    | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
  };

  VkAttachmentDescription attachments[2] =
    {color_attachment, depth_attachment};
  
  VkRenderPassCreateInfo render_pass_info = {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
    .attachmentCount = 2,
    .pAttachments = attachments,
    .subpassCount = 1,
    .pSubpasses = &subpass,
    .dependencyCount = 1,
    .pDependencies = &dependency
  };

  if(vkCreateRenderPass(context.l_dev, &render_pass_info, NULL, render_pass)
     != VK_SUCCESS) {
    printf("!failed to create render pass!");
    return 1;
  }

  return 0;
}

int
uber_pipeline_create(GfxContext context, GfxPipeline *gfx)
{
  long vert_spv_size;
  char* vert_spv = shader_file_read("shaders/vert.spv", &vert_spv_size);
  long frag_spv_size;
  char* frag_spv = shader_file_read("shaders/frag.spv", &frag_spv_size);

  VkShaderModule vert_shader_mod = shader_module_create
    (context.l_dev, vert_spv_size, vert_spv);
  VkShaderModule frag_shader_mod = shader_module_create
    (context.l_dev, frag_spv_size, frag_spv);
  free(vert_spv);
  free(frag_spv);
  
  VkPipelineShaderStageCreateInfo vert_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    .stage = VK_SHADER_STAGE_VERTEX_BIT,
    .module = vert_shader_mod,
    .pName = "main",
    .pSpecializationInfo = NULL,
  };

  VkPipelineShaderStageCreateInfo frag_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
    .module = frag_shader_mod,
    .pName = "main",
    .pSpecializationInfo = NULL,
  };

  VkPipelineShaderStageCreateInfo shader_stages[2] =
    {vert_info, frag_info};

  VkPipelineDepthStencilStateCreateInfo depth_stencil = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
    .depthTestEnable = VK_TRUE,
    .depthWriteEnable = VK_TRUE,
    .depthCompareOp = VK_COMPARE_OP_LESS,
    .depthBoundsTestEnable = VK_FALSE,
    .minDepthBounds = 0.0f,
    .maxDepthBounds = 1.0f,
    .stencilTestEnable = VK_FALSE,
    .front = { 0 },
    .back = { 0 },
  };
  
  VkDynamicState dynamic_states[2] =
    { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
  
  VkPipelineDynamicStateCreateInfo dynamic_state_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
    .dynamicStateCount = sizeof(dynamic_states) / sizeof(dynamic_states[0]),
    .pDynamicStates = dynamic_states
  };

  // Vertex Buffer Creation
  VkVertexInputAttributeDescription attribute_descriptions[3];
  attribute_descriptions[0].binding = 0;
  attribute_descriptions[0].location = 0;
  attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
  attribute_descriptions[0].offset = offsetof(vertex, pos);

  attribute_descriptions[1].binding = 0;
  attribute_descriptions[1].location = 1;
  attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
  attribute_descriptions[1].offset = offsetof(vertex, normal);

  attribute_descriptions[2].binding = 0;
  attribute_descriptions[2].location = 2;
  attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
  attribute_descriptions[2].offset = offsetof(vertex, tex);
  
  VkVertexInputBindingDescription binding_description = {
    .binding = 0,
    .stride = sizeof(vertex),
    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
  };
  
  VkPipelineVertexInputStateCreateInfo vertex_input_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .vertexBindingDescriptionCount = 1,
    .vertexAttributeDescriptionCount = 3,
    .pVertexBindingDescriptions = &binding_description,
    .pVertexAttributeDescriptions = attribute_descriptions,
  };

  VkPipelineInputAssemblyStateCreateInfo input_assembly_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    .primitiveRestartEnable = VK_FALSE,
  };

  VkViewport viewport = {
    .x = 0.0f,
    .y = 0.0f,
    .width = context.extent.width,
    .height = context.extent.height,
    .minDepth = 0.0f,
    .maxDepth = 1.0f,
  };

  VkRect2D scissor  = {
    .offset = { 0, 0},
    .extent = context.extent,
  };

  VkPipelineViewportStateCreateInfo viewport_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .viewportCount = 1,
    .pViewports = &viewport,
    .scissorCount = 1,
    .pScissors = &scissor,
  };

  VkPipelineRasterizationStateCreateInfo rasterization_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    .depthClampEnable = VK_FALSE,
    .rasterizerDiscardEnable = VK_FALSE,
    .polygonMode = VK_POLYGON_MODE_FILL,
    .lineWidth = 1.0f,
    .cullMode = VK_CULL_MODE_BACK_BIT,
    .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
    .depthBiasEnable = VK_FALSE,
    .depthBiasConstantFactor = 0.0f,
    .depthBiasClamp = 0.0f,
    .depthBiasSlopeFactor = 0.0f,
  };

  VkPipelineMultisampleStateCreateInfo multisample_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    .sampleShadingEnable = VK_FALSE,
    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    .minSampleShading = 1.0f,
    .pSampleMask = NULL,
    .alphaToCoverageEnable = VK_FALSE,
    .alphaToOneEnable = VK_FALSE,
  };

  VkPipelineColorBlendAttachmentState color_blend_attachment = {
    .colorWriteMask =
    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
    | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    .blendEnable = VK_FALSE,
    .srcColorBlendFactor = VK_BLEND_FACTOR_ONE, // Optional
    .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO, // Optional
    .colorBlendOp = VK_BLEND_OP_ADD, // Optional
    .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE, // Optional
    .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO, // Optional
    .alphaBlendOp = VK_BLEND_OP_ADD, // Optional
  };

  VkPipelineColorBlendStateCreateInfo color_blend_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .logicOpEnable = VK_FALSE,
    .logicOp = VK_LOGIC_OP_COPY,
    .attachmentCount = 1,
    .pAttachments = &color_blend_attachment,
    .blendConstants[0] = 0.0f,
    .blendConstants[1] = 0.0f,
    .blendConstants[2] = 0.0f,
    .blendConstants[3] = 0.0f,
  };
 
  VkGraphicsPipelineCreateInfo pipeline_info = {
    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .stageCount = 2,
    .pStages = shader_stages,
    .pVertexInputState = &vertex_input_info,
    .pInputAssemblyState = &input_assembly_info,
    .pViewportState = &viewport_info,
    .pRasterizationState = &rasterization_info,
    .pMultisampleState = &multisample_info,
    .pDepthStencilState = &depth_stencil,
    .pColorBlendState = &color_blend_info,
    .pDynamicState = &dynamic_state_info,
    .layout = gfx->layout,
    .renderPass = gfx->render_pass,
    .subpass = 0,
    .basePipelineHandle = VK_NULL_HANDLE,
    .basePipelineIndex = -1,
  };

  if(vkCreateGraphicsPipelines
     (context.l_dev, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &gfx->pipeline) != VK_SUCCESS) {
    printf("!failed to create graphics pipeline!\n");
  }
    
  vkDestroyShaderModule(context.l_dev, frag_shader_mod, NULL);
  vkDestroyShaderModule(context.l_dev, vert_shader_mod, NULL);
    
  return 0;
}

int
framebuffers_create(GfxContext context, GfxPipeline *pipeline)
{
  pipeline->framebuffers = malloc(context.frame_c * sizeof(VkFramebuffer));

  for(size_t i = 0; i < context.frame_c; i++){
    VkImageView attachments[] = {
      context.views[i],
      pipeline->depth_view,
    };

    VkFramebufferCreateInfo framebuffer_info = {
      .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      .renderPass = pipeline->render_pass,
      .attachmentCount = 2,
      .pAttachments = attachments,
      .width = context.extent.width,
      .height = context.extent.height,
      .layers = 1,
    };

    if(vkCreateFramebuffer(context.l_dev, &framebuffer_info,
			   NULL, &pipeline->framebuffers[i] )
       != VK_SUCCESS) {
      return 1;
      printf("!failed to create framebuffer!\n");
    }
  }

  return 0;
}

int
command_buffer_create(GfxContext context, VkCommandBuffer *command_buffer)
{
  VkCommandBufferAllocateInfo allocate_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool = context.command_pool,
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = 1,
  };

  if(vkAllocateCommandBuffers(context.l_dev, &allocate_info, command_buffer)
     != VK_SUCCESS) {
    printf("!failed to allocate command buffers!\n");
    return 1;
  }

  return 0;
}

int
pipeline_create(GfxContext context, GfxResources resources, GfxPipeline *pipeline)
{
  depth_create(context, pipeline);
  render_pass_create(context, &pipeline->render_pass);
  framebuffers_create(context, pipeline);
  
  /* Sync Objects for Command Buffers */
  command_buffer_create(context, &pipeline->command_buffer);
  VkSemaphoreCreateInfo semaphore_info = {
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
  };

  VkFenceCreateInfo fence_info = {
    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    .flags = VK_FENCE_CREATE_SIGNALED_BIT,
  };

  if(vkCreateSemaphore(context.l_dev, &semaphore_info,
		       NULL, &pipeline->image_available)
     != VK_SUCCESS ||
     vkCreateSemaphore(context.l_dev, &semaphore_info,
		       NULL, &pipeline->render_finished)
     != VK_SUCCESS ||
     vkCreateFence(context.l_dev, &fence_info,
		   NULL, &pipeline->in_flight)
     != VK_SUCCESS ) {
    printf("!failed to create sync objects!\n");
    return 1;
  }

  VkDescriptorSetLayout descriptor_set_layouts[2] = {
    resources.slow_layout, resources.rapid_layout
  };
  
  VkPipelineLayoutCreateInfo pipeline_layout_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .setLayoutCount = 2,
    .pSetLayouts = descriptor_set_layouts,

  };

  if(vkCreatePipelineLayout(context.l_dev, &pipeline_layout_info,
			    NULL, &pipeline->layout)
       != VK_SUCCESS)
    {
      printf("!failed to create pipeline layout!\n");
      return 1;
    }



  uber_pipeline_create(context, pipeline);
 
  return 0;
}

int
pipeline_destroy(GfxContext context, GfxPipeline pipeline)
{
  vkResetCommandBuffer(pipeline.command_buffer, 0);
  vkDestroyCommandPool(context.l_dev, context.command_pool, NULL);
  vkDestroyImageView(context.l_dev, pipeline.depth_view, NULL);
  vkDestroyImage(context.l_dev, pipeline.depth, NULL);
  vkFreeMemory(context.l_dev, pipeline.depth_memory, NULL);
  
  /* Cleaning Up */
  for(uint32_t i = 0; i < context.frame_c; i++){
    vkDestroyFramebuffer(context.l_dev, pipeline.framebuffers[i], NULL);
  }
  free(pipeline.framebuffers);

  vkDestroySemaphore(context.l_dev, pipeline.image_available, NULL);
  printf("render finsihed!\n");
  vkDestroySemaphore(context.l_dev, pipeline.render_finished, NULL);
  vkDestroyFence(context.l_dev, pipeline.in_flight, NULL);
  vkDestroyRenderPass(context.l_dev, pipeline.render_pass, NULL);
  
  vkDestroyPipeline(context.l_dev, pipeline.pipeline, NULL);
  vkDestroyPipelineLayout(context.l_dev, pipeline.layout, NULL);
  
 
  return 0;
}




