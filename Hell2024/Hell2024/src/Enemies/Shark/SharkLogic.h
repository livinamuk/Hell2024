#pragma once
#include "../../Common/HellCommon.h"
#include "Shark.h"

struct SharkPathPoint {
    glm::vec3 position;
    glm::vec3 forward = glm::vec3(0, 0, 1);
    glm::vec3 left = glm::vec3(-1, 0, 0);
    glm::vec3 right = glm::vec3(1, 0, 0);
};

struct SharkPath {
    std::vector<SharkPathPoint> m_points;
};

namespace SharkLogic {
    void AddPath(std::vector<glm::vec3>& path);
    void Update(float deltaTime);

    std::vector<SharkPath>& GetSharkPaths();
    void ClearSharkPaths();
    void SetSharkToBeginningOfPath();
    Shark* GetSharkByIndex(int index);
    SharkPath* GetSharkPathByIndex(int index);
    bool PathExists();

};