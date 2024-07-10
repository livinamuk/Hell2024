#include "Player.h"
#include "Game.h"
#include "../Editor/Editor.h"
#include "../Renderer/RendererUtil.hpp"
#include "../Renderer/TextBlitter.h"

std::vector<RenderItem2D> Player::GetHudRenderItems(ivec2 presentSize) {

    std::vector<RenderItem2D> renderItems;

    ivec2 debugTextLocation;
    debugTextLocation.x = RendererUtil::GetViewportLeftX(m_playerIndex, Game::GetSplitscreenMode(), presentSize.x, presentSize.y);
    debugTextLocation.y = RendererUtil::GetViewportTopY(m_playerIndex, Game::GetSplitscreenMode(), presentSize.x, presentSize.y);

    ivec2 pickupTextLocation;
    pickupTextLocation.x = RendererUtil::GetViewportLeftX(m_playerIndex, Game::GetSplitscreenMode(), presentSize.x, presentSize.y);
    pickupTextLocation.y = RendererUtil::GetViewportBottomY(m_playerIndex, Game::GetSplitscreenMode(), presentSize.x, presentSize.y);

    ivec2 viewportCenter;
    viewportCenter.x = RendererUtil::GetViewportCenterX(m_playerIndex, Game::GetSplitscreenMode(), presentSize.x, presentSize.y);
    viewportCenter.y = RendererUtil::GetViewportCenterY(m_playerIndex, Game::GetSplitscreenMode(), presentSize.x, presentSize.y);

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
    if (!Game::DebugTextIsEnabled() && IsAlive() && !Editor::IsOpen()) {
        std::string text;
        text += "Health: " + std::to_string(_health) + "\n";
        text += "" + std::to_string(m_killCount) + "\n";
        text += "Pos: " + Util::Vec3ToString(GetViewPos()) + "\n";

        AnimatedGameObject* viewWeapon = GetViewWeaponAnimatedGameObject();
        int frameNumber = viewWeapon->GetAnimationFrameNumber();
        text += "Anim frame number: " + std::to_string(frameNumber) + "\n";

        if (viewWeapon->AnimationIsPastFrameNumber(10)) {
            text += "Passed frame 10!\n";
        }

        RendererUtil::AddRenderItems(renderItems, TextBlitter::CreateText(text, debugTextLocation, presentSize, Alignment::TOP_LEFT, BitmapFontType::STANDARD));
    }

    // Press Start
    if (RespawnAllowed()) {
        renderItems.push_back(RendererUtil::CreateRenderItem2D("PressStart", viewportCenter, presentSize, Alignment::CENTERED));
    }

    //glm::vec3 cubePos(0, 1, 0);
    //Player* player = Game::GetPlayerByIndex(0);
    //glm::mat4 mvp = player->GetProjectionMatrix() * player->GetViewMatrix();
    //auto res = Util::CalculateScreenSpaceCoordinates(cubePos, mvp, PRESENT_WIDTH, PRESENT_HEIGHT);
    //renderItems.push_back(RendererUtil::CreateRenderItem2D("PressStart", {res.x, res.y}, presentSize, Alignment::CENTERED));

    if (IsAlive()) {

        // Crosshair
        switch (GetCrosshairType()) {
        case CrosshairType::REGULAR:
            renderItems.push_back(RendererUtil::CreateRenderItem2D("CrosshairDot", viewportCenter, presentSize, Alignment::CENTERED));
            break;
        case CrosshairType::INTERACT:
            renderItems.push_back(RendererUtil::CreateRenderItem2D("CrosshairSquare", viewportCenter, presentSize, Alignment::CENTERED));
            break;
        default:
            break;
        }

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
    }

    return renderItems;
}

