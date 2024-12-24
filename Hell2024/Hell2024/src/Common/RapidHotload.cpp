#include "RapidHotload.h"
#include "../Game/Game.h"
#include "../Game/Scene.h"
#include "../Input/input.h"

void RapidHotload::Update() {

    GameObject* mermaid = Scene::GetGameObjectByName("Mermaid");
    //mermaid->SetMeshBlendingMode("EyelashUpper", BlendingMode::BLENDED);
    //mermaid->SetMeshBlendingMode("EyelashLower", BlendingMode::BLENDED);
    //mermaid->SetMeshBlendingMode("HairScalp", BlendingMode::BLENDED);
    //mermaid->SetMeshBlendingMode("HairOutta", BlendingMode::ALPHA_DISCARDED);
    //mermaid->SetMeshBlendingMode("HairInner", BlendingMode::ALPHA_DISCARDED);

    if (Input::KeyPressed(HELL_KEY_NUMPAD_8)) {
        mermaid->SetModel("Mermaid3");
        mermaid->SetMeshMaterial("Gold");
        mermaid->SetMeshMaterialByMeshName("Rock", "Rock");
        mermaid->SetMeshMaterialByMeshName("BoobTube", "BoobTube");
        mermaid->SetMeshMaterialByMeshName("Face", "MermaidFace");
        mermaid->SetMeshMaterialByMeshName("Body", "MermaidBody");
        mermaid->SetMeshMaterialByMeshName("Arms", "MermaidArms");
        mermaid->SetMeshMaterialByMeshName("HairInner", "MermaidHair");
        mermaid->SetMeshMaterialByMeshName("HairOutta", "MermaidHair");
        mermaid->SetMeshMaterialByMeshName("HairScalp", "MermaidScalp");
        mermaid->SetMeshMaterialByMeshName("EyeLeft", "MermaidEye");
        mermaid->SetMeshMaterialByMeshName("EyeRight", "MermaidEye");
        mermaid->SetMeshMaterialByMeshName("Tail", "MermaidTail");
        mermaid->SetMeshMaterialByMeshName("TailFin", "MermaidTail");
        mermaid->SetMeshMaterialByMeshName("EyelashUpper", "MermaidLashes");
        mermaid->SetMeshMaterialByMeshName("EyelashLower", "MermaidLashes");
        mermaid->SetMeshMaterialByMeshName("red", "ChristmasHat");
        mermaid->SetMeshMaterialByMeshName("Nails", "Nails");
        mermaid->SetMeshBlendingMode("EyelashUpper", BlendingMode::BLENDED);
        mermaid->SetMeshBlendingMode("EyelashLower", BlendingMode::BLENDED);
        mermaid->SetMeshBlendingMode("HairScalp", BlendingMode::BLENDED);
        mermaid->SetMeshBlendingMode("HairOutta", BlendingMode::ALPHA_DISCARDED);
        mermaid->SetMeshBlendingMode("HairInner", BlendingMode::ALPHA_DISCARDED);
        mermaid->SetName("Mermaid");
        mermaid->SetKinematic(true);
        mermaid->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("Rock_ConvexMesh"));
        mermaid->SetModelMatrixMode(ModelMatrixMode::GAME_TRANSFORM);
        mermaid->SetCollisionType(CollisionType::STATIC_ENVIROMENT);
    }
    if (Input::KeyPressed(HELL_KEY_NUMPAD_9)) {
        mermaid->SetModel("MermaidChristmas2");
        mermaid->SetMeshMaterial("Gold");
        mermaid->SetMeshMaterialByMeshName("Rock", "Rock");
        mermaid->SetMeshMaterialByMeshName("BoobTube", "BoobTube");
        mermaid->SetMeshMaterialByMeshName("Face", "MermaidFace");
        mermaid->SetMeshMaterialByMeshName("Body", "MermaidBody");
        mermaid->SetMeshMaterialByMeshName("Arms", "MermaidArms");
        mermaid->SetMeshMaterialByMeshName("HairInner", "MermaidHair");
        mermaid->SetMeshMaterialByMeshName("HairOutta", "MermaidHair");
        mermaid->SetMeshMaterialByMeshName("HairScalp", "MermaidScalp");
        mermaid->SetMeshMaterialByMeshName("EyeLeft", "MermaidEye");
        mermaid->SetMeshMaterialByMeshName("EyeRight", "MermaidEye");
        mermaid->SetMeshMaterialByMeshName("Tail", "MermaidTail");
        mermaid->SetMeshMaterialByMeshName("TailFin", "MermaidTail");
        mermaid->SetMeshMaterialByMeshName("EyelashUpper", "MermaidLashes");
        mermaid->SetMeshMaterialByMeshName("EyelashLower", "MermaidLashes");
        mermaid->SetMeshMaterialByMeshName("ChristmasTopRed", "ChristmasTopRed");
        mermaid->SetMeshMaterialByMeshName("ChristmasTopWhite", "ChristmasTopWhite");
        mermaid->SetMeshMaterialByMeshName("ChristmasHat", "ChristmasHat");
        mermaid->SetMeshMaterialByMeshName("red", "ChristmasHat");
        mermaid->SetMeshMaterialByMeshName("Nails", "Nails");
        mermaid->SetMeshBlendingMode("EyelashUpper", BlendingMode::BLENDED);
        mermaid->SetMeshBlendingMode("EyelashLower", BlendingMode::BLENDED);
        mermaid->SetMeshBlendingMode("HairScalp", BlendingMode::BLENDED);
        mermaid->SetMeshBlendingMode("HairOutta", BlendingMode::ALPHA_DISCARDED);
        mermaid->SetMeshBlendingMode("HairInner", BlendingMode::ALPHA_DISCARDED);
        mermaid->SetMeshBlendingMode("BoobTube", BlendingMode::DO_NOT_RENDER);
        mermaid->SetName("Mermaid");
        mermaid->SetKinematic(true);
        mermaid->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("Rock_ConvexMesh"));
        mermaid->SetModelMatrixMode(ModelMatrixMode::GAME_TRANSFORM);
        mermaid->SetCollisionType(CollisionType::STATIC_ENVIROMENT);
    }

    if (Input::KeyPressed(HELL_KEY_U)) {
        Texture* texture;
        texture = AssetManager::GetTextureByName("BathroomFloor_NRM");
        if (texture) {
            texture->GetGLTexture().HotloadFromPath("res/textures/ui/BathroomFloor_NRM.png");
        }
        texture = AssetManager::GetTextureByName("PresentSmallRed_ALB");
        if (texture) {
            texture->GetGLTexture().HotloadFromPath("res/textures/ui/PresentSmallRed_ALB.png");
        }
        texture = AssetManager::GetTextureByName("MermaidEye_RMA");
        if (texture) {
            texture->GetGLTexture().HotloadFromPath("res/textures/ui/MermaidEye_RMA.png");
        }
    }
}
