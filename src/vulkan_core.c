#include "vulkan_public.h"
#include "vulkan_util.h"
#include <string.h>

const int WIDTH = 640;
const int HEIGHT = 480;

/* Device Extensions required to run */
const char *cfg_device_extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, };
/* Debug/Validation Layers */
const int enable_debug = 1;
const char *cfg_validation_layers[] = { "VK_LAYER_KHRONOS_validation", };
const VkFormat cfg_format = VK_FORMAT_B8G8R8A8_SRGB;

/* START OF CODE */

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

int
surface_create(GfxContext *context)
{
  if(glfwCreateWindowSurface(VULKAN_INSTANCE, context->window,
			     NULL, &VULKAN_SURFACE)){
    printf("failed to create glfw surface");
    glfwDestroyWindow(context->window);
    glfwTerminate();
    return 1;
  }
  
  /* Test for queue presentation support */
  VkBool32 present_support = VK_FALSE;
  vkGetPhysicalDeviceSurfaceSupportKHR
    (context->p_dev, queue_index(context->p_dev, VK_QUEUE_GRAPHICS_BIT),
     VULKAN_SURFACE, &present_support);
  if(present_support == VK_FALSE){
    printf("!no device presentation support!\n");
    return -2;
  }
  
  /* Find colour format */
  uint32_t format_c;
  vkGetPhysicalDeviceSurfaceFormatsKHR(context->p_dev, VULKAN_SURFACE,
				       &format_c, NULL);
  VkSurfaceFormatKHR formats[format_c];
  vkGetPhysicalDeviceSurfaceFormatsKHR(context->p_dev, VULKAN_SURFACE,
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
  vkGetPhysicalDeviceSurfacePresentModesKHR(context->p_dev, VULKAN_SURFACE,
					    &mode_c, NULL);
  VkPresentModeKHR modes[mode_c];
  vkGetPhysicalDeviceSurfacePresentModesKHR(context->p_dev, VULKAN_SURFACE,
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
swapchain_create(GfxContext *context)
{
  context->extent.width = WIDTH;
  context->extent.height = HEIGHT;

  VkSurfaceCapabilitiesKHR capable;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR
    (context->p_dev, VULKAN_SURFACE, &capable);

  if (capable.currentExtent.width != context->extent.width
     || capable.currentExtent.height != context->extent.height) {
    context->extent.width =
      context->extent.width > capable.maxImageExtent.width ?
      capable.maxImageExtent.width : context->extent.width;

    context->extent.width =
      context->extent.width < capable.minImageExtent.width ?
      capable.minImageExtent.width : context->extent.width;
    
    context->extent.height =
      context->extent.height > capable.maxImageExtent.height ?
      capable.maxImageExtent.height : context->extent.height;

    context->extent.height =
      context->extent.height < capable.minImageExtent.height ?
      capable.minImageExtent.height : context->extent.height;
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
    .imageExtent = context->extent,
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

  if (vkCreateSwapchainKHR(context->l_dev, &create_info, NULL,
			   &context->swapchain_handle)
     != VK_SUCCESS){
    printf("!failed to create swap chain!\n");
    return 1;
  }

  /* Create Image Views */
  vkGetSwapchainImagesKHR
    (context->l_dev, context->swapchain_handle, &context->frame_c, NULL);
  context->frames = malloc(context->frame_c * sizeof(VkImage));
  context->views = malloc(context->frame_c * sizeof(VkImageView));

  vkGetSwapchainImagesKHR
    (context->l_dev, context->swapchain_handle,
     &context->frame_c, context->frames);

  for (uint32_t i = 0; i < context->frame_c; i++) {
    image_view_create(context->l_dev, context->frames[i],
		      &context->views[i], cfg_format,
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

int allocator_create(GfxContext context, VmaAllocator* allocator){

  VmaAllocatorCreateInfo allocator_info = {
    .physicalDevice = context.p_dev,
    .device = context.l_dev,
    .instance = VULKAN_INSTANCE,
  };
  vmaCreateAllocator(&allocator_info, allocator);

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

  uint32_t ext_c;
  const char** ext = glfwGetRequiredInstanceExtensions(&ext_c);

  uint32_t vk_ext_c = 0;
  vkEnumerateInstanceExtensionProperties(NULL, &vk_ext_c, NULL);
  VkExtensionProperties vk_ext[vk_ext_c+1];
  vkEnumerateInstanceExtensionProperties(NULL, &vk_ext_c, vk_ext);

  int ext_found;
  printf("instance extensions...\n");
  for (uint32_t g = 0; g < ext_c; g++){
    ext_found = 0;
    for (uint32_t i = 0; i < vk_ext_c; i++){
      if (strcmp(ext[g], vk_ext[i].extensionName) == 0){
	ext_found = 1;
	printf("[X]\t%s\n", vk_ext[i].extensionName);
      }
    }
    if(!ext_found){
      printf("!extension not found! %s\n", ext[g]);
      return 1;
    }
  }

  VkInstanceCreateInfo create_info = {
    .sType= VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pApplicationInfo = &app_info,
    .enabledExtensionCount = ext_c,
    .ppEnabledExtensionNames = ext,
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

int
render_pass_create(GfxContext context, VkRenderPass* render_pass )
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
depth_create(GfxContext context, GfxImage* depth)
{
  VkFormat format;
  if(depth_format_check(context.p_dev, &format) == 1) return 1;
  
  image_create(context, &depth->handle, &depth->allocation,
	       VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
	       format, context.extent.width, context.extent.height);

  image_view_create(context.l_dev, depth->handle, &depth->view,
		    format, VK_IMAGE_ASPECT_DEPTH_BIT);


  depth->sampler = VK_NULL_HANDLE;
  
  return 0;
}

int
framebuffers_create(GfxContext *context)
{
  context->framebuffers = malloc(context->frame_c * sizeof(VkFramebuffer));

  for(size_t i = 0; i < context->frame_c; i++){
    VkImageView attachments[] = {
      context->views[i],
      context->depth.view,
    };

    VkFramebufferCreateInfo framebuffer_info = {
      .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      .renderPass = context->render_pass,
      .attachmentCount = 2,
      .pAttachments = attachments,
      .width = context->extent.width,
      .height = context->extent.height,
      .layers = 1,
    };

    if(vkCreateFramebuffer(context->l_dev, &framebuffer_info,
			   NULL, &context->framebuffers[i] )
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
descriptor_pool_create(GfxContext context, VkDescriptorPool* descriptor_pool)
{
  VkDescriptorPoolSize pool_sizes[2];
  pool_sizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  pool_sizes[0].descriptorCount = MAX_SAMPLERS;
  
  pool_sizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  pool_sizes[1].descriptorCount = context.frame_c;
 
  VkDescriptorPoolCreateInfo pool_info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    .poolSizeCount = 2,
    .pPoolSizes = pool_sizes,
    .maxSets = MAX_SAMPLERS + context.frame_c,
    .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT,
  };

  if(vkCreateDescriptorPool(context.l_dev, &pool_info, NULL,
			    descriptor_pool)
     != VK_SUCCESS) {
    printf("failed to create descriptor pool\n");
    return 1;
  }
  return 0;
}


int
sync_bits_create(GfxContext* context)
{
  VkSemaphoreCreateInfo semaphore_info = {
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
  };

  VkFenceCreateInfo fence_info = {
    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    .flags = VK_FENCE_CREATE_SIGNALED_BIT,
  };
  
  if(vkCreateSemaphore(context->l_dev, &semaphore_info,
		       NULL, &context->render_finished) != VK_SUCCESS ||
     vkCreateSemaphore(context->l_dev, &semaphore_info,
		       NULL, &context->image_available) != VK_SUCCESS ||
     vkCreateFence(context->l_dev, &fence_info,
		   NULL, &context->in_flight) != VK_SUCCESS ){
    printf("!failed to create sync objects!\n");
    return 1;
  }

  return 0;
}

int context_create(GfxContext *context)
{
  if(!glfwInit()){
    printf("glfw init failure\n");
    return 1;
  }
 
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  context->window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan/GLFW", NULL, NULL);
  glfwMakeContextCurrent(context->window);
  glfwSetInputMode(context->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glfwSetCursorPos(context->window, 0,0);

  if (glfwRawMouseMotionSupported())
    glfwSetInputMode(context->window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
 
  
  if(instance_create() > 0)
    return 1;

  if(devices_create(context) > 0) return 2;

  allocator_create(*context, &context->allocator);
 
  /* Shared Pipeline Features Begin */
  
  command_pool_create(*context, &context->command_pool);

  if(surface_create(context) > 0) return 3;

  if(swapchain_create(context) > 0) return 4;

  render_pass_create(*context, &context->render_pass);

  depth_create(*context, &context->depth);

  framebuffers_create(context);

  command_buffer_create(*context, &context->command_buffer);

  descriptor_pool_create(*context, &context->descriptor_pool);

  sync_bits_create(context);

  return 0;
}

int context_destroy(GfxContext context)
{
  vkDestroySemaphore(context.l_dev, context.image_available, NULL);
  vkDestroySemaphore(context.l_dev, context.render_finished, NULL);
  vkDestroyFence(context.l_dev, context.in_flight, NULL);
  
  vkDestroyDescriptorSetLayout(context.l_dev,
			       context.texture_descriptors_layout, NULL);
  
  vkDestroyDescriptorPool(context.l_dev, context.descriptor_pool, NULL);
  vkDestroyCommandPool(context.l_dev, context.command_pool, NULL);
 
  
  for(uint32_t i = 0; i < context.frame_c; i++){
    vkDestroyFramebuffer(context.l_dev, context.framebuffers[i], NULL);
  }
  free(context.framebuffers);
  
  image_destroy(context, context.depth);

  vkDestroyRenderPass(context.l_dev, context.render_pass, NULL);
  
  for(uint32_t i = 0; i < context.frame_c; i++)
    vkDestroyImageView(context.l_dev, context.views[i], NULL);

  free(context.views);
  free(context.frames);
 
  vkDestroySwapchainKHR(context.l_dev, context.swapchain_handle, NULL);

  vmaDestroyAllocator(context.allocator);
  
  vkDestroyDevice(context.l_dev, NULL);
  vkDestroySurfaceKHR(VULKAN_INSTANCE, VULKAN_SURFACE, NULL);
  vkDestroyInstance(VULKAN_INSTANCE, NULL);

  glfwDestroyWindow(context.window);
  glfwTerminate();
  
  return 0;
}



