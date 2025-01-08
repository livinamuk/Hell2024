#include "Shark.h"
#include "SharkPathManager.h"
#include "../Input/Input.h"
#include "../Game/Game.h"
#include "../Game/Scene.h"
#include "../Math/LineMath.hpp"

std::string Shark::SharkMovementStateToString(SharkMovementState state) {
    switch (state) {
    case SharkMovementState::FOLLOWING_PATH: return "FOLLOWING_PATH";
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
    default: return "UNDEFINED";
    }
}

std::string Shark::GetDebugText() {
    std::string debugText = "";
   //debugText += "Movement State: " + SharkMovementStateToString(m_movementState) + "\n";
   //debugText += "Hunting State: " + SharkHuntingStateToString(m_huntingState) + "\n";
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

    debugText += "GetAnimationFrameNumber(): " + std::to_string(GetAnimationFrameNumber()) + "\n";
    debugText += "IsBehindEvadePoint(playerPosition): " + Util::BoolToString(IsBehindEvadePoint(Game::GetPlayerByIndex(0)->GetViewPos())) + "\n";
    

    debugText += "\nm_health: " + std::to_string(m_health) + "\n";

    if (m_playerSafe) {
        debugText += "\nSafe: YES\n";
    }
    else {
        debugText += "\nSafe: NO\n";
    }
    return debugText;
}

void Shark::CheckDebugKeyPresses() {
    if (Input::KeyPressed(HELL_KEY_6)) {
        SetPositionToBeginningOfPath();
        m_movementState = SharkMovementState::ARROW_KEYS;
        m_nextPathPointIndex = 1;
        m_forward = glm::vec3(0, 0, 1.0f);
        m_health = SHARK_HEALTH_MAX;
        m_huntingState = SharkHuntingState::CHARGE_PLAYER;
        m_huntedPlayerIndex = -1;
        PlayAndLoopAnimation("Shark_Swim", 1.0f);
    }
    if (Input::KeyPressed(HELL_KEY_7)) {
        HuntPlayer(0);
    }
    if (Input::KeyPressed(HELL_KEY_8)) {
        m_movementState = SharkMovementState::FOLLOWING_PATH;
    }
    if (Input::KeyPressed(HELL_KEY_9)) {
        PlayAndLoopAnimation("Shark_Attack_Left_Quick", 1.0f);
    }
    if (Input::KeyPressed(HELL_KEY_0)) {
        HuntPlayer(0);
       // PlayAndLoopAnimation("Shark_Swim", 1.0f);
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
        //glm::vec3 closestPointOnLine = LineMath::ClosestPointOnLine(lineStart, lineEnd, GetTargetPosition2D());
        //
        //
        //vertices.push_back(Vertex(lineStart, YELLOW));
        //vertices.push_back(Vertex(lineEnd, YELLOW));
        //vertices.push_back(Vertex(closestPointOnLine, GREEN));
        //vertices.push_back(Vertex(GetTargetPosition2D(), GREEN));
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

        //glm::vec3 lineStart = GetEvadePoint2D() + (GetForwardVector() * 10.0f);
        //glm::vec3 lineEnd = GetEvadePoint2D() - (GetForwardVector() * 10.0f);
        //glm::vec3 closestPointOnLine = LineMath::ClosestPointOnLine(lineStart, lineEnd, GetTargetPosition2D());
        //
        //
        //vertices.push_back(Vertex(lineStart, YELLOW));
        //vertices.push_back(Vertex(lineEnd, YELLOW));
        //vertices.push_back(Vertex(closestPointOnLine, GREEN));
        //vertices.push_back(Vertex(GetTargetPosition2D(), GREEN));


        // Forward Vector
        //vertices.push_back(Vertex(GetSpinePosition(0), RED));
        //vertices.push_back(Vertex(GetSpinePosition(0) + (m_forward * 2.0f), RED));
        //// Target Vector
        //vertices.push_back(Vertex(GetSpinePosition(0), GREEN));
        //vertices.push_back(Vertex(GetSpinePosition(0) + (GetTargetDirection2D() * 2.0f), GREEN));
        //// Mouth forward Vector
        //vertices.push_back(Vertex(GetMouthPosition3D(), BLUE));
        //vertices.push_back(Vertex(GetMouthPosition3D() + (GetMouthForwardVector() * 2.0f), BLUE));
    }
    return vertices;
}
