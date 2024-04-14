
namespace Game {
    
    enum class GameMode { GAME, EDITOR_TOP_DOWN, EDITOR_3D };
    enum class MultiplayerMode { NONE, LOCAL, ONLINE };
    enum class SplitscreenMode { NONE = 0, TWO_PLAYER, FOUR_PLAYER, SPLITSCREEN_MODE_COUNT };

    void Create();
    bool IsLoaded();
    void Update();
    void NextSplitScreenMode();
    void SetSplitscreenMode(SplitscreenMode mode);

    const GameMode& GetGameMode();
    const MultiplayerMode& GetMultiplayerMode();
    const SplitscreenMode& GetSplitscreenMode();

}