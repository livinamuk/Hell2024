#include "Player.h"

namespace Game {

    enum class GameMode { GAME, EDITOR_TOP_DOWN, EDITOR_3D };
    enum class MultiplayerMode { NONE, LOCAL, ONLINE };

    struct GameSettings {
        bool takeDamageOutside = false;
        glm::vec3 skyBoxTint = glm::vec3(1);
    };

    void Create();
    void CreatePlayers(unsigned int playerCount);
    bool IsLoaded();
    void Update();
    void NextSplitScreenMode();
    void SetSplitscreenMode(SplitscreenMode mode);
    void SetPlayerKeyboardAndMouseIndex(int playerIndex, int keyboardIndex, int mouseIndex);
    void SetPlayerGroundedStates();
    void GiveControlToPlayer1();
    int GetPlayerViewportCount();

    inline int g_killLimit = 10;
    inline float g_globalFadeOut = 1.0f;
    inline float g_globalFadeOutWaitTimer = 0.0f;
    inline bool g_liceneToKill = false;


    bool KillLimitReached();
    const int GetPlayerCount();
    const int GetPlayerIndexFromPlayerPointer(Player* player);
    Player* GetPlayerByIndex(unsigned int index);
    const GameMode& GetGameMode();
    const MultiplayerMode& GetMultiplayerMode();
    const SplitscreenMode& GetSplitscreenMode();
    const bool DebugTextIsEnabled();
    float GetTime();
    // Pickups
    void SpawnPickup(PickUpType pickupType, glm::vec3 position, glm::vec3 rotation, bool wakeOnStart);
    void SpawnAmmo(std::string type, glm::vec3 position, glm::vec3 rotation, bool wakeOnStart);

    const GameSettings& GetSettings();

    inline int g_dogDeaths = -1;
    inline int g_dogKills = -1;
    inline int g_sharkDeaths = -1;
    inline int g_sharkKills = -1;
    inline std::vector<glm::vec3> testPoints;
    inline std::vector<PlayerData> g_playerData;
    inline EditorMode g_editorMode = EditorMode::SHARK_PATH;

}