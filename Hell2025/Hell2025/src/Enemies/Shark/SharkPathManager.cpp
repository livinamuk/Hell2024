#include "SharkPathManager.h" 
#include "../Core/Audio.h"
#include "../Game/Scene.h"
#include "../Input/Input.h"
#include "../Util.hpp"

namespace SharkPathManager {
    std::vector<SharkPath> g_sharkPaths;
}

bool SharkPathManager::PathExists() {
    return (g_sharkPaths.size() > 0 && g_sharkPaths[0].m_points.size() > 3);
}

Shark* SharkPathManager::GetSharkByIndex(int index) {
    return &Scene::GetShark();
}

SharkPath* SharkPathManager::GetSharkPathByIndex(int index) {
    if (index >= 0 && index < g_sharkPaths.size()) {
        return &g_sharkPaths[index];
    }
    else {
        std::cout << "SharkPathManager::GetSharkPathByIndex() failed: " << index << " out of range of size" << g_sharkPaths.size() << "\n";
        return nullptr;
    }
}


void SharkPathManager::AddPath(std::vector<glm::vec3>& path) {
    SharkPath& sharkPath = g_sharkPaths.emplace_back();
    for (int i = 0; i < path.size(); i++) {
        SharkPathPoint& pathPoint = sharkPath.m_points.emplace_back();
        pathPoint.position = path[i];
    }
    std::cout << "Added new shark path with " << sharkPath.m_points.size() << "\n";

    // Calculate forward vector and left/right etc
    for (int i = 0; i < sharkPath.m_points.size()-1; i++) {
        SharkPathPoint& pathPoint = sharkPath.m_points[i];
        SharkPathPoint& nextPathPoint = sharkPath.m_points[i+1];
        pathPoint.forward = glm::normalize(nextPathPoint.position - pathPoint.position);
        pathPoint.right = glm::cross(pathPoint.forward, glm::vec3(0, 1, 0));
        pathPoint.left = -pathPoint.right;
    }
    SharkPathPoint& pathPoint = sharkPath.m_points[sharkPath.m_points.size()-1];
    SharkPathPoint& nextPathPoint = sharkPath.m_points[0];
    pathPoint.forward = glm::normalize(nextPathPoint.position - pathPoint.position);
    pathPoint.right = glm::cross(pathPoint.forward, glm::vec3(0, 1, 0));
    pathPoint.left = -pathPoint.right;
}

std::vector<SharkPath>& SharkPathManager::GetSharkPaths() {
    return g_sharkPaths;
}

void SharkPathManager::ClearSharkPaths() {
    g_sharkPaths.clear();
}