std::vector<RenderItem2D> Player::GetHudRenderItemsHiRes(ivec2 gBufferSize) {

    std::vector<RenderItem2D> renderItems;

    int leftX = RendererUtil::GetViewportLeftX(m_playerIndex, Game::GetSplitscreenMode(), gBufferSize.x, gBufferSize.y);
    int rightX = RendererUtil::GetViewportRightX(m_playerIndex, Game::GetSplitscreenMode(), gBufferSize.x, gBufferSize.y);
    int bottomY = RendererUtil::GetViewportBottomY(m_playerIndex, Game::GetSplitscreenMode(), gBufferSize.x, gBufferSize.y);

    float ammoTextScale = 0.6f;
    ivec2 ammoSlashTextLocation = { 0,0 };
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

    ivec2 ammoClipTextLocation = { ammoSlashTextLocation.x - int(TextBlitter::GetCharacterSize("/", BitmapFontType::AMMO_NUMBERS).x * 0.7f * ammoTextScale), ammoSlashTextLocation.y };
    ivec2 ammoTotalTextLocation = { ammoSlashTextLocation.x + int(TextBlitter::GetCharacterSize("/", BitmapFontType::AMMO_NUMBERS).x * 1.6f * ammoTextScale), ammoSlashTextLocation.y };


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

    if (Editor::IsOpen() || !IsAlive()) {
        return;
    }

    AnimatedGameObject* viewWeaponAnimatedGameObject = GetViewWeaponAnimatedGameObject();
    WeaponInfo* weaponInfo = GetCurrentWeaponInfo();
    WeaponState* weaponState = GetCurrentWeaponState();

    // Red dot sight
    if (weaponInfo && weaponState && weaponInfo->type == WeaponType::PISTOL && weaponState->hasScope) {

        glm::mat4 modelMatrix = viewWeaponAnimatedGameObject->GetModelMatrix() * m_weaponSwayMatrix * viewWeaponAnimatedGameObject->GetAnimatedTransformByBoneName("Weapon");
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
        RenderItem3D& renderItem = m_attachmentRenderItems.emplace_back();
        renderItem.vertexOffset = mesh->baseVertex;
        renderItem.indexOffset = mesh->baseIndex;
        renderItem.modelMatrix = modelMatrix;
        renderItem.inverseModelMatrix = inverse(renderItem.modelMatrix);
        renderItem.meshIndex = meshIndex;
        renderItem.normalTextureIndex = AssetManager::GetMaterialByIndex(materialIndex)->_normal;
        renderItem.baseColorTextureIndex = AssetManager::GetMaterialByIndex(materialIndex)->_basecolor;
        renderItem.rmaTextureIndex = AssetManager::GetMaterialByIndex(materialIndex)->_rma;
        renderItem.castShadow = false;
    }

    // Silencer
    if (weaponInfo && weaponState && weaponInfo->type == WeaponType::PISTOL && weaponState->hasSilencer) {

        glm::mat4 modelMatrix = viewWeaponAnimatedGameObject->GetModelMatrix() * m_weaponSwayMatrix * viewWeaponAnimatedGameObject->GetAnimatedTransformByBoneName("Muzzle");
        static int redDotSightMaterialIndex = AssetManager::GetMaterialIndex("RedDotSight");
        static int goldMaterialIndex = AssetManager::GetMaterialIndex("Gold");
        int materialIndex = 0;
        if (viewWeaponAnimatedGameObject->IsGold()) {
            materialIndex = goldMaterialIndex;
        }
        else {
            materialIndex = redDotSightMaterialIndex;
        }
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
        renderItem.normalTextureIndex = AssetManager::GetMaterialByIndex(materialIndex)->_normal;
        renderItem.baseColorTextureIndex = AssetManager::GetMaterialByIndex(materialIndex)->_basecolor;
        renderItem.rmaTextureIndex = AssetManager::GetMaterialByIndex(materialIndex)->_rma;
        renderItem.castShadow = false;
    }



    if (weaponInfo->name == "GoldenGlock") {
        // Laser
        if (weaponInfo && weaponState && weaponInfo->type == WeaponType::PISTOL && viewWeaponAnimatedGameObject->_skinnedModel->_filename == "Glock") {

            glm::mat4 modelMatrix = viewWeaponAnimatedGameObject->GetModelMatrix() * m_weaponSwayMatrix * viewWeaponAnimatedGameObject->GetAnimatedTransformByBoneName("Laser");
            static int redDotSightMaterialIndex = AssetManager::GetMaterialIndex("GlockLaser");
            static int goldMaterialIndex = AssetManager::GetMaterialIndex("Gold");
            int materialIndex = 0;
            if (viewWeaponAnimatedGameObject->IsGold()) {
                materialIndex = goldMaterialIndex;
            }
            else {
                materialIndex = redDotSightMaterialIndex;
            }
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
            renderItem.normalTextureIndex = AssetManager::GetMaterialByIndex(materialIndex)->_normal;
            renderItem.baseColorTextureIndex = AssetManager::GetMaterialByIndex(materialIndex)->_basecolor;
            renderItem.rmaTextureIndex = AssetManager::GetMaterialByIndex(materialIndex)->_rma;
            renderItem.castShadow = false;
        }

        // Actual Lazer
        if (weaponInfo && weaponState && weaponInfo->type == WeaponType::PISTOL && viewWeaponAnimatedGameObject->_skinnedModel->_filename == "Glock") {

            glm::mat4 modelMatrix = viewWeaponAnimatedGameObject->GetModelMatrix() * m_weaponSwayMatrix * viewWeaponAnimatedGameObject->GetAnimatedTransformByBoneName("Laser");

            glm::vec3 start = Util::GetTranslationFromMatrix(modelMatrix);
            glm::vec3 end = _cameraRayResult.hitPosition;
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
            renderItem.normalTextureIndex = AssetManager::GetMaterialByIndex(materialIndex)->_normal;
            renderItem.baseColorTextureIndex = AssetManager::GetMaterialByIndex(materialIndex)->_basecolor;
            renderItem.rmaTextureIndex = AssetManager::GetMaterialByIndex(materialIndex)->_rma;
            renderItem.emissiveColor = RED;
            renderItem.useEmissiveMask = true;
            renderItem.castShadow = false;
        }
    }
}

void Player::UpdateAttachmentGlassRenderItems() {

    m_attachmentGlassRenderItems.clear();

    if (Editor::IsOpen() || !IsAlive()) {
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
        uint32_t& meshIndex = model->GetMeshIndices()[1];
        Mesh* mesh = AssetManager::GetMeshByIndex(meshIndex);
        RenderItem3D& renderItem = m_attachmentGlassRenderItems.emplace_back();
        renderItem.vertexOffset = mesh->baseVertex;
        renderItem.indexOffset = mesh->baseIndex;
        renderItem.modelMatrix = modelMatrix;
        renderItem.inverseModelMatrix = inverse(renderItem.modelMatrix);
        renderItem.meshIndex = meshIndex;
        renderItem.normalTextureIndex = AssetManager::GetMaterialByIndex(materialIndex)->_normal;
        renderItem.baseColorTextureIndex = AssetManager::GetMaterialByIndex(materialIndex)->_basecolor;
        renderItem.rmaTextureIndex = AssetManager::GetMaterialByIndex(materialIndex)->_rma;
        renderItem.castShadow = false;

    }
}

std::vector<RenderItem3D>& Player::GetAttachmentRenderItems() {
    return m_attachmentRenderItems;
}

std::vector<RenderItem3D>& Player::GetAttachmentGlassRenderItems() {
    return m_attachmentGlassRenderItems;
}