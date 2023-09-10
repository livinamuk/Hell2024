# Voxel Based Global Illumination 

#### September 10 2023
Indirect light is working. It's all done with ray-triangle intersection tests at this point, need to look into faster methods for geometry visiblity checks and light propogation.

![Image](https://www.principiaprogrammatica.com/dump/vxgi.png)

#### September 4 2023

Direct lighting only. Will finish light propogation on the cpu, then move it all to compute if it looks any good. Rendered in OpenGL. Will likely eventually port to Vulkan and let them evaluate the raycasts for me, don't really see myself writing something faster, gonna have a crack in GL compute first tho.

![Image](https://www.principiaprogrammatica.com/dump/Voxel.jpg)
