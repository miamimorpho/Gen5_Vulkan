# Gen5_Vulkan
## Vulkan Real-time Rendering

After an initial interest in recreating the visual inaccuracies of 5th generation consoles, I embarked on a journey to develop a real-time GLTF (GL Transmission Format) file viewer. Staying true to the era's spirit, I chose the C99 programming standard, which was appropriate for consoles like the PS1 and N64. Getting to learn the Vulkan API has given me a finer grain control over intentional/accidental rendering glitches and new ways to explore the 'charm' found in imperfection. As the project has progressed my focus has shifted toward analysing contemporary rendering tecniques and writing portable and open-source implementations for educational purposes.

To reduce the overhead of handling multiple meshes and textures, data is packed tight into buffers which are read using offsets. Multiple instances of a model and variations of such models can re-use data through an interpretation of entity component systems. The overall architecture has been inspired by Doom Eternals implementation of *bindless textures* and primarily *indirect multidrawing* like used in Rainbow Six Siege, Assasins Creed and EA's Frostbite Engine. 

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
