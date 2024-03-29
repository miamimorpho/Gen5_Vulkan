#include "solid.h"
#include <SDL2/SDL_vulkan.h>
#include <SDL2/SDL.h>

const int WIDTH = 640;
const int HEIGHT = 480;

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
  /* Create surface */
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
  printf("device_extensions...\n");
  for(uint32_t i = 0; i < ext_c; i++){
    layer_found = 0;
    for(uint32_t a = 0; a < avail_ext_c; a++){
      if(strcmp(cfg_device_extensions[i], avail_ext[a].extensionName) == 0){
	printf("\t%s\n", avail_ext[a].extensionName);
	layer_found = 1;
      }
    }
    if(!layer_found){
      printf("\t!%s not found!\n", cfg_device_extensions[i]);
      return 0;
    }
  }
  
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
  
  printf("physical device...\n\t%s\n", dev_properties.deviceName);      
  printf("device features\n");

  VkPhysicalDeviceFeatures dev_features;
  vkGetPhysicalDeviceFeatures(p_dev, &dev_features);  
  if(dev_features.geometryShader){
    // should be enabled by default
    printf("\t[X] geometry sampler\n");
  }else{
    printf("no suitable devices found!\n");
    return 1;
  }
  if(dev_features.samplerAnisotropy){
    /* enabled_dev_features.samplerAnisotropy = VK_TRUE;
       should be enabled by default */
    printf("\t[X] sampler anisotropy\n");
  }
  if(dev_features.multiDrawIndirect){
    //enabled_dev_features.multiDrawIndirect = VK_TRUE;
    printf("\t[X] multidrawing\n");
  }

  /* Enable Descriptor Indexing */
  VkPhysicalDeviceDescriptorIndexingFeatures descriptor_indexing_features = {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT,
  };

  VkPhysicalDeviceFeatures2 dev_features2 = {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
    .pNext = &descriptor_indexing_features,
  };
  
  printf("device features 2\n");
  vkGetPhysicalDeviceFeatures2(p_dev, &dev_features2 );
  if(descriptor_indexing_features.descriptorBindingPartiallyBound
     && descriptor_indexing_features.runtimeDescriptorArray)
    printf("\t[X] descriptor indexing\n");

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

  printf("validation_layers... \n");
  for (int i = 0; i < valid_layer_c; i++){
    layer_found = 0;
    for(uint32_t a = 0; a < avail_layer_c; a++){
      if(strcmp(cfg_validation_layers[i], avail_layers[a].layerName) == 0){
	printf("[X]\t%s\n", avail_layers[a].layerName);
	layer_found = 1;
      }
    }
    if(!layer_found){
      printf("\t!%s not found!\n", cfg_validation_layers[i]);
      return 0;
    }
  }
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
  const char *SDL_extensions[SDL_ext_c];
  SDL_Vulkan_GetInstanceExtensions(SDL_WINDOW, &SDL_ext_c, SDL_extensions);

  uint32_t ext_c = 0;
  vkEnumerateInstanceExtensionProperties(NULL, &ext_c, NULL);
  VkExtensionProperties ext[ext_c+1];
  vkEnumerateInstanceExtensionProperties(NULL, &ext_c, ext);

  int ext_found;
  printf("instance extensions...\n");
  for (uint32_t g = 0; g < SDL_ext_c; g++){
    ext_found = 0;
    for (uint32_t i = 0; i < ext_c; i++){
      if (strcmp(SDL_extensions[g], ext[i].extensionName) == 0){
	ext_found = 1;
	printf("[X]\t%s\n", ext[i].extensionName);
      }
    }
    if(!ext_found){
      printf("!extension not found! %s\n", SDL_extensions[g]);
      return 1;
    }
  }

  VkInstanceCreateInfo create_info = {
    .sType= VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pApplicationInfo = &app_info,
    .enabledExtensionCount = SDL_ext_c,
    .ppEnabledExtensionNames = SDL_extensions,
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
stage1_create(GfxContext *stage1)
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

int
stage1_destroy(GfxContext stage1){
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

