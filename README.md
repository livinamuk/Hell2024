# HELL ENGINE: rewrite one billion++

#### December 27 2023
This codebase began as a CPU raycaster to explore the viability of a voxel based idea I had for global illumination. The voxel idea didn't work out in the end, the voxel grid caused lighting artefacts, but rewriting it to use a pointcloud did and here we are. This is now the new Hell Engine, the engine I'm writing to house a splitscreen roguelike/deathmatch survival horror game.       

![Image](https://www.principiaprogrammatica.com/dump/ChristmasShot.png)

```
CONTROLS:
WSAD: movement
Left Mouse: fire
Space bar:  jump
Q: cycle weapons
E: interact
R: reload
Left ctrl: crouch
F: fullscreen
Z: previous render mode
X: next render mode
B: cycle debug lines
N: reload map
L: show lights
P: force realtime indirect lighting updates
V: splitscreen
C: switch player
Y: show probes


Build in release. Debug doesn't have libs/dlls setup correctly.
```

#### November 14 2023
Huge improvements since the entries below. I scrapped the voxel thing entirely because it was causing lighting artefacts for geometry that didn't fit nicely into the voxel grid, most importantly swinging doors. It now calculates direct lighting for an approximated point cloud of the scene which is in turn used to propogate light through a 3D grid to simulate bounced light, both these passes are performed with raytracing in custom compute shaders. I've got skeletal animation too now and we're running realtime baby.     

![Image](https://www.principiaprogrammatica.com/dump/SHITT2.jpg)

#### September 12 2023
Things are looking promising. I really need to move to Vulkan so I can do visiblity checks for each pixel against any probe that may contribute indirect light to it. Gonna see how far I can push this in GL first though. All direct and indirect light is still calculated on the CPU, would be nice to move to compute but there's really no point if I'm moving to VK soon. I'll work on voxelizing regular mesh next I think.

![Image](https://www.principiaprogrammatica.com/dump/vxgi2.jpg)

#### September 10 2023
Indirect light is working. It's all done with ray-triangle intersection tests at this point, need to look into faster methods for geometry visiblity checks and light propogation.

![Image](https://www.principiaprogrammatica.com/dump/vxgi.png)

#### September 4 2023

I started a new project today, to explore using voxels to calculate and render indirect diffuse light at realtime speeds. So far direct lighting only. Will finish light propogation on the cpu, then move it all to compute if it looks any good. Rendered in OpenGL. Will likely eventually port to Vulkan and let them evaluate the raycasts for me, don't really see myself writing something faster, gonna have a crack in GL compute first tho.

![Image](https://www.principiaprogrammatica.com/dump/Voxel.jpg)
