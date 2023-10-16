#version 420 core
in vec4 worldPos;

layout(rgba16, binding = 0)uniform writeonly image3D tex3D;

#define VOXEL_SIZE 0.2

uniform int axis;

void main() {

    vec3 pos = worldPos.xyz;
    
    // looking into Z neg
    if (axis == 0)  {
        pos.x += (VOXEL_SIZE * 0.5);
        pos.y += (VOXEL_SIZE * 0.5);
        pos.z += (VOXEL_SIZE * 0.0);
        int x = int(pos.x / VOXEL_SIZE);
        int y = int(pos.y / VOXEL_SIZE);
        int z = int(pos.z / VOXEL_SIZE);
        imageStore(tex3D, ivec3(x, y, z), vec4(1,0,1,1));
    }
    else if (axis == 1)  {
        pos.x += (VOXEL_SIZE * 0.5);
        pos.y += (VOXEL_SIZE * 0.5);
        pos.z += (VOXEL_SIZE * 0.5);
        int x = int(pos.x / VOXEL_SIZE);
        int y = int(pos.y / VOXEL_SIZE);
        int z = int(pos.z / VOXEL_SIZE);
        imageStore(tex3D, ivec3(x, y, z), vec4(1,0,1,1));
    }
}