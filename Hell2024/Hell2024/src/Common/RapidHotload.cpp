#include "RapidHotload.h"
#include "../Game/Game.h"
#include "../Game/Scene.h"
#include "../Input/input.h"

void RapidHotload::Update() {

    GameObject* mermaid = Scene::GetGameObjectByName("Mermaid");
    mermaid->SetMeshBlendingMode("BoobTube", BlendingMode::NONE);
    //mermaid->SetMeshBlendingMode("EyelashUpper", BlendingMode::BLENDED);
    //mermaid->SetMeshBlendingMode("EyelashLower", BlendingMode::BLENDED);
    //mermaid->SetMeshBlendingMode("HairScalp", BlendingMode::BLENDED);
    //mermaid->SetMeshBlendingMode("HairOutta", BlendingMode::ALPHA_DISCARDED);
    //mermaid->SetMeshBlendingMode("HairInner", BlendingMode::ALPHA_DISCARDED);

    if (Input::KeyPressed(HELL_KEY_U)) {
        Texture* texture;
        texture = AssetManager::GetTextureByName("BathroomFloor2_NRM");
        if (texture) {
            texture->GetGLTexture().HotloadFromPath("res/textures/ui/BathroomFloor2_NRM.png");
        }
        //texture = AssetManager::GetTextureByName("PresentSmallRed_ALB");
        //if (texture) {
        //    texture->GetGLTexture().HotloadFromPath("res/textures/ui/PresentSmallRed_ALB.png");
        //}
        //texture = AssetManager::GetTextureByName("MermaidEye_RMA");
        //if (texture) {
        //    texture->GetGLTexture().HotloadFromPath("res/textures/ui/MermaidEye_RMA.png");
        //}
    }
}
