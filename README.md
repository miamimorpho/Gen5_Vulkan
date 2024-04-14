## SolidGfx

renderer written in C99 + Vulkan

**Progress**
 - [X] Load in multiple models of multiple textures
 - [X] Descriptor sets organised by update frequency
 - [X] FPS Camera Movement
 - [X] directional flat lighting
 - [X] basic simplex noise terrain generation
 - [ ] collision detection
 - [ ] directional sprites

**Device Features**
 - samplerAnisotropy
 - multiDrawIndirect

**Device Features 2**
 - descriptorBindingPartiallyBound
 - runtimeDescriptorArray

## Libraries

- **SDL2**: SDL2 is used for Wayland/Xorg support, as well as mouse and keyboard input handling.

- **FastNoiseLite** [FastNoiseLite](https://github.com/Auburn/FastNoiseLite/tree/master/C)

- **cgltf**: [cgltf](https://github.com/jkuhlmann/cgltf/tree/master) is used for loading GTLF files, enabling the real-time rendering of 3D models and scenes.

- **stb_image.h**: [stb_image.h](https://github.com/nothings/stb/blob/master/stb_image.h) is utilized for image loading, providing support for various image formats.


