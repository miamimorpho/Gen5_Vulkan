## Vulkan Playground

renderer written in C99 + Vulkan

**Roadmap**
 - [X] Load in multiple models of multiple textures
 - [X] Descriptor sets organised by update frequency
 - [X] FPS Camera Movement
 - [X] directional flat lighting
 - [X] basic simplex noise terrain generation
 - [X] bdf font loading, ASCII text rendering
 - [ ] directional sprites
 - [ ] scripting language integration

**Required Device Features**
 - samplerAnisotropy
 - multiDrawIndirect

**Device Features 2**
 - descriptorBindingPartiallyBound
 - runtimeDescriptorArray

** Dependencies **
- [Vulkan Memory Allocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)
- gflw
- [FastNoiseLite](https://github.com/Auburn/FastNoiseLite/tree/master/C)
- [cgltf](https://github.com/jkuhlmann/cgltf/tree/master)
- [stb_image.h](https://github.com/nothings/stb/blob/master/stb_image.h)


