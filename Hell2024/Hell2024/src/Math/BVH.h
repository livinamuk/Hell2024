#pragma once
#include <vector>
#include <glm/glm.hpp>

struct Ray {
    glm::vec3 origin = glm::vec3(0);
    glm::vec3 direction = glm::vec3(0);
    float minDist = 0;
    float maxDist = 100;
};

namespace BVH {

    void UpdateBVH(std::vector<glm::vec3>& vertices);
    bool AnyHit(Ray& ray);

}