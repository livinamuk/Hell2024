#include "SharkLogic.h" 
#include "../Core/Audio.h"
#include "../Game/Scene.h"
#include "../Input/Input.h"
#include "../Util.hpp"

namespace SharkLogic {
    std::vector<SharkPath> g_sharkPaths;
}
void SharkLogic::Update(float deltaTime) {
    
    Shark* shark = GetSharkByIndex(0);

    //if (Input::KeyPressed(HELL_KEY_6)) {
    //    Audio::PlayAudio(AUDIO_SELECT, 1.00f);
    //    g_sharkPaths.clear();
    //}
    if (Input::KeyPressed(HELL_KEY_6)) {
        SetSharkToBeginningOfPath();
        shark->m_sharkMovementState = SharkMovementState::STOPPED;
        shark->m_nextPathPointIndex = 1;
    }
    if (Input::KeyPressed(HELL_KEY_7)) {
        shark->m_sharkMovementState = SharkMovementState::FOLLOWING_PATH;
    }

}

bool SharkLogic::PathExists() {
    return (g_sharkPaths.size() > 0 && g_sharkPaths[0].m_points.size() > 3);
}

Shark* SharkLogic::GetSharkByIndex(int index) {
    return &Scene::GetShark();
}

SharkPath* SharkLogic::GetSharkPathByIndex(int index) {
    if (index >= 0 && index < g_sharkPaths.size()) {
        return &g_sharkPaths[index];
    }
    else {
        std::cout << "SharkLogic::GetSharkPathByIndex() failed: " << index << " out of range of size" << g_sharkPaths.size() << "\n";
        return nullptr;
    }
}

void SharkLogic::SetSharkToBeginningOfPath() {
    Shark* shark = GetSharkByIndex(0);
    if (PathExists()) {
        SharkPath* path = GetSharkPathByIndex(0);
        shark->SetPosition(path->m_points[0].position);
        //TODO: shark->SetDirection(path->m_points[0].forward);
    }
}

void SharkLogic::AddPath(std::vector<glm::vec3>& path) {
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

std::vector<SharkPath>& SharkLogic::GetSharkPaths() {
    return g_sharkPaths;
}

void SharkLogic::ClearSharkPaths() {
    g_sharkPaths.clear();
}

