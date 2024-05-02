#include "Player.h"

namespace Game {
    
    enum class GameMode { GAME, EDITOR_TOP_DOWN, EDITOR_3D };
    enum class MultiplayerMode { NONE, LOCAL, ONLINE };

    void Create();
    void CreatePlayers(unsigned int playerCount);
    bool IsLoaded();
    void Update();
    void NextSplitScreenMode();
    void SetSplitscreenMode(SplitscreenMode mode);
    void SetPlayerKeyboardAndMouseIndex(int playerIndex, int keyboardIndex, int mouseIndex); 
    void SetPlayerGroundedStates();

    const int GetPlayerCount(); 
    const int GetPlayerIndexFromPlayerPointer(Player* player);
    Player* GetPlayerByIndex(unsigned int index);
    const GameMode& GetGameMode();
    const MultiplayerMode& GetMultiplayerMode();
    const SplitscreenMode& GetSplitscreenMode();
    const bool DebugTextIsEnabled();

}