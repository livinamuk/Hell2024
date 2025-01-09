#include "Shark.h"
#include "SharkPathManager.h"
#include "../Input/Input.h"
#include "../Game/Game.h"
#include "../Game/Scene.h"
#include "../Math/LineMath.hpp"

std::string Shark::SharkMovementStateToString(SharkMovementState state) {
    switch (state) {
    case SharkMovementState::FOLLOWING_PATH: return "FOLLOWING_PATH";
    case SharkMovementState::FOLLOWING_PATH_ANGRY: return "FOLLOWING_PATH_ANGRY";
    case SharkMovementState::STOPPED: return "STOPPED";
    case SharkMovementState::ARROW_KEYS: return "ARROW_KEYS";
    case SharkMovementState::HUNT_PLAYER: return "HUNT_PLAYER";
    default: return "UNDEFINED";
    }
}

std::string Shark::SharkHuntingStateToString(SharkHuntingState state) {
    switch (state) {
    case SharkHuntingState::BITING_PLAYER: return "BITING_PLAYER";
    case SharkHuntingState::CHARGE_PLAYER: return "CHARGE_PLAYER";
    case SharkHuntingState::UNDEFINED: return "UNDEFINED";
    default: return "UNDEFINED";
    }
}

std::string Shark::GetDebugText() {
    std::string debugText = "";
   //
   //debugText += "head positon: " + Util::Vec3ToString(GetHeadPosition2D()) + "\n";
   //debugText += "m_nextPathPointIndex: " + std::to_string(m_nextPathPointIndex) + "\n";
   //debugText += "m_targetPosition: " + Util::Vec3ToString(m_targetPosition) + "\n";
   //debugText += "m_rotation: " + std::to_string(m_rotation) + "\n";
   //debugText += "GetMouthPosition3D(): " + Util::Vec3ToString(GetMouthPosition3D()) + "\n";
   //debugText += "GetDistanceMouthToTarget3D(): " + std::to_string(GetDistanceMouthToTarget3D()) + "\n";
   //debugText += "GetDotMouthDirectionToTarget3D(): " + std::to_string(GetDotMouthDirectionToTarget3D()) + "\n";
   //debugText += "m_hasBitPlayer(): " + std::to_string(m_hasBitPlayer) + "\n";
   //debugText += "m_huntedPlayerIndex: " + std::to_string(m_huntedPlayerIndex) + "\n";
   //debugText += "GetForwardVector(): " + Util::Vec3ToString(GetForwardVector()) + "\n";
   //debugText += "GetTargetPosition2D(): " + Util::Vec3ToString(GetTargetPosition2D()) + "\n";
   //debugText += "GetEvadePoint2D(): " + Util::Vec3ToString(GetEvadePoint2D()) + "\n";

    //debugText += "GetAnimationFrameNumber(): " + std::to_string(GetAnimationFrameNumber()) + "\n";
    //debugText += "IsBehindEvadePoint(playerPosition): " + Util::BoolToString(IsBehindEvadePoint(Game::GetPlayerByIndex(0)->GetViewPos())) + "\n";
    

    debugText += "Shark Movement State: " + SharkMovementStateToString(m_movementState) + "\n";
    debugText += "Shark Hunting State: " + SharkHuntingStateToString(m_huntingState) + "\n";
    debugText += "\nShark health: " + std::to_string(m_health) + "\n";

    //if (m_playerSafe) {
    //    debugText += "\nSafe: YES\n";
    //}
    //else {
    //    debugText += "\nSafe: NO\n";
    //}
    return debugText;
}

void Shark::CheckDebugKeyPresses() {
    if (Input::KeyPressed(HELL_KEY_6)) {
        Reset();
        m_movementState = SharkMovementState::FOLLOWING_PATH;
    }
    if (Input::KeyPressed(HELL_KEY_7)) {
        m_movementState = SharkMovementState::ARROW_KEYS;
    }
}

std::vector<Vertex> Shark::GetDebugPointVertices() {
    std::vector<Vertex> vertices;
    if (m_drawDebug) {
        // Spine segments
        for (int i = 0; i < SHARK_SPINE_SEGMENT_COUNT; i++) {
            vertices.push_back(Vertex(GetSpinePosition(i), WHITE));
        }
        // Evade point
        vertices.push_back(Vertex(GetEvadePoint3D(), RED));
        
        // Forward Vector
        glm::vec3 lineStart = GetHeadPosition2D() + (GetForwardVector() * 1.0f);
        glm::vec3 lineEnd = GetHeadPosition2D();
        vertices.push_back(Vertex(lineStart, GREEN));
        vertices.push_back(Vertex(lineEnd, GREEN));
    }
    return vertices;
}

std::vector<Vertex> Shark::GetDebugLineVertices() {    
    std::vector<Vertex> vertices;
    if (m_drawDebug) {
        // Spine segments
        for (int i = 0; i < SHARK_SPINE_SEGMENT_COUNT - 1; i++) {
            vertices.push_back(Vertex(GetSpinePosition(i), WHITE));
            vertices.push_back(Vertex(GetSpinePosition(i + 1), WHITE));
        }
        // Forward Vector
        glm::vec3 lineStart = GetHeadPosition2D() + (GetForwardVector() * 1.0f);
        glm::vec3 lineEnd = GetHeadPosition2D();
        vertices.push_back(Vertex(lineStart, GREEN));
        vertices.push_back(Vertex(lineEnd, GREEN));
    }
    return vertices;
}
