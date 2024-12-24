#include "Editor.h" 
#include "../Game/Game.h"
#include "../Util.hpp"

namespace Editor {
    std::string g_debugText = "";
}

std::string& Editor::GetDebugText() {
    return g_debugText;
}

void Editor::UpdateDebugText() {

    g_debugText = "Map Editor Mode: " +  Util::EditorModeToString(Game::g_editorMode);
}
