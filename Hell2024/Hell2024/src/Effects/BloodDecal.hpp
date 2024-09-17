#pragma once
#include "HellCommon.h"

struct BloodDecal {
    Transform transform;
    Transform localOffset;
    int type;
    glm::mat4 modelMatrix;

    BloodDecal(Transform transform, int type) {
        this->transform = transform;
        this->type = type;
        if (type != 2) {
            localOffset.position.z = 0.55f;
            transform.scale = glm::vec3(2.0f);
            transform.rotation.y += Util::RandomFloat(-0.5f, 0.5f);
        }
        else {
            localOffset.rotation.y = Util::RandomFloat(0, HELL_PI * 2);
            transform.scale = glm::vec3(1.5f);
        }
        modelMatrix = transform.to_mat4() * localOffset.to_mat4();
    }
};