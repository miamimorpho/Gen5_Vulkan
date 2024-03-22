# Gen5_Vulkan
## Vulkan Real-time Rendering

Vulkan renderer written in C99

**Progress**
 - FPS Camera Movement
 - Load in multiple models of multiple textures
 - Descriptor sets organised by update frequency
 - gltf/glb support

## Required Device Features
**Core**
 - samplerAnisotropy
 - multiDrawIndirect
**Descriptor Indexing**
 - descriptorBindingPartiallyBound
 - runtimeDescriptorArray

## Libraries
- **cgltf**: [cgltf](https://github.com/jkuhlmann/cgltf/tree/master) is used for loading GTLF files, enabling the real-time rendering of 3D models and scenes.

- **stb_image.h**: [stb_image.h](https://github.com/nothings/stb/blob/master/stb_image.h) is utilized for image loading, providing support for various image formats.

- **SDL2**: SDL2 is used for Wayland/Xorg support, as well as mouse and keyboard input handling.
