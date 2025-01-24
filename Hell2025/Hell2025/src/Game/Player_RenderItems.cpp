#include "Player.h"
#include "Game.h"
#include "../Editor/Editor.h"
#include "../Input/Input.h"
#include "../Game/Scene.h"
#include "../Renderer/Renderer.h"
#include "../Renderer/RendererUtil.hpp"
#include "../Renderer/TextBlitter.h"

std::vector<RenderItem2D> Player::GetHudRenderItems(hell::ivec2 presentSize) {

    std::vector<RenderItem2D> renderItems;

    float framebufferToWindowRatioX = PRESENT_WIDTH / (float)BackEnd::GetCurrentWindowWidth();
    float framebufferToWindowRatioY = PRESENT_HEIGHT / (float)BackEnd::GetCurrentWindowHeight();
    float cursorX = Input::GetMouseX() * framebufferToWindowRatioX;
    float cursorY = (BackEnd::GetCurrentWindowHeight() - Input::GetMouseY()) * framebufferToWindowRatioY;
    hell::ivec2 cursorLocation = hell::ivec2(cursorX, cursorY);
    hell::ivec2 viewportCenter = hell::ivec2(RendererUtil::GetViewportCenterX(m_playerIndex, Game::GetSplitscreenMode(), presentSize.x, presentSize.y), RendererUtil::GetViewportCenterY(m_playerIndex, Game::GetSplitscreenMode(), presentSize.x, presentSize.y));

    if (IsAtShop()) {
        // Cursor
        renderItems.push_back(RendererUtil::CreateRenderItem2D("Cursor_0", cursorLocation, presentSize, Alignment::TOP_LEFT, WHITE, hell::ivec2(40, 40)));
        return renderItems;
    }

    hell::ivec2 debugTextLocation;
    debugTextLocation.x = RendererUtil::GetViewportLeftX(m_playerIndex, Game::GetSplitscreenMode(), presentSize.x, presentSize.y);
    debugTextLocation.y = RendererUtil::GetViewportTopY(m_playerIndex, Game::GetSplitscreenMode(), presentSize.x, presentSize.y);

    hell::ivec2 pickupTextLocation;
    pickupTextLocation.x = RendererUtil::GetViewportLeftX(m_playerIndex, Game::GetSplitscreenMode(), presentSize.x, presentSize.y);
    pickupTextLocation.y = RendererUtil::GetViewportBottomY(m_playerIndex, Game::GetSplitscreenMode(), presentSize.x, presentSize.y);


    if (Game::GetSplitscreenMode() == SplitscreenMode::NONE) {
        pickupTextLocation.x += presentSize.x * 0.09f;
        pickupTextLocation.y += presentSize.y * 0.09f;
    }
    else if (Game::GetSplitscreenMode() == SplitscreenMode::TWO_PLAYER) {
        pickupTextLocation.x += presentSize.x * 0.05f;
        pickupTextLocation.y += presentSize.y * 0.037f;

    }
    else if (Game::GetSplitscreenMode() == SplitscreenMode::FOUR_PLAYER) {
        pickupTextLocation.x += presentSize.x * 0.0375f;
        pickupTextLocation.y += presentSize.y * 0.035f;
    }

    // Text
    //if (!Game::DebugTextIsEnabled() && IsAlive() && !Editor::IsOpen() && !Game::KillLimitReached()) {
    if (!Game::DebugTextIsEnabled() && !Editor::IsOpen() && !Game::KillLimitReached()) {
            std::string text;
        text += "Health: " + std::to_string(_health) + "\n";
        text += "Kills: " + std::to_string(m_killCount) + "\n";
        //text += "Pos: " + Util::Vec3ToString(GetViewPos()) + "\n";
        //text += "Rot: " + Util::Vec3ToString(GetViewRotation()) + "\n";
        //text += "Crosshair size: " + std::to_string(m_crosshairCrossSize) + "\n";
        //text += "Accuracy Modifier: " + std::to_string(m_accuracyModifer) + "\n";
        
        RendererUtil::AddRenderItems(renderItems, TextBlitter::CreateText(text, debugTextLocation, presentSize, Alignment::TOP_LEFT, BitmapFontType::STANDARD));

    }

    if (Game::g_globalFadeOut < 0.05f) {

        std::string winMessage = "";

        if (m_killCount == Game::g_killLimit) {
            winMessage += "Rank: FIRST\n";
        }
        else {
            winMessage += "Rank: SECOND\n";
        }
        winMessage += "Kills: " + std::to_string(m_killCount) + "\n";
        hell::ivec2 size = TextBlitter::GetTextSizeInPixels(winMessage, presentSize, BitmapFontType::STANDARD);


        hell::ivec2 location;
        location.x = RendererUtil::GetViewportCenterX(m_playerIndex, Game::GetSplitscreenMode(), presentSize.x, presentSize.y);
        location.y = RendererUtil::GetViewportCenterY(m_playerIndex, Game::GetSplitscreenMode(), presentSize.x, presentSize.y);

        location.x -= (size.x * 0.5f);
        location.y -= (size.y * 0.5f);

        RendererUtil::AddRenderItems(renderItems, TextBlitter::CreateText(winMessage, location, presentSize, Alignment::TOP_LEFT, BitmapFontType::STANDARD));

    }



  


    // Press Start
    if (RespawnAllowed() && !Game::KillLimitReached()) {
        renderItems.push_back(RendererUtil::CreateRenderItem2D("PressStart", viewportCenter, presentSize, Alignment::CENTERED));
    }

    //glm::vec3 cubePos(0, 1, 0);
    //Player* player = Game::GetPlayerByIndex(0);
    //glm::mat4 mvp = player->GetProjectionMatrix() * player->GetViewMatrix();
    //auto res = Util::CalculateScreenSpaceCoordinates(cubePos, mvp, PRESENT_WIDTH, PRESENT_HEIGHT);
    //renderItems.push_back(RendererUtil::CreateRenderItem2D("PressStart", {res.x, res.y}, presentSize, Alignment::CENTERED));


    /*
    for (Light& light : Scene::g_lights) {
        glm::vec3 position = light.position;
       Player* player = Game::GetPlayerByIndex(0);
       glm::mat4 mvp = player->GetProjectionMatrix() * player->GetViewMatrix();
       auto res = Util::CalculateScreenSpaceCoordinates(position, mvp, PRESENT_WIDTH, PRESENT_HEIGHT, true);
       renderItems.push_back(RendererUtil::CreateRenderItem2D("Icon_Light", {res.x, res.y}, presentSize, Alignment::CENTERED));
    }*/

    if (IsAlive()) {

        hell::ivec2 crosshairPos = viewportCenter;
       // if (GetCrosshairType() == CrosshairType::REGULAR) {
       //     renderItems.push_back(RendererUtil::CreateRenderItem2D("CrosshairDot", crosshairPos, presentSize, Alignment::CENTERED));
       // }
       // else if (GetCrosshairType() == CrosshairType::INTERACT) {
       //     renderItems.push_back(RendererUtil::CreateRenderItem2D("CrosshairSquare", crosshairPos, presentSize, Alignment::CENTERED));
       // }




      //  if (m_crosshairCrossSize > 5) {
      //      renderItems.push_back(RendererUtil::CreateRenderItem2D("CrosshairCrossLeft", crosshairPos + hell::ivec2{-int(m_crosshairCrossSize), 0}, presentSize, Alignment::CENTERED));
      //      renderItems.push_back(RendererUtil::CreateRenderItem2D("CrosshairCrossRight", crosshairPos + hell::ivec2{int(m_crosshairCrossSize), 0}, presentSize, Alignment::CENTERED));
      //      renderItems.push_back(RendererUtil::CreateRenderItem2D("CrosshairCrossTop", crosshairPos + hell::ivec2{0, int(m_crosshairCrossSize)}, presentSize, Alignment::CENTERED));
      //      renderItems.push_back(RendererUtil::CreateRenderItem2D("CrosshairCrossBottom", crosshairPos + hell::ivec2{0, -int(m_crosshairCrossSize)}, presentSize, Alignment::CENTERED));
      //  }
      //  else{
      //       renderItems.push_back(RendererUtil::CreateRenderItem2D("CrosshairDot", crosshairPos, presentSize, Alignment::CENTERED));
      //  }

        if (!Game::KillLimitReached()) {
            if (GetCrosshairType() == CrosshairType::REGULAR) {
                renderItems.push_back(RendererUtil::CreateRenderItem2D("CrosshairDot", crosshairPos, presentSize, Alignment::CENTERED));
            }
            if (GetCrosshairType() == CrosshairType::INTERACT) {
                renderItems.push_back(RendererUtil::CreateRenderItem2D("CrosshairSquare", crosshairPos, presentSize, Alignment::CENTERED));
            }
        }

        static int texHeight = AssetManager::GetTextureByName("inventory_mockup")->GetHeight();
        static int height = (presentSize.y - texHeight) / 2;

        //renderItems.push_back(RendererUtil::CreateRenderItem2D("inventory_mockup", {40, height}, presentSize, Alignment::BOTTOM_LEFT));

        // Pickup text
        pickupTextLocation.y += m_pickUpTexts.size() * TextBlitter::GetLineHeight(BitmapFontType::STANDARD);

        std::string pickUpTextToBlit = "";
        //for (int i = m_pickUpTexts.size() - 1; i >= 0; i--) {
        for (int i = 0; i < m_pickUpTexts.size(); i++) {
            pickUpTextToBlit += m_pickUpTexts[i].text;
            if (m_pickUpTexts[i].count > 1) {
                pickUpTextToBlit += " +" + std::to_string(m_pickUpTexts[i].count);
            }
            pickUpTextToBlit += "\n";
        }

        RendererUtil::AddRenderItems(renderItems, TextBlitter::CreateText(pickUpTextToBlit, pickupTextLocation, presentSize, Alignment::BOTTOM_LEFT, BitmapFontType::STANDARD));


        /*
        std::string question = "Will you take the [g]GLOCK[w]?\n";
        question += ">YES  NO";
        float scale = 1.5f;
        RendererUtil::AddRenderItems(renderItems, TextBlitter::CreateText(question, ivec2(80, 80), presentSize, Alignment::BOTTOM_LEFT, BitmapFontType::STANDARD, glm::vec2(scale)));
        */
    }

    return renderItems;
}

