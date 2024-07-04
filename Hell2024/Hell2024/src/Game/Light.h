#pragma once
#include "../Common.h"

struct Light {

    struct BoundingVolume {
        glm::vec3 min {0};
        glm::vec3 max {0};
    };

    Light() {

    }
    Light(glm::vec3 position, glm::vec3 color, float radius, float strength, int type) {
        this->position = position;
        this->color = color;
        this->radius = radius;
        this->strength = strength;
        this->type = type;
    }

    glm::vec3 position;
    float strength = 1.0f;
    glm::vec3 color = glm::vec3(1, 0.7799999713897705, 0.5289999842643738);
    bool isDirty = false;
    float radius = 6.0f;
    int type = 0;

    std::vector<BoundingVolume> boundingVolumes;
};
