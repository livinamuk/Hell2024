
#include "Engine.h"
#include "Core/VoxelWorld.h"
#include <vector>
#include "Math.h"

#include <cfloat>
#include <iostream>
#include "Common.h"

double _bin_size = 1;


std::vector<vec3i> voxel_traversal(glm::vec3 ray_start, glm::vec3 ray_end) {
    std::vector<vec3i> visited_voxels;

    // This id of the first/current voxel hit by the ray.
    // Using floor (round down) is actually very important,
    // the implicit int-casting will round up for negative numbers.

    vec3i current_voxel(std::floor(ray_start.x / _bin_size),
        std::floor(ray_start.y / _bin_size),
        std::floor(ray_start.z / _bin_size));

        // The id of the last voxel hit by the ray.
        // TODO: what happens if the end point is on a border?
    vec3i last_voxel(std::floor(ray_end.x / _bin_size),
        std::floor(ray_end.y / _bin_size),
        std::floor(ray_end.z / _bin_size));

    // Compute normalized ray direction.
    glm::vec3 ray = normalize(ray_end - ray_start);
    //ray.normalize();

    // In which direction the voxel ids are incremented.
    double stepX = (ray.x >= 0) ? 1 : -1; // correct
    double stepY = (ray.y >= 0) ? 1 : -1; // correct
    double stepZ = (ray.z >= 0) ? 1 : -1; // correct

    // Distance along the ray to the next voxel border from the current position (tMaxX, tMaxY, tMaxZ).
    double next_voxel_boundary_x = (current_voxel.x + stepX) * _bin_size; // correct
    double next_voxel_boundary_y = (current_voxel.y + stepY) * _bin_size; // correct
    double next_voxel_boundary_z = (current_voxel.z + stepZ) * _bin_size; // correct

    // tMaxX, tMaxY, tMaxZ -- distance until next intersection with voxel-border
    // the value of t at which the ray crosses the first vertical voxel boundary
    double tMaxX = (ray.x != 0) ? (next_voxel_boundary_x - ray_start.x) / ray.x : DBL_MAX; //
    double tMaxY = (ray.y != 0) ? (next_voxel_boundary_y - ray_start.y) / ray.y : DBL_MAX; //
    double tMaxZ = (ray.z != 0) ? (next_voxel_boundary_z - ray_start.z) / ray.z : DBL_MAX; //

    // tDeltaX, tDeltaY, tDeltaZ --
    // how far along the ray we must move for the horizontal component to equal the width of a voxel
    // the direction in which we traverse the grid
    // can only be FLT_MAX if we never go in that direction
    double tDeltaX = (ray.x != 0) ? _bin_size / ray.x * stepX : DBL_MAX;
    double tDeltaY = (ray.y != 0) ? _bin_size / ray.y * stepY : DBL_MAX;
    double tDeltaZ = (ray.z != 0) ? _bin_size / ray.z * stepZ : DBL_MAX;

    vec3i diff(0, 0, 0);
    bool neg_ray = false;
    if (current_voxel.x != last_voxel.x && ray.x < 0) { diff.x--; neg_ray = true; }
    if (current_voxel.y != last_voxel.y && ray.y < 0) { diff.y--; neg_ray = true; }
    if (current_voxel.z != last_voxel.z && ray.z < 0) { diff.z--; neg_ray = true; }
    visited_voxels.push_back(current_voxel);
    if (neg_ray) {
        current_voxel += diff;
        visited_voxels.push_back(current_voxel);
    }

    while (last_voxel != current_voxel) {
        if (tMaxX < tMaxY) {
            if (tMaxX < tMaxZ) {
                current_voxel.x += stepX;
                tMaxX += tDeltaX;
            }
            else {
                current_voxel.z += stepZ;
                tMaxZ += tDeltaZ;
            }
        }
        else {
            if (tMaxY < tMaxZ) {
                current_voxel.y += stepY;
                tMaxY += tDeltaY;
            }
            else {
                current_voxel.z += stepZ;
                tMaxZ += tDeltaZ;
            }
        }
        visited_voxels.push_back(current_voxel);
    }
    return visited_voxels;
}


int main() {

   /*glm::vec3 ray_start(2, 23, 2);
    glm::vec3 ray_end(77, 2, 2);
    std::cout << "Voxel size: " << _bin_size << std::endl;
    std::cout << "Voxel ID's from start to end:" << std::endl;
    std::vector<vec3i> ids = voxel_traversal(ray_start, ray_end);

    for (auto& i : ids) {
        std::cout << "" << i << std::endl;
    }
    std::cout << "Total number of traversed voxels: " << ids.size() << std::endl;*/
 //   return 0;

   Engine::Run();
    return 0;
}