std::vector<RenderItem2D> Player::GetHudRenderItemsHiRes(hell::ivec2 gBufferSize) {

    std::vector<RenderItem2D> renderItems;

    if (IsAtShop() || Editor::IsOpen()) {

        // Cursor
        float framebufferToWindowRatioX = PRESENT_WIDTH * 2 / (float)BackEnd::GetCurrentWindowWidth();
        float framebufferToWindowRatioY = PRESENT_HEIGHT * 2 / (float)BackEnd::GetCurrentWindowHeight();
        float cursorX = Input::GetMouseX() * framebufferToWindowRatioX;
        float cursorY = (BackEnd::GetCurrentWindowHeight() - Input::GetMouseY()) * framebufferToWindowRatioY;
        hell::ivec2 cursorLocation = hell::ivec2(cursorX, cursorY);
        cursorLocation = hell::ivec2(100, 100);
        //renderItems.push_back(RendererUtil::CreateRenderItem2D("Cursor_0", cursorLocation, gBufferSize, Alignment::TOP_LEFT, WHITE, hell::ivec2(12, 12)));

        return renderItems;
    }

    int leftX = RendererUtil::GetViewportLeftX(m_playerIndex, Game::GetSplitscreenMode(), gBufferSize.x, gBufferSize.y);
    int rightX = RendererUtil::GetViewportRightX(m_playerIndex, Game::GetSplitscreenMode(), gBufferSize.x, gBufferSize.y);
    int bottomY = RendererUtil::GetViewportBottomY(m_playerIndex, Game::GetSplitscreenMode(), gBufferSize.x, gBufferSize.y);

    float ammoTextScale = 0.7f;
    hell::ivec2 ammoSlashTextLocation = { 0,0 };
    if (Game::GetSplitscreenMode() == SplitscreenMode::NONE) {
        ammoSlashTextLocation.x = rightX - (gBufferSize.x * 0.125f);
        ammoSlashTextLocation.y = bottomY + (gBufferSize.x * 0.1f);
    }
    if (Game::GetSplitscreenMode() == SplitscreenMode::TWO_PLAYER) {
        ammoSlashTextLocation.x = rightX - (gBufferSize.x * 0.125f);
        ammoSlashTextLocation.y = bottomY + (gBufferSize.x * 0.065f);
    }
    if (Game::GetSplitscreenMode() == SplitscreenMode::FOUR_PLAYER) {
        ammoSlashTextLocation.x = rightX - (gBufferSize.x * 0.08f);
        ammoSlashTextLocation.y = bottomY + (gBufferSize.x * 0.065f);
    }

    hell::ivec2 ammoClipTextLocation = { ammoSlashTextLocation.x - int(TextBlitter::GetCharacterSize("/", BitmapFontType::AMMO_NUMBERS).x * 0.7f * ammoTextScale), ammoSlashTextLocation.y };
    hell::ivec2 ammoTotalTextLocation = { ammoSlashTextLocation.x + int(TextBlitter::GetCharacterSize("/", BitmapFontType::AMMO_NUMBERS).x * 1.6f * ammoTextScale), ammoSlashTextLocation.y };


    if (IsAlive()) {

        WeaponInfo* weaponInfo = GetCurrentWeaponInfo();

        // Ammo
        if (weaponInfo->type != WeaponType::MELEE) {
            std::string clipText = std::to_string(GetCurrentWeaponMagAmmo());
            std::string totalText = std::to_string(GetCurrentWeaponTotalAmmo());
            if (GetCurrentWeaponMagAmmo() == 0) {
                clipText = "[lr]" + clipText;
            }
            else {
                clipText = "[lg]" + clipText;
            }
            RendererUtil::AddRenderItems(renderItems, TextBlitter::CreateText(totalText, ammoTotalTextLocation, gBufferSize, Alignment::TOP_LEFT, BitmapFontType::AMMO_NUMBERS, glm::vec3(ammoTextScale * 0.8f)));
            RendererUtil::AddRenderItems(renderItems, TextBlitter::CreateText("/", ammoSlashTextLocation, gBufferSize, Alignment::TOP_LEFT, BitmapFontType::AMMO_NUMBERS, glm::vec3(ammoTextScale)));
            RendererUtil::AddRenderItems(renderItems, TextBlitter::CreateText(clipText, ammoClipTextLocation, gBufferSize, Alignment::TOP_RIGHT, BitmapFontType::AMMO_NUMBERS, glm::vec3(ammoTextScale)));
        }
    }

    if (Game::KillLimitReached()) {
        renderItems.clear();
    }

    return renderItems;
}

void Player::DisableControl() {
    m_ignoreControl = true;
}

void Player::EnableControl() {
    m_ignoreControl = false;
}

RenderItem3D Player::CreateAttachmentRenderItem(WeaponAttachmentInfo* weaponAttachmentInfo, const char* boneName) {


    // this is full of mistakes
    // u also dont need a function for this, its messy coz u return an empty object

    /*
    AnimatedGameObject* viewWeapon = GetViewWeaponAnimatedGameObject();

    if (viewWeapon && weaponAttachmentInfo) {

        glm::mat4 modelMatrix = viewWeaponAnimatedGameObject->GetModelMatrix() * m_weaponSwayMatrix * viewWeaponAnimatedGameObject->GetAnimatedTransformByBoneName(boneName);
        static int redDotSightMaterialIndex = AssetManager::GetMaterialIndex("RedDotSight");
        static int goldMaterialIndex = AssetManager::GetMaterialIndex("Gold");
        int materialIndex = 0;
        if (viewWeaponAnimatedGameObject->IsGold()) {
            materialIndex = goldMaterialIndex;
        }
        else {
            materialIndex = redDotSightMaterialIndex;
        }
        uint32_t modelIndex = AssetManager::GetModelIndexByName("Glock_RedDotSight");
        Model* model = AssetManager::GetModelByIndex(modelIndex);
        uint32_t& meshIndex = model->GetMeshIndices()[0];
        Mesh* mesh = AssetManager::GetMeshByIndex(meshIndex);
        RenderItem3D renderItem;// = m_attachmentRenderItems.emplace_back();
        renderItem.vertexOffset = mesh->baseVertex;
        renderItem.indexOffset = mesh->baseIndex;
        renderItem.modelMatrix = modelMatrix;
        renderItem.inverseModelMatrix = inverse(renderItem.modelMatrix);
        renderItem.meshIndex = meshIndex;
        renderItem.normalTextureIndex = AssetManager::GetMaterialByIndex(materialIndex)->_normal;
        renderItem.baseColorTextureIndex = AssetManager::GetMaterialByIndex(materialIndex)->_basecolor;
        renderItem.rmaTextureIndex = AssetManager::GetMaterialByIndex(materialIndex)->_rma;
        renderItem.castShadow = false;

        return renderItem;
    }
    else {
        return RenderItem3D();
    }*/
    return RenderItem3D();

}

void Player::UpdateAttachmentRenderItems() {

    m_attachmentRenderItems.clear();

    if (Editor::IsOpen() || !IsAlive() || IsAtShop()) {
        return;
    }

    AnimatedGameObject* viewWeaponAnimatedGameObject = GetViewWeaponAnimatedGameObject();
    WeaponInfo* weaponInfo = GetCurrentWeaponInfo();
    WeaponState* weaponState = GetCurrentWeaponState();


    // Red dot sight
    if (weaponInfo && weaponState && weaponInfo->type == WeaponType::PISTOL && weaponState->hasScope) {
        glm::mat4 modelMatrix = viewWeaponAnimatedGameObject->GetModelMatrix() * m_weaponSwayMatrix * viewWeaponAnimatedGameObject->GetAnimatedTransformByBoneName("Weapon");
        static int materialIndex = AssetManager::GetMaterialIndex("RedDotSight");
        uint32_t modelIndex = AssetManager::GetModelIndexByName("Glock_RedDotSight");
        Model* model = AssetManager::GetModelByIndex(modelIndex);
        uint32_t& meshIndex = model->GetMeshIndices()[0];
        Mesh* mesh = AssetManager::GetMeshByIndex(meshIndex);
        RenderItem3D& renderItem = m_attachmentRenderItems.emplace_back();
        renderItem.vertexOffset = mesh->baseVertex;
        renderItem.indexOffset = mesh->baseIndex;
        renderItem.modelMatrix = modelMatrix;
        renderItem.inverseModelMatrix = inverse(renderItem.modelMatrix);
        renderItem.meshIndex = meshIndex;
        renderItem.castShadow = false;
        Material* material = AssetManager::GetMaterialByIndex(materialIndex);
        if (viewWeaponAnimatedGameObject->IsGold()) {
            renderItem.baseColorTextureIndex = AssetManager::GetGoldBaseColorTextureIndex();
            renderItem.rmaTextureIndex = AssetManager::GetGoldRMATextureIndex();
        }
        else {
            renderItem.baseColorTextureIndex = material->_basecolor;
            renderItem.rmaTextureIndex = material->_rma;
        }
        renderItem.normalMapTextureIndex = material->_normal;
    }


    // AKS74U Scope
    if (weaponInfo && weaponState && weaponInfo->name == "AKS74U" && false) {
        glm::mat4 modelMatrix = viewWeaponAnimatedGameObject->GetModelMatrix() * m_weaponSwayMatrix * viewWeaponAnimatedGameObject->GetAnimatedTransformByBoneName("Weapon");
        static int materialIndex = AssetManager::GetMaterialIndex("Gold");
        uint32_t modelIndex = AssetManager::GetModelIndexByName("AKS74U_Scope");
        Model* model = AssetManager::GetModelByIndex(modelIndex);
        for (uint32_t& meshIndex : model->GetMeshIndices()) {
            Mesh* mesh = AssetManager::GetMeshByIndex(meshIndex);
            RenderItem3D& renderItem = m_attachmentRenderItems.emplace_back();
            renderItem.vertexOffset = mesh->baseVertex;
            renderItem.indexOffset = mesh->baseIndex;
            renderItem.modelMatrix = modelMatrix;
            renderItem.inverseModelMatrix = inverse(renderItem.modelMatrix);
            renderItem.meshIndex = meshIndex;
            renderItem.castShadow = false;
            Material* material = AssetManager::GetMaterialByIndex(materialIndex);
            if (viewWeaponAnimatedGameObject->IsGold()) {
                renderItem.baseColorTextureIndex = AssetManager::GetGoldBaseColorTextureIndex();
                renderItem.rmaTextureIndex = AssetManager::GetGoldRMATextureIndex();
            }
            else {
                renderItem.baseColorTextureIndex = material->_basecolor;
                renderItem.rmaTextureIndex = material->_rma;
            }
            renderItem.normalMapTextureIndex = material->_normal;
        }
    }

    // Silencer
    if (weaponInfo && weaponState && weaponInfo->type == WeaponType::PISTOL && weaponState->hasSilencer) {
        glm::mat4 modelMatrix = viewWeaponAnimatedGameObject->GetModelMatrix() * m_weaponSwayMatrix * viewWeaponAnimatedGameObject->GetAnimatedTransformByBoneName("Muzzle");
        static int materialIndex = AssetManager::GetMaterialIndex("Silencer");
        uint32_t modelIndex = AssetManager::GetModelIndexByName("Glock_Silencer");
        Model* model = AssetManager::GetModelByIndex(modelIndex);
        uint32_t& meshIndex = model->GetMeshIndices()[0];
        Mesh* mesh = AssetManager::GetMeshByIndex(meshIndex);
        RenderItem3D& renderItem = m_attachmentRenderItems.emplace_back();
        renderItem.vertexOffset = mesh->baseVertex;
        renderItem.indexOffset = mesh->baseIndex;
        renderItem.modelMatrix = modelMatrix;
        renderItem.inverseModelMatrix = inverse(renderItem.modelMatrix);
        renderItem.meshIndex = meshIndex;
        renderItem.castShadow = false;
        Material* material = AssetManager::GetMaterialByIndex(materialIndex);
        if (viewWeaponAnimatedGameObject->IsGold()) {
            renderItem.baseColorTextureIndex = AssetManager::GetGoldBaseColorTextureIndex();
            renderItem.rmaTextureIndex = AssetManager::GetGoldRMATextureIndex();
        }
        else {
            renderItem.baseColorTextureIndex = material->_basecolor;
            renderItem.rmaTextureIndex = material->_rma;
        }
        renderItem.normalMapTextureIndex = material->_normal;
    }

    if (weaponInfo->name == "GoldenGlock" && false) {
        // Laser
        if (weaponInfo && weaponState && weaponInfo->type == WeaponType::PISTOL && viewWeaponAnimatedGameObject->_skinnedModel->_filename == "Glock") {
            glm::mat4 modelMatrix = viewWeaponAnimatedGameObject->GetModelMatrix() * m_weaponSwayMatrix * viewWeaponAnimatedGameObject->GetAnimatedTransformByBoneName("Laser");
            static int materialIndex = AssetManager::GetMaterialIndex("GlockLaser");
            uint32_t modelIndex = AssetManager::GetModelIndexByName("Glock_Laser");
            Model* model = AssetManager::GetModelByIndex(modelIndex);
            uint32_t& meshIndex = model->GetMeshIndices()[0];
            Mesh* mesh = AssetManager::GetMeshByIndex(meshIndex);
            RenderItem3D& renderItem = m_attachmentRenderItems.emplace_back();
            renderItem.vertexOffset = mesh->baseVertex;
            renderItem.indexOffset = mesh->baseIndex;
            renderItem.modelMatrix = modelMatrix;
            renderItem.inverseModelMatrix = inverse(renderItem.modelMatrix);
            renderItem.meshIndex = meshIndex;
            renderItem.castShadow = false;
            Material* material = AssetManager::GetMaterialByIndex(materialIndex);
            if (viewWeaponAnimatedGameObject->IsGold()) {
                renderItem.baseColorTextureIndex = AssetManager::GetGoldBaseColorTextureIndex();
                renderItem.rmaTextureIndex = AssetManager::GetGoldRMATextureIndex();
            }
            else {
                renderItem.baseColorTextureIndex = material->_basecolor;
                renderItem.rmaTextureIndex = material->_rma;
            }
            renderItem.normalMapTextureIndex = material->_normal;
        }

        // Actual Lazer
        if (weaponInfo && weaponState && weaponInfo->type == WeaponType::PISTOL && viewWeaponAnimatedGameObject->_skinnedModel->_filename == "Glock") {

            glm::mat4 modelMatrix = viewWeaponAnimatedGameObject->GetModelMatrix() * m_weaponSwayMatrix * viewWeaponAnimatedGameObject->GetAnimatedTransformByBoneName("Laser");

            glm::vec3 start = Util::GetTranslationFromMatrix(modelMatrix);
            glm::vec3 end = m_cameraRayResult.hitPosition;
            float laserLength = glm::distance(start, end);

            Transform laserScaleTransform;
            laserScaleTransform.scale.z = laserLength;

            modelMatrix = modelMatrix * laserScaleTransform.to_mat4();
            static int materialIndex = AssetManager::GetMaterialIndex("PresentA");
            uint32_t modelIndex = AssetManager::GetModelIndexByName("Glock_ActualLaser");
            Model* model = AssetManager::GetModelByIndex(modelIndex);
            uint32_t& meshIndex = model->GetMeshIndices()[0];
            Mesh* mesh = AssetManager::GetMeshByIndex(meshIndex);
            RenderItem3D& renderItem = m_attachmentRenderItems.emplace_back();
            renderItem.vertexOffset = mesh->baseVertex;
            renderItem.indexOffset = mesh->baseIndex;
            renderItem.modelMatrix = modelMatrix;
            renderItem.inverseModelMatrix = inverse(renderItem.modelMatrix);
            renderItem.meshIndex = meshIndex;
            renderItem.emissiveColor = RED;
            renderItem.useEmissiveMask = true;
            renderItem.castShadow = false;
            Material* material = AssetManager::GetMaterialByIndex(materialIndex);
            if (viewWeaponAnimatedGameObject->IsGold()) {
                renderItem.baseColorTextureIndex = AssetManager::GetGoldBaseColorTextureIndex();
                renderItem.rmaTextureIndex = AssetManager::GetGoldRMATextureIndex();
            }
            else {
                renderItem.baseColorTextureIndex = material->_basecolor;
                renderItem.rmaTextureIndex = material->_rma;
            }
            renderItem.normalMapTextureIndex = material->_normal;
        }
    }
}

void Player::UpdateAttachmentGlassRenderItems() {

    m_attachmentGlassRenderItems.clear();

    if (Editor::IsOpen() || !IsAlive() || IsAtShop()) {
        return;
    }

    AnimatedGameObject* viewWeaponAnimatedGameObject = GetViewWeaponAnimatedGameObject();
    WeaponInfo* weaponInfo = GetCurrentWeaponInfo();
    WeaponState* weaponState = GetCurrentWeaponState();

    // Red dot sight
    if (weaponInfo && weaponState && weaponInfo->type == WeaponType::PISTOL && weaponState->hasScope) {
        glm::mat4 modelMatrix = viewWeaponAnimatedGameObject->GetModelMatrix() * m_weaponSwayMatrix * viewWeaponAnimatedGameObject->GetAnimatedTransformByBoneName("Weapon");
        static int materialIndex = AssetManager::GetMaterialIndex("RedDotSight");
        uint32_t modelIndex = AssetManager::GetModelIndexByName("Glock_RedDotSight");
        Model* model = AssetManager::GetModelByIndex(modelIndex);
        if (model) {
            uint32_t& meshIndex = model->GetMeshIndices()[1];
            Mesh* mesh = AssetManager::GetMeshByIndex(meshIndex);
            RenderItem3D& renderItem = m_attachmentGlassRenderItems.emplace_back();
            renderItem.vertexOffset = mesh->baseVertex;
            renderItem.indexOffset = mesh->baseIndex;
            renderItem.modelMatrix = modelMatrix;
            renderItem.inverseModelMatrix = inverse(renderItem.modelMatrix);
            renderItem.meshIndex = meshIndex;
            renderItem.castShadow = false;
            Material* material = AssetManager::GetMaterialByIndex(materialIndex);
            renderItem.baseColorTextureIndex = material->_basecolor;
            renderItem.rmaTextureIndex = material->_rma;
            renderItem.normalMapTextureIndex = material->_normal;
        }
    }
}

std::vector<RenderItem3D>& Player::GetAttachmentRenderItems() {
    return m_attachmentRenderItems;
}

std::vector<RenderItem3D>& Player::GetAttachmentGlassRenderItems() {
    return m_attachmentGlassRenderItems;
}