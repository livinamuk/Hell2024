
namespace Game {
    
    enum class GameMode { GAME, EDITOR_TOP_DOWN, EDITOR_3D };
    enum class MultiplayerMode { NONE, LOCAL, ONLINE };
    enum class SplitscreenMode { NONE, TWO_PLAYER, THREE_PLAYER, FOUR_PLAYER };

    void Create();
    bool IsLoaded();
    void Update();

    const GameMode& GetGameMode();
    const MultiplayerMode& GetMultiplayerMode();
    const SplitscreenMode& GetSplitscreenMode();

}