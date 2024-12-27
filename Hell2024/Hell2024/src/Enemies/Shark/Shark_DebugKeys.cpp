#include "Shark.h"
#include "SharkPathManager.h"
#include "../Input/Input.h"
#include "../Game/Scene.h"

void Shark::CheckDebugKeyPresses() {

    if (Input::KeyPressed(HELL_KEY_6)) {
        SetPositionToBeginningOfPath();
        m_movementState = SharkMovementState::ARROW_KEYS;
        m_nextPathPointIndex = 1;
    }
    if (Input::KeyPressed(HELL_KEY_7)) {
        HuntPlayer(0);
    }
    if (Input::KeyPressed(HELL_KEY_8)) {
        m_movementState = SharkMovementState::FOLLOWING_PATH;
    }
    if (Input::KeyPressed(HELL_KEY_9)) {
        AnimatedGameObject* animatedGameObject = Scene::GetAnimatedGameObjectByIndex(m_animatedGameObjectIndex);
        animatedGameObject->PlayAnimation("Shark_Attack_Left_Quick", 1.0f);
        //m_drawPath = !m_drawPath;
    }

    //if (SharkPathManager::PathExists()) {
    //    SharkPath* path = SharkPathManager::GetSharkPathByIndex(0);
    //    SetPosition(path->m_points[0].position);
    //    m_nextPathPointIndex = 1;
    //    //TODO: shark->SetDirection(path->m_points[0].forward);
    //}
}
