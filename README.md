# Gen5_Vulkan
## Vulkan Real-time Rendering

After an initial interest in recreating the visual inaccuracies of 5th generation consoles, I embarked on a journey to develop a real-time GTLF (Graphics Transformation and Lighting Format) file viewer. Staying true to the era's spirit, I chose the C99 programming standard, which was appropriate for consoles like the PS1 and N64.

As the project progressed, I discovered that this approach could provide innovative solutions to contemporary challenges of portability and optimization. To ensure cross-platform compatibility and tackle optimization issues, I'm actively testing and refining the renderer on OpenBSD and FreeBSD platforms. This effort is crucial in diagnosing optimization-related bugs that may not be immediately apparent on the more common x86/Windows system.

To reduce the overhead of handling multiple meshes and textures, data is packed tight into buffers which are read using offsets. Multiple instances of a model and variations of such models can re-use data through an interpretation of entity component systems. 

## Required Device Extensions
- **Core Features**
 - samplerAnisotropy
 - multiDrawIndirect
- **Descriptor Indexing Features**
 - descriptorBindingPartiallyBound
 - runtimeDescriptorArra

## Libraries
- **cgltf**: [cgltf](https://github.com/jkuhlmann/cgltf/tree/master) is used for loading GTLF files, enabling the real-time rendering of 3D models and scenes.

- **stb_image.h**: [stb_image.h](https://github.com/nothings/stb/blob/master/stb_image.h) is utilized for image loading, providing support for various image formats.

- **SDL2**: SDL2 is used for Wayland/Xorg support, as well as mouse and keyboard input handling.
