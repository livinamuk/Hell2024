#include "Editor.h" 
#include "../Game/Game.h"
#include "../Game/Scene.h"
#include "../Util.hpp"

std::string Editor::GetDebugText() {
    std::string debugText = "";
    debugText += "Map Editor Mode: " + Util::EditorModeToString(Game::g_editorMode) + "\n\n";
    debugText += Scene::GetShark().GetDebugText();
    return debugText;
}
