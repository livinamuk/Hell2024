#include "Renderer.h"
#include <vector>
#include <map>
#include "../API/OpenGL/GL_renderer.h"
#include "../API/Vulkan/VK_renderer.h"
#include "../BackEnd/BackEnd.h"
#include "../Core/Game.h"
#include "../Core/Input.h"
#include "../Core/Player.h"
#include "../Core/Scene.h"
#include "../Renderer/GlobalIllumination.h"
#include "../Renderer/RenderData.h"
#include "../Renderer/TextBlitter.h"
#include "../Renderer/RendererUtil.hpp"
#include "../Effects/MuzzleFlash.h"


//void CreateGeometryDrawCommands(RenderData& renderData);

IndirectDrawInfo CreateIndirectDrawInfo(std::vector<RenderItem3D>& potentialRenderItems, int playerCount);



//CameraData CreateCameraData(unsigned int playerIndex);
std::vector<RenderItem2D> CreateRenderItems2D(ivec2 presentSize, int playerCount);
std::vector<RenderItem2D> CreateRenderItems2DHiRes(ivec2 gBufferSize, int playerCount);
std::vector<RenderItem3D> CreateRenderItems3D();
std::vector<RenderItem3D> CreateGlassRenderItems();
std::vector<GPULight> CreateGPULights();
BlitDstCoords GetBlitDstCoords(unsigned int playerIndex);
void UpdateDebugLinesMesh();
void UpdateDebugPointsMesh();
void RenderGame(RenderData& renderData);
void RenderEditor3D(RenderData& renderData);
void RenderEditorTopDown(RenderData& renderData);
//void ResizeRenderTargets();
void PresentFinalImage();
MuzzleFlashData GetMuzzleFlashData(unsigned int playerIndex);
std::vector<SkinnedRenderItem> GetSkinnedRenderItemsForPlayer(int playerIndex);

DetachedMesh _debugLinesMesh;
DetachedMesh _debugPointsMesh;
DebugLineRenderMode _debugLineRenderMode = DebugLineRenderMode::SHOW_NO_LINES;
RenderMode _renderMode = RenderMode::COMPOSITE;
std::vector<glm::mat4> _animatedTransforms;
std::vector<glm::mat4> _instanceMatrices;
//std::vector<glm::mat4> _decalMatrices;

/*

██████╗ ███████╗███╗   ██╗██████╗ ███████╗██████╗     ██████╗  █████╗ ████████╗ █████╗
██╔══██╗██╔════╝████╗  ██║██╔══██╗██╔════╝██╔══██╗    ██╔══██╗██╔══██╗╚══██╔══╝██╔══██╗
██████╔╝█████╗  ██╔██╗ ██║██║  ██║█████╗  ██████╔╝    ██║  ██║███████║   ██║   ███████║
██╔══██╗██╔══╝  ██║╚██╗██║██║  ██║██╔══╝  ██╔══██╗    ██║  ██║██╔══██║   ██║   ██╔══██║
██║  ██║███████╗██║ ╚████║██████╔╝███████╗██║  ██║    ██████╔╝██║  ██║   ██║   ██║  ██║
╚═╝  ╚═╝╚══════╝╚═╝  ╚═══╝╚═════╝ ╚══════╝╚═╝  ╚═╝    ╚═════╝ ╚═╝  ╚═╝   ╚═╝   ╚═╝  ╚═╝ */


std::vector<RenderItem2D> CreateLoadingScreenRenderItems() {

    int desiredTotalLines = 40;
    float linesPerPresentHeight = (float)PRESENT_HEIGHT / (float)TextBlitter::GetLineHeight(BitmapFontType::STANDARD);
    float scaleRatio = (float)desiredTotalLines / (float)linesPerPresentHeight;
    float loadingScreenWidth = PRESENT_WIDTH * scaleRatio;
    float loadingScreenHeight = PRESENT_HEIGHT * scaleRatio;

    std::string text = "";
    int maxLinesDisplayed = 40;
    int endIndex = AssetManager::GetLoadLog().size();
    int beginIndex = std::max(0, endIndex - maxLinesDisplayed);
    for (int i = beginIndex; i < endIndex; i++) {
        text += AssetManager::GetLoadLog()[i] + "\n";
    }

    ivec2 location = ivec2(0.0f, loadingScreenHeight);
    ivec2 viewportSize = ivec2(loadingScreenWidth, loadingScreenHeight);
    return TextBlitter::CreateText(text, location, viewportSize, Alignment::TOP_LEFT, BitmapFontType::STANDARD);
}

std::vector<RenderItem2D> CreateRenderItems2D(ivec2 presentSize, int playerCount) {

    std::vector<RenderItem2D> renderItems;

    for (int i = 0; i < playerCount; i++) {

        // Debug Text
        if (Game::DebugTextIsEnabled()) {
            std::string text = "";
            text += "Splitscreen mode: " + Util::SplitscreenModeToString(Game::GetSplitscreenMode()) + "\n";
            text += "Render mode: " + Util::RenderModeToString(_renderMode) + "\n";
            //text += "Line mode: " + Util::DebugLineRenderModeToString(_debugLineRenderMode) + "\n";
            //text += "Cam pos: " + Util::Vec3ToString(Game::GetPlayerByIndex(i)->GetViewPos()) + "\n";
            text += "Weapon Action: " + Util::WeaponActionToString(Game::GetPlayerByIndex(i)->GetWeaponAction()) + "\n";

            Player* player = Game::GetPlayerByIndex(i);

            text += "\n";
            text += "Current weapon index: " + std::to_string(player->m_currentWeaponIndex) + "\n\n";
            for (int i = 0; i < player->m_weaponStates.size(); i++) {

                if (player->m_weaponStates[i].has) {
                    std::string str;
                    str += player->m_weaponStates[i].name;
                    str += " ";
                    str += "\n";
                    if (i == player->m_currentWeaponIndex) {
                        str = ">" + str;
                    }
                    else {
                        str = " " + str;
                    }
                    text += str;
                }
            }

            text += "\nAMMO\n";
            for (int i = 0; i < player->m_ammoStates.size(); i++) {
                text += "-";
                text += player->m_ammoStates[i].name;
                text += " ";
                text += std::to_string(player->m_ammoStates[i].ammoOnHand);
                text += "\n";
            }


            int x = RendererUtil::GetViewportLeftX(i, Game::GetSplitscreenMode(), presentSize.x, presentSize.y);
            int y = RendererUtil::GetViewportTopY(i, Game::GetSplitscreenMode(), presentSize.x, presentSize.y);
            RendererUtil::AddRenderItems(renderItems, TextBlitter::CreateText(text, { x, y }, presentSize, Alignment::TOP_LEFT, BitmapFontType::STANDARD));
        }

        // Player HUD
        std::vector<RenderItem2D> playerHUD = Game::GetPlayerByIndex(i)->GetHudRenderItems(presentSize);
        RendererUtil::AddRenderItems(renderItems, playerHUD);

    }

    return renderItems;
}

std::vector<RenderItem2D> CreateRenderItems2DHiRes(ivec2 gbufferSize, int playerCount) {

    std::vector<RenderItem2D> renderItems;
    for (int i = 0; i < playerCount; i++) {
        RendererUtil::AddRenderItems(renderItems, Game::GetPlayerByIndex(i)->GetHudRenderItemsHiRes(gbufferSize));
    }
    return renderItems;
}

std::vector<RenderItem3D> CreateRenderItems3D() {

    std::vector<RenderItem3D> renderItems = Scene::GetAllRenderItems();
    return renderItems;
}

std::vector<GPULight> CreateGPULights() {

    std::vector<GPULight> gpuLights;

    for (Light& light : Scene::_lights) {
        GPULight& gpuLight = gpuLights.emplace_back();
        gpuLight.posX = light.position.x;
        gpuLight.posY = light.position.y;
        gpuLight.posZ = light.position.z;
        gpuLight.colorR = light.color.x;
        gpuLight.colorG = light.color.y;
        gpuLight.colorB = light.color.z;
        gpuLight.strength = light.strength;
        gpuLight.radius = light.radius;
    }
    return gpuLights;
}

void UpdateDebugLinesMesh() {

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    std::vector<PxRigidActor*> ignoreList;

    // Skip debug lines for player 0 ragdoll
    Player* player = Game::GetPlayerByIndex(0);
    AnimatedGameObject* characterModel = Scene::GetAnimatedGameObjectByIndex(player->GetCharacterModelAnimatedGameObjectIndex());
    for (auto r : characterModel->_ragdoll._rigidComponents) {
        ignoreList.push_back(r.pxRigidBody);
    }

    if (_debugLineRenderMode == DebugLineRenderMode::PHYSX_ALL ||
        _debugLineRenderMode == DebugLineRenderMode::PHYSX_COLLISION ||
        _debugLineRenderMode == DebugLineRenderMode::PHYSX_RAYCAST ||
        _debugLineRenderMode == DebugLineRenderMode::PHYSX_EDITOR) {
        std::vector<Vertex> physicsDebugLineVertices = Physics::GetDebugLineVertices(_debugLineRenderMode, ignoreList);
        vertices.reserve(vertices.size() + physicsDebugLineVertices.size());
        vertices.insert(std::end(vertices), std::begin(physicsDebugLineVertices), std::end(physicsDebugLineVertices));
    }
    else if (_debugLineRenderMode == DebugLineRenderMode::BOUNDING_BOXES) {
        for (GameObject& gameObject : Scene::GetGamesObjects()) {
            std::vector<Vertex> aabbVertices = gameObject.GetAABBVertices();
            vertices.reserve(vertices.size() + aabbVertices.size());
            vertices.insert(std::end(vertices), std::begin(aabbVertices), std::end(aabbVertices));
        }
    }

    else if (_debugLineRenderMode == DebugLineRenderMode::RTX_LAND_AABBS) {
        for (Wall& wall : Scene::_walls) {
            std::vector<Vertex> aabbVertices = Util::GetAABBVertices(wall.aabb, GREEN);
            vertices.reserve(vertices.size() + aabbVertices.size());
            vertices.insert(std::end(vertices), std::begin(aabbVertices), std::end(aabbVertices));
        }
        for (Floor& floor : Scene::_floors) {
            std::vector<Vertex> aabbVertices = Util::GetAABBVertices(floor.aabb, GREEN);
            vertices.reserve(vertices.size() + aabbVertices.size());
            vertices.insert(std::end(vertices), std::begin(aabbVertices), std::end(aabbVertices));
        }
        for (Ceiling& ceiling : Scene::_ceilings) {
            std::vector<Vertex> aabbVertices = Util::GetAABBVertices(ceiling.aabb, GREEN);
            vertices.reserve(vertices.size() + aabbVertices.size());
            vertices.insert(std::end(vertices), std::begin(aabbVertices), std::end(aabbVertices));
        }
        for (Door& door : Scene::_doors) {
            std::vector<Vertex> aabbVertices = Util::GetAABBVertices(door._aabb, GREEN);
            vertices.reserve(vertices.size() + aabbVertices.size());
            vertices.insert(std::end(vertices), std::begin(aabbVertices), std::end(aabbVertices));
        }
    }
    /*
    Player* player = Game::GetPlayerByIndex(0);

    // Frustum
    float FoV = player->_zoom;
    float yFac = tanf(FoV * HELL_PI / 360.0f);
    float width = (float)BackEnd::GetWindowedWidth();
    float height = (float)BackEnd::GetWindowedHeight();

    glm::mat4 prjMatrix = player->GetProjectionMatrix();

    float aspectRatio = prjMatrix[1][1] / prjMatrix[0][0];;
    float xFac = yFac * aspectRatio;
    float near = NEAR_PLANE;
    float far = FAR_PLANE;
    glm::vec3 viewPos = player->GetViewPos();
    glm::vec3 Forward, Right, Up; //Simply the three columns from your transformation matrix (or the inverse of your view matrix)

    glm::vec3 farLeftTop = viewPos + Forward * far - far * Right * xFac * far + Up * yFac * far;
    glm::vec3 farRightTop = viewPos + Forward * far + far * Right * xFac * far + Up * yFac * far;
    glm::vec3 farLeftBottom = viewPos + Forward * far - far * Right * xFac * far - Up * yFac * far;
    glm::vec3 farRightBottom = viewPos + Forward * far + far * Right * xFac * far - Up * yFac * far;
    glm::vec3 nearLeftTop = viewPos + Forward * near - near * Right * xFac * near + Up * yFac * near;
    glm::vec3 nearRightTop = viewPos + Forward * near + near * Right * xFac * near + Up * yFac * near;
    glm::vec3 nearLeftBottom = viewPos + Forward * near - near * Right * xFac * near - Up * yFac * near;
    glm::vec3 nearRightBottom = viewPos + Forward * near + near * Right * xFac * near - Up * yFac * near;



    glm::vec3 p = player->GetViewPos();
    glm::vec3 d = -player->GetCameraForward();
    float farDist = FAR_PLANE;
    float nearDist = NEAR_PLANE;
    glm::vec3 fc = p + d * farDist;


    float Hfar = 2 * tan(FoV / 2) * farDist;
    float Wfar = Hfar * aspectRatio;
    float Hnear = 2 * tan(FoV / 2) * nearDist;
    float Wnear = Hnear * aspectRatio;

    glm::vec3 up = player->GetCameraUp();
    glm::vec3 right = player->GetCameraRight();

    glm::vec3 ftl = fc + (up * Hfar / 2.0f) - (right * Wfar / 2.0f);
    glm::vec3 ftr = fc + (up * Hfar / 2.0f) + (right * Wfar / 2.0f);
    glm::vec3 fbl = fc - (up * Hfar / 2.0f) - (right * Wfar / 2.0f);
    glm::vec3 fbr = fc - (up * Hfar / 2.0f) + (right * Wfar / 2.0f);


    glm::vec3 nc = p + d * nearDist;

    glm::vec3 ntl = nc + (up * Hnear / 2.0f) - (right * Wnear / 2.0f);
    glm::vec3 ntr = nc + (up * Hnear / 2.0f) + (right * Wnear / 2.0f);
    glm::vec3 nbl = nc - (up * Hnear / 2.0f) - (right * Wnear / 2.0f);
    glm::vec3 nbr = nc - (up * Hnear / 2.0f) + (right * Wnear / 2.0f);

    Vertex cornerA;
    Vertex cornerB;
    Vertex cornerC;
    Vertex cornerD;
    Vertex cornerA2;
    Vertex cornerB2;
    Vertex cornerC2;
    Vertex cornerD2;
    cornerA.normal = GREEN;
    cornerB.normal = GREEN;
    cornerC.normal = GREEN;
    cornerD.normal = GREEN;
    cornerA2.normal = GREEN;
    cornerB2.normal = GREEN;
    cornerC2.normal = GREEN;
    cornerD2.normal = GREEN;

    cornerA.position = nbl;
    cornerB.position = nbr;
    cornerC.position = ntl;
    cornerD.position = ntr;

    cornerA2.position = fbl;
    cornerB2.position = fbr;
    cornerC2.position = ftl;
    cornerD2.position = ftr;

    vertices.push_back(cornerA);
    vertices.push_back(cornerB);

    vertices.push_back(cornerC);
    vertices.push_back(cornerD);

    vertices.push_back(cornerA);
    vertices.push_back(cornerC);

    vertices.push_back(cornerB);
    vertices.push_back(cornerD);

    vertices.push_back(cornerA2);
    vertices.push_back(cornerB2);

    vertices.push_back(cornerC2);
    vertices.push_back(cornerD2);

    vertices.push_back(cornerA2);
    vertices.push_back(cornerC2);

    vertices.push_back(cornerB2);
    vertices.push_back(cornerD2);


    vertices.push_back(cornerA);
    vertices.push_back(cornerA2);

    vertices.push_back(cornerB);
    vertices.push_back(cornerB2);

    vertices.push_back(cornerC);
    vertices.push_back(cornerC2);

    vertices.push_back(cornerD);
    vertices.push_back(cornerD2);
    */

   // std::cout << Util::Vec3ToString(cornerA.position) << " " << Util::Vec3ToString(cornerB.position) << "\n";

    for (int i = 0; i < vertices.size(); i++) {
        indices.push_back(i);
    }
    _debugLinesMesh.UpdateVertexBuffer(vertices, indices);
}

void UpdateDebugPointsMesh() {

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;


    Player* player = Game::GetPlayerByIndex(0);
    Vertex v;
    v.position = player->GetPistolCasingSpawnPostion();
    v.normal = RED;
//    vertices.push_back(v);


    for (auto& object : Scene::GetAnimatedGamesObjects()) {

        if (object._skinnedModel->_filename == "Glock") {



            glm::mat4 muzzleMatrix = object.GetJointWorldTransformByName("Barrel");
           // glm::mat4 ejectionPortMatrix = object.GetJointWorldTransformByName("Ejection");



            glm::vec3 offest = glm::vec3(0, 0, 0.1);
            offest /= object.GetScale();

            Vertex v;
            v.position = muzzleMatrix * glm::vec4(offest, 1);
            v.normal = RED;
          //  vertices.push_back(v);
            /*
            Vertex v2;
            v2.position = ejectionPortMatrix * glm::vec4(0, 0, 0, 1);
            v2.normal = GREEN;
            vertices.push_back(v2);*/
        }

    }

   // int index = Game::GetPlayerByIndex(0)->GetFirstPersonWeaponAnimatedGameObjectIndex();
   // index = Game::GetPlayerByIndex(1)->GetCharacterModelAnimatedGameObjectIndex();
   // AnimatedGameObject* object = Scene::GetAnimatedGameObjectByIndex(index);
  //  SkinnedModel* skinnedModel = object->_skinnedModel;

    for (int i = 0; i < vertices.size(); i++) {
        indices.push_back(i);
    }

    _debugPointsMesh.UpdateVertexBuffer(vertices, indices);
}

BlitDstCoords GetBlitDstCoordsPresent(unsigned int playerIndex) {
    BlitDstCoords coords;
    coords.dstX0 = 0;
    coords.dstY0 = 0;
    coords.dstX1 = PRESENT_WIDTH;
    coords.dstY1 = PRESENT_HEIGHT;
    if (playerIndex == 2) {
        if (Game::GetSplitscreenMode() == SplitscreenMode::TWO_PLAYER) {
            coords.dstY0 = PRESENT_HEIGHT / 2;
            coords.dstY1 = PRESENT_HEIGHT;
        }
        if (Game::GetSplitscreenMode() == SplitscreenMode::FOUR_PLAYER) {
            coords.dstX0 = 0;
            coords.dstX1 = PRESENT_WIDTH / 2;
            coords.dstY0 = PRESENT_HEIGHT / 2;
            coords.dstY1 = PRESENT_HEIGHT;
        }
    }
    if (playerIndex == 3) {

        if (Game::GetSplitscreenMode() == SplitscreenMode::TWO_PLAYER) {
            coords.dstY0 = PRESENT_HEIGHT / 2;
            coords.dstY1 = PRESENT_HEIGHT;
            coords.dstY0 = 0;
            coords.dstY1 = PRESENT_HEIGHT / 2;
        }
        if (Game::GetSplitscreenMode() == SplitscreenMode::FOUR_PLAYER) {
            coords.dstX0 = PRESENT_WIDTH / 2;
            coords.dstX1 = PRESENT_WIDTH;
            coords.dstY0 = PRESENT_HEIGHT / 2;
            coords.dstY1 = PRESENT_HEIGHT;
        }
    }
    if (playerIndex == 0) {

        if (Game::GetSplitscreenMode() == SplitscreenMode::TWO_PLAYER) {
            coords.dstX0 = 0;
            coords.dstX1 = PRESENT_WIDTH;
            coords.dstY0 = 0;
            coords.dstY1 = PRESENT_HEIGHT / 2;
        }
        if (Game::GetSplitscreenMode() == SplitscreenMode::FOUR_PLAYER) {
            coords.dstX0 = 0;
            coords.dstX1 = PRESENT_WIDTH / 2;
            coords.dstY0 = 0;
            coords.dstY1 = PRESENT_HEIGHT / 2;
        }
    }
    if (playerIndex == 1) {

        if (Game::GetSplitscreenMode() == SplitscreenMode::TWO_PLAYER) {
            coords.dstX0 = 0;
            coords.dstX1 = PRESENT_WIDTH;
            coords.dstY0 = PRESENT_HEIGHT / 2;
            coords.dstY1 = PRESENT_HEIGHT;
        }
        if (Game::GetSplitscreenMode() == SplitscreenMode::FOUR_PLAYER) {
            coords.dstX0 = PRESENT_WIDTH / 2;
            coords.dstX1 = PRESENT_WIDTH;
            coords.dstY0 = 0;
            coords.dstY1 = PRESENT_HEIGHT / 2;
        }
    }


    return coords;
}

BlitDstCoords GetBlitDstCoords(unsigned int playerIndex) {
    BlitDstCoords coords;
    coords.dstX0 = 0;
    coords.dstY0 = 0;
    coords.dstX1 = BackEnd::GetCurrentWindowWidth();
    coords.dstY1 = BackEnd::GetCurrentWindowHeight();
    if (playerIndex == 0) {
        if (Game::GetSplitscreenMode() == SplitscreenMode::TWO_PLAYER) {
            coords.dstY0 = BackEnd::GetCurrentWindowHeight() * 0.5f;
            coords.dstY1 = BackEnd::GetCurrentWindowHeight();
        }
        if (Game::GetSplitscreenMode() == SplitscreenMode::FOUR_PLAYER) {
            coords.dstX0 = 0;
            coords.dstX1 = BackEnd::GetCurrentWindowWidth() * 0.5f;
            coords.dstY0 = BackEnd::GetCurrentWindowHeight() * 0.5f;
            coords.dstY1 = BackEnd::GetCurrentWindowHeight();
        }
    }
    if (playerIndex == 1) {

        if (Game::GetSplitscreenMode() == SplitscreenMode::TWO_PLAYER) {
            coords.dstY0 = BackEnd::GetCurrentWindowHeight() * 0.5f;
            coords.dstY1 = BackEnd::GetCurrentWindowHeight();
            coords.dstY0 = 0;
            coords.dstY1 = BackEnd::GetCurrentWindowHeight() * 0.5f;
        }
        if (Game::GetSplitscreenMode() == SplitscreenMode::FOUR_PLAYER) {
            coords.dstX0 = BackEnd::GetCurrentWindowWidth() * 0.5f;
            coords.dstX1 = BackEnd::GetCurrentWindowWidth();
            coords.dstY0 = BackEnd::GetCurrentWindowHeight() * 0.5f;
            coords.dstY1 = BackEnd::GetCurrentWindowHeight();
        }
    }
    if (playerIndex == 2) {

        if (Game::GetSplitscreenMode() == SplitscreenMode::TWO_PLAYER) {
            coords;
        }
        if (Game::GetSplitscreenMode() == SplitscreenMode::FOUR_PLAYER) {
            coords.dstX0 = 0;
            coords.dstX1 = BackEnd::GetCurrentWindowWidth() * 0.5f;
            coords.dstY0 = 0;
            coords.dstY1 = BackEnd::GetCurrentWindowHeight() * 0.5f;
        }
    }
    if (playerIndex == 3) {

        if (Game::GetSplitscreenMode() == SplitscreenMode::TWO_PLAYER) {
            coords;
        }
        if (Game::GetSplitscreenMode() == SplitscreenMode::FOUR_PLAYER) {
            coords.dstX0 = BackEnd::GetCurrentWindowWidth() * 0.5f;
            coords.dstX1 = BackEnd::GetCurrentWindowWidth();
            coords.dstY0 = 0;
            coords.dstY1 = BackEnd::GetCurrentWindowHeight() * 0.5f;
        }
    }
    return coords;
}

MuzzleFlashData GetMuzzleFlashData(unsigned int playerIndex) {

    Player* player = Game::GetPlayerByIndex(playerIndex);

    int rows = 5;
    int columns = 4;
    float time =player->GetMuzzleFlashTime() * 0.25f;
    float duration = 1.0;

    auto dt = duration / static_cast<float>(rows * columns - 1);
    int frameIndex = (int)std::floorf(time / dt);
    float interpolate = (time - frameIndex * dt) / dt;

    glm::vec3 viewRotation = player->GetViewRotation();

    glm::vec3 worldPos = player->GetMuzzleFlashPosition();

    Transform worldTransform;
    worldTransform.position = worldPos;
    worldTransform.rotation = viewRotation;

    Transform localTransform;
    localTransform.rotation.z = player->GetMuzzleFlashRotation();;
    localTransform.scale = glm::vec3(0.25f, 0.125f, 1);
    localTransform.scale.x *= 0.375;
    localTransform.scale.y *= 0.375;

    if (time > duration * 0.125f) {
        localTransform.scale = glm::vec3(0);
    }

    MuzzleFlashData muzzleFlashData;
    muzzleFlashData.RowCount = rows;
    muzzleFlashData.ColumnCont = columns;
    muzzleFlashData.frameIndex = frameIndex;
    muzzleFlashData.timeLerp = interpolate;
    muzzleFlashData.modelMatrix = worldTransform.to_mat4() * localTransform.to_mat4();

    return muzzleFlashData;
}


std::vector<RenderItem3D> CreateDecalRenderItems() {

    static Material* bulletHolePlasterMaterial = AssetManager::GetMaterialByIndex(AssetManager::GetMaterialIndex("BulletHole_Plaster"));
    static Material* bulletHoleGlassMaterial = AssetManager::GetMaterialByIndex(AssetManager::GetMaterialIndex("BulletHole_Glass"));
    std::vector<RenderItem3D> renderItems;
    renderItems.reserve(Scene::GetBulletHoleDecalCount());

    for (int i = 0; i < Scene::GetBulletHoleDecalCount(); i++) {
        BulletHoleDecal* decal = Scene::GetBulletHoleDecalByIndex(i);
        RenderItem3D& renderItem = renderItems.emplace_back();
        renderItem.modelMatrix = decal->GetModelMatrix();
        renderItem.meshIndex = AssetManager::GetQuadMeshIndex();
        if (decal->GetType() == BulletHoleDecalType::REGULAR) {
            renderItem.baseColorTextureIndex = bulletHolePlasterMaterial->_basecolor;
            renderItem.rmaTextureIndex = bulletHolePlasterMaterial->_rma;
            renderItem.normalTextureIndex = bulletHolePlasterMaterial->_normal;
        }
        else if (decal->GetType() == BulletHoleDecalType::GLASS) {
            renderItem.baseColorTextureIndex = bulletHoleGlassMaterial->_basecolor;
            renderItem.rmaTextureIndex = bulletHoleGlassMaterial->_rma;
            renderItem.normalTextureIndex = bulletHoleGlassMaterial->_normal;
        }
    }
    return renderItems;
}

std::vector<RenderItem3D> CreateBloodDecalRenderItems() {

    static int textureIndexType0 = AssetManager::GetTextureIndexByName("blood_decal_4");
    static int textureIndexType1 = AssetManager::GetTextureIndexByName("blood_decal_6");
    static int textureIndexType2 = AssetManager::GetTextureIndexByName("blood_decal_7");
    static int textureIndexType3 = AssetManager::GetTextureIndexByName("blood_decal_9");

    std::vector<RenderItem3D> renderItems;
    renderItems.reserve(Scene::_bloodDecals.size());

    for (BloodDecal& decal : Scene::_bloodDecals) {

        RenderItem3D& renderItem = renderItems.emplace_back();
        renderItem.meshIndex = AssetManager::GetUpFacingPlaneMeshIndex();
        renderItem.modelMatrix = decal.modelMatrix;
        renderItem.inverseModelMatrix = glm::inverse(renderItem.modelMatrix);

        if (decal.type == 0) {
            renderItem.baseColorTextureIndex = textureIndexType0;
        }
        else if (decal.type == 1) {
            renderItem.baseColorTextureIndex = textureIndexType1;
        }
        else if (decal.type == 2) {
            renderItem.baseColorTextureIndex = textureIndexType2;
        }
        else if (decal.type == 3) {
            renderItem.baseColorTextureIndex = textureIndexType3;
        }
    }

    return renderItems;
}


std::vector<RenderItem3D> CreateBloodVATRenderItems() {

    static int textureIndexBloodPos4 = AssetManager::GetTextureIndexByName("blood_pos4");
    static int textureIndexBloodPos6 = AssetManager::GetTextureIndexByName("blood_pos6");
    static int textureIndexBloodPos7 = AssetManager::GetTextureIndexByName("blood_pos7");
    static int textureIndexBloodPos9 = AssetManager::GetTextureIndexByName("blood_pos9");
    static int textureIndexBloodNorm4 = AssetManager::GetTextureIndexByName("blood_norm4");
    static int textureIndexBloodNorm6 = AssetManager::GetTextureIndexByName("blood_norm6");
    static int textureIndexBloodNorm7 = AssetManager::GetTextureIndexByName("blood_norm7");
    static int textureIndexBloodNorm9 = AssetManager::GetTextureIndexByName("blood_norm9");
    static int meshIndex4 = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("blood_mesh4"))->GetMeshIndices()[0];
    static int meshIndex6 = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("blood_mesh6"))->GetMeshIndices()[0];
    static int meshIndex7 = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("blood_mesh7"))->GetMeshIndices()[0];
    static int meshIndex9 = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("blood_mesh9"))->GetMeshIndices()[0];

    std::vector<RenderItem3D> renderItems;
    renderItems.reserve(Scene::_volumetricBloodSplatters.size());

    for (auto& bloodVAT : Scene::_volumetricBloodSplatters) {

        RenderItem3D& renderItem = renderItems.emplace_back();
        renderItem.modelMatrix = bloodVAT.GetModelMatrix();
        renderItem.inverseModelMatrix = glm::inverse(renderItem.modelMatrix);
        renderItem.emissiveColor.r = bloodVAT.m_CurrentTime;

        if (bloodVAT.m_type == 4) {
            renderItem.baseColorTextureIndex = textureIndexBloodPos4;
            renderItem.normalTextureIndex = textureIndexBloodNorm4;
            renderItem.meshIndex = meshIndex4;
        }
        else if (bloodVAT.m_type == 6) {
            renderItem.baseColorTextureIndex = textureIndexBloodPos6;
            renderItem.normalTextureIndex = textureIndexBloodNorm6;
            renderItem.meshIndex = meshIndex6;
        }
        else if (bloodVAT.m_type == 7) {
            renderItem.baseColorTextureIndex = textureIndexBloodPos7;
            renderItem.normalTextureIndex = textureIndexBloodNorm7;
            renderItem.meshIndex = meshIndex7;
        }
        else if (bloodVAT.m_type == 9) {
            renderItem.baseColorTextureIndex = textureIndexBloodPos9;
            renderItem.normalTextureIndex = textureIndexBloodNorm9;
            renderItem.meshIndex = meshIndex9;
        }
        else {
            continue;
        }
    }
    return renderItems;
}

struct Frustum {

    bool ContainsAABB(glm::vec3 aabbMin, glm::vec3 aabbMax) {
        return true;
    }

};

std::vector<DrawIndexedIndirectCommand> CreateMultiDrawIndirectCommands(std::vector<RenderItem3D>& renderItems) {

    std::vector<DrawIndexedIndirectCommand> commands;
    std::sort(renderItems.begin(), renderItems.end());
    int baseInstance = 0;

    for (RenderItem3D& renderItem : renderItems) {

        Mesh* mesh = AssetManager::GetMeshByIndex(renderItem.meshIndex);
        bool found = false;

        // Does a draw command already exist for this mesh?
        for (auto& cmd : commands) {
            // If so then increment the instance count
            if (cmd.baseVertex == mesh->baseVertex) {
                cmd.instanceCount++;
                baseInstance++;
                found = true;
                break;
            }
        }
        // If not, then create the command
        if (!found) {
            auto& cmd = commands.emplace_back();
            cmd.indexCount = mesh->indexCount;
            cmd.firstIndex = mesh->baseIndex;
            cmd.baseVertex = mesh->baseVertex;
            cmd.baseInstance = baseInstance;
            cmd.instanceCount = 1;
            baseInstance++;
        }
    }
    return commands;
}


MultiDrawIndirectDrawInfo CreateMultiDrawIndirectDrawInfo(std::vector<RenderItem3D>& renderItems) {

    MultiDrawIndirectDrawInfo drawInfo;
    drawInfo.renderItems = renderItems;
    std::sort(drawInfo.renderItems.begin(), drawInfo.renderItems.end());

    // Create indirect draw commands
    drawInfo.commands.clear();
    int baseInstance = 0;
    for (RenderItem3D& renderItem : drawInfo.renderItems) {
        Mesh* mesh = AssetManager::GetMeshByIndex(renderItem.meshIndex);
        bool found = false;
        // Does a draw command already exist for this mesh?
        for (auto& cmd : drawInfo.commands) {
            // If so then increment the instance count
            if (cmd.baseVertex == mesh->baseVertex) {
                cmd.instanceCount++;
                baseInstance++;
                found = true;
                break;
            }
        }
        // If not, then create the command
        if (!found) {
            auto& cmd = drawInfo.commands.emplace_back();
            cmd.indexCount = mesh->indexCount;
            cmd.firstIndex = mesh->baseIndex;
            cmd.baseVertex = mesh->baseVertex;
            cmd.baseInstance = baseInstance;
            cmd.instanceCount = 1;
            baseInstance++;
        }
    }
    return drawInfo;
}

std::vector<SkinnedRenderItem> GetSkinnedRenderItemsForPlayer (int playerIndex) {

    Player* player = Game::GetPlayerByIndex(playerIndex);
    std::vector<SkinnedRenderItem> skinnedMeshRenderItems;

    for (int i = 0; i < Scene::GetAnimatedGameObjectCount(); i++) {
        AnimatedGameObject* animatedGameObject = Scene::GetAnimatedGameObjectByIndex(i);

        if (animatedGameObject->GetFlag() == AnimatedGameObject::Flag::CHARACTER_MODEL && i == player->GetCharacterModelAnimatedGameObjectIndex() ||
            animatedGameObject->GetFlag() == AnimatedGameObject::Flag::FIRST_PERSON_WEAPON && i != player->GetViewWeaponAnimatedGameObjectIndex()) {
            continue;
        }
        std::vector<SkinnedRenderItem>& items = animatedGameObject->GetSkinnedMeshRenderItems();
        skinnedMeshRenderItems.insert(skinnedMeshRenderItems.end(), items.begin(), items.end());
    }
    return skinnedMeshRenderItems;
}

RenderData CreateRenderData() {

    // Viewport size
    ivec2 viewportSize = { PRESENT_WIDTH, PRESENT_HEIGHT };
    ivec2 viewportDoubleSize = { PRESENT_WIDTH * 2, PRESENT_HEIGHT * 2 };

    if (Game::GetSplitscreenMode() == SplitscreenMode::TWO_PLAYER) {
        viewportSize.y *= 0.5;
        viewportDoubleSize.y *= 0.5;
    }
    if (Game::GetSplitscreenMode() == SplitscreenMode::FOUR_PLAYER) {
        viewportSize.x *= 0.5;
        viewportSize.y *= 0.5;
        viewportDoubleSize.x *= 0.5;
        viewportDoubleSize.y *= 0.5;
    }

    ivec2 presentSize = { PRESENT_WIDTH, PRESENT_HEIGHT };
    ivec2 gbufferSize = { PRESENT_WIDTH, PRESENT_HEIGHT };

    std::vector<RenderItem3D> decalRenderItems = Scene::CreateDecalRenderItems();
    std::vector<RenderItem3D> geometryRenderItems = CreateRenderItems3D();
    std::vector<RenderItem3D> glassRenderItems = CreateGlassRenderItems();
    std::vector<RenderItem3D> bloodDecalRenderItems = CreateBloodDecalRenderItems();
    std::vector<RenderItem3D> bloodVATRenderItems = CreateBloodVATRenderItems();

    // Cull shit for lights
    std::vector<RenderItem3D> shadowMapGeometryRenderItems;
    for (RenderItem3D& renderItem : geometryRenderItems) {
        if (renderItem.castShadow) {
            shadowMapGeometryRenderItems.push_back(renderItem);
        }
    }

    RenderData renderData;

    // Player count
    if (Game::GetSplitscreenMode() == SplitscreenMode::NONE) {
        renderData.playerCount = 1;
    }
    else if (Game::GetSplitscreenMode() == SplitscreenMode::TWO_PLAYER) {
        renderData.playerCount = 2;
    }
    else if (Game::GetSplitscreenMode() == SplitscreenMode::FOUR_PLAYER) {
        renderData.playerCount = 4;
    }



    // Create render items
    renderData.renderItems.geometry = CreateRenderItems3D();
    renderData.renderItems.decals = Scene::CreateDecalRenderItems();

    // Sort render items by mesh index
    std::sort(renderData.renderItems.geometry.begin(), renderData.renderItems.geometry.end());
    std::sort(renderData.renderItems.decals.begin(), renderData.renderItems.decals.end());

    renderData.lights = CreateGPULights();
    renderData.debugLinesMesh = &_debugLinesMesh;
    renderData.debugPointsMesh = &_debugPointsMesh;
    renderData.renderDebugLines = (_debugLineRenderMode != DebugLineRenderMode::SHOW_NO_LINES);
    renderData.renderItems2D = CreateRenderItems2D(presentSize, renderData.playerCount);
    renderData.renderItems2DHiRes = CreateRenderItems2DHiRes(gbufferSize, renderData.playerCount);
    renderData.blitDstCoords = GetBlitDstCoords(0);
    renderData.blitDstCoordsPresent = GetBlitDstCoordsPresent(0);


    for (int i = 0; i < renderData.playerCount; i++) {
        renderData.muzzleFlashData[i] = GetMuzzleFlashData(i);
    }

    renderData.animatedTransforms = &_animatedTransforms;
    renderData.finalImageColorTint = Game::GetPlayerByIndex(0)->finalImageColorTint;
    renderData.finalImageColorContrast = Game::GetPlayerByIndex(0)->finalImageContrast;
    renderData.renderMode = _renderMode;

    renderData.geometryDrawInfo = CreateMultiDrawIndirectDrawInfo(geometryRenderItems);
    renderData.bulletHoleDecalDrawInfo = CreateMultiDrawIndirectDrawInfo(decalRenderItems);
    renderData.glassDrawInfo = CreateMultiDrawIndirectDrawInfo(glassRenderItems);
    renderData.bloodDecalDrawInfo = CreateMultiDrawIndirectDrawInfo(bloodDecalRenderItems);
    renderData.shadowMapGeometryDrawInfo = CreateMultiDrawIndirectDrawInfo(shadowMapGeometryRenderItems);
    renderData.bloodVATDrawInfo = CreateMultiDrawIndirectDrawInfo(bloodVATRenderItems);


    // Camera data
    for (int i = 0; i < renderData.playerCount; i++) {
        Player* player = Game::GetPlayerByIndex(i);
        renderData.cameraData[i].projection = player->GetProjectionMatrix();
        renderData.cameraData[i].projectionInverse = glm::inverse(renderData.cameraData[i].projection);
        renderData.cameraData[i].view = player->GetViewMatrix();
        renderData.cameraData[i].viewInverse = glm::inverse(renderData.cameraData[i].view);
        renderData.cameraData[i].colorMultiplierR = Game::GetPlayerByIndex(i)->finalImageColorTint.x;
        renderData.cameraData[i].colorMultiplierG = Game::GetPlayerByIndex(i)->finalImageColorTint.y;
        renderData.cameraData[i].colorMultiplierB = Game::GetPlayerByIndex(i)->finalImageColorTint.z;
        renderData.cameraData[i].contrast = Game::GetPlayerByIndex(i)->finalImageContrast;

        // Viewport size
        ivec2 viewportSize = { PRESENT_WIDTH, PRESENT_HEIGHT };
        ivec2 viewportDoubleSize = { PRESENT_WIDTH * 2, PRESENT_HEIGHT * 2 };

        if (Game::GetSplitscreenMode() == SplitscreenMode::TWO_PLAYER) {
            viewportSize.y *= 0.5;
            viewportDoubleSize.y *= 0.5;
        }
        if (Game::GetSplitscreenMode() == SplitscreenMode::FOUR_PLAYER) {
            viewportSize.x *= 0.5;
            viewportSize.y *= 0.5;
            viewportDoubleSize.x *= 0.5;
            viewportDoubleSize.y *= 0.5;
        }

        renderData.cameraData[i].viewportWidth = viewportDoubleSize.x;
        renderData.cameraData[i].viewportHeight = viewportDoubleSize.y;

        // Clipspace range
        if (Game::GetSplitscreenMode() == SplitscreenMode::NONE) {
            renderData.cameraData[i].clipSpaceXMin = 0.0f;
            renderData.cameraData[i].clipSpaceXMax = 1.0f;
            renderData.cameraData[i].clipSpaceYMin = 0.0f;
            renderData.cameraData[i].clipSpaceYMax = 1.0f;
        }
        else if (Game::GetSplitscreenMode() == SplitscreenMode::TWO_PLAYER) {
            if (i == 0) {
                renderData.cameraData[i].clipSpaceXMin = 0.0f;
                renderData.cameraData[i].clipSpaceXMax = 1.0f;
                renderData.cameraData[i].clipSpaceYMin = 0.5f;
                renderData.cameraData[i].clipSpaceYMax = 1.0f;
            }
            if (i == 1) {
                renderData.cameraData[i].clipSpaceXMin = 0.0f;
                renderData.cameraData[i].clipSpaceXMax = 1.0f;
                renderData.cameraData[i].clipSpaceYMin = 0.0f;
                renderData.cameraData[i].clipSpaceYMax = 0.5f;
            }
        }
        else if (Game::GetSplitscreenMode() == SplitscreenMode::FOUR_PLAYER) {
            if (i == 0) {
                renderData.cameraData[i].clipSpaceXMin = 0.0f;
                renderData.cameraData[i].clipSpaceXMax = 0.5f;
                renderData.cameraData[i].clipSpaceYMin = 0.5f;
                renderData.cameraData[i].clipSpaceYMax = 1.0f;
            }
            if (i == 1) {
                renderData.cameraData[i].clipSpaceXMin = 0.5f;
                renderData.cameraData[i].clipSpaceXMax = 1.0f;
                renderData.cameraData[i].clipSpaceYMin = 0.5f;
                renderData.cameraData[i].clipSpaceYMax = 1.0f;
            }
            if (i == 2) {
                renderData.cameraData[i].clipSpaceXMin = 0.0f;
                renderData.cameraData[i].clipSpaceXMax = 0.5f;
                renderData.cameraData[i].clipSpaceYMin = 0.0f;
                renderData.cameraData[i].clipSpaceYMax = 0.5f;
            }
            if (i == 3) {
                renderData.cameraData[i].clipSpaceXMin = 0.5f;
                renderData.cameraData[i].clipSpaceXMax = 1.0f;
                renderData.cameraData[i].clipSpaceYMin = 0.0f;
                renderData.cameraData[i].clipSpaceYMax = 0.5f;
            }
        }

        ViewportInfo viewportInfo = RendererUtil::CreateViewportInfo(i, Game::GetSplitscreenMode(), PRESENT_WIDTH * 2, PRESENT_HEIGHT * 2);
        renderData.cameraData[i].viewportOffsetX = viewportInfo.xOffset;
        renderData.cameraData[i].viewportOffsetY = viewportInfo.yOffset;
    }




    // Get everything you need to skin
    renderData.animatedGameObjectsToSkin = Scene::GetAnimatedGamesObjectsToSkin();

    // Now conjoin all their matrices in one big ol' buffer
    uint32_t baseAnimatedTransformIndex = 0;
    uint32_t baseSkinnedVertex = 0;

    for (int i = 0; i < renderData.animatedGameObjectsToSkin.size(); i++) {
        AnimatedGameObject& animatedGameObject = *renderData.animatedGameObjectsToSkin[i];

        animatedGameObject.SetBaseSkinnedVertex(baseSkinnedVertex);
        animatedGameObject.SetBaseTransformIndex(baseAnimatedTransformIndex);

        const size_t transformCount = animatedGameObject.GetAnimatedTransformCount();
        //renderData.animatedGameObjectsToSkin.push_back(&animatedGameObject);
        renderData.baseAnimatedTransformIndices.push_back(baseAnimatedTransformIndex);
        renderData.skinningTransforms.insert(renderData.skinningTransforms.end(), animatedGameObject._animatedTransforms.local.begin(), animatedGameObject._animatedTransforms.local.end());
        baseAnimatedTransformIndex += animatedGameObject._animatedTransforms.GetSize();
        baseSkinnedVertex += animatedGameObject.GetVerteXCount();
    }

    // Create render items
    for (AnimatedGameObject* animatedGameObject : renderData.animatedGameObjectsToSkin) {
        animatedGameObject->CreateSkinnedMeshRenderItems();
    }

    // Get per player animated render items
    renderData.skinnedRenderItems[0] = GetSkinnedRenderItemsForPlayer(0);
    renderData.skinnedRenderItems[1] = GetSkinnedRenderItemsForPlayer(1);
    renderData.skinnedRenderItems[2] = GetSkinnedRenderItemsForPlayer(2);
    renderData.skinnedRenderItems[3] = GetSkinnedRenderItemsForPlayer(3);

    //std::vector<SkinnedRenderItem> allSkinnedRenderItems;
    for (int i = 0; i < renderData.playerCount; i++) {
        renderData.allSkinnedRenderItems.insert(renderData.allSkinnedRenderItems.end(), renderData.skinnedRenderItems[i].begin(), renderData.skinnedRenderItems[i].end());
    }

    std::vector<RenderItem3D> geometryRenderItems2 = CreateRenderItems3D();
    std::vector<RenderItem3D> bulletHoleDecalRenderItems = Scene::CreateDecalRenderItems();

    renderData.geometryIndirectDrawInfo = CreateIndirectDrawInfo(geometryRenderItems2, 4);
    renderData.bulletHoleDecalIndirectDrawInfo = CreateIndirectDrawInfo(bulletHoleDecalRenderItems, 4);




    return renderData;
}


std::vector<RenderItem3D> CreateGlassRenderItems() {

    std::vector<RenderItem3D> renderItems;

    for (Window& window : Scene::_windows) {

        static Model* glassModel = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Glass"));
        static Material* glassMaterial = AssetManager::GetMaterialByIndex(AssetManager::GetMaterialIndex("WindowExterior"));

        for (int i = 0; i < glassModel->GetMeshIndices().size(); i++) {
            int meshIndex = glassModel->GetMeshIndices()[i];
            Mesh* mesh = AssetManager::GetMeshByIndex(meshIndex);
            RenderItem3D& renderItem = renderItems.emplace_back();
            renderItem.meshIndex = meshIndex;
            renderItem.baseColorTextureIndex = glassMaterial->_basecolor;
            renderItem.normalTextureIndex = glassMaterial->_normal;
            renderItem.rmaTextureIndex = glassMaterial->_rma;
            renderItem.indexOffset = mesh->baseIndex;
            renderItem.vertexOffset = mesh->baseVertex;
            renderItem.modelMatrix = window.GetModelMatrix();
            renderItem.inverseModelMatrix = glm::inverse(renderItem.modelMatrix);
        }
    }
    return renderItems;
}

void Renderer::UpdatePointCloud() {

    if (BackEnd::GetAPI() == API::OPENGL) {
        OpenGLRenderer::UpdatePointCloud();
    }
    else if (BackEnd::GetAPI() == API::VULKAN) {
        // TODO: VulkanRenderer::UpdatePointCloud();
    }
}

/*

██████╗ ███████╗███╗   ██╗██████╗ ███████╗██████╗     ███████╗██████╗  █████╗ ███╗   ███╗███████╗
██╔══██╗██╔════╝████╗  ██║██╔══██╗██╔════╝██╔══██╗    ██╔════╝██╔══██╗██╔══██╗████╗ ████║██╔════╝
██████╔╝█████╗  ██╔██╗ ██║██║  ██║█████╗  ██████╔╝    █████╗  ██████╔╝███████║██╔████╔██║█████╗
██╔══██╗██╔══╝  ██║╚██╗██║██║  ██║██╔══╝  ██╔══██╗    ██╔══╝  ██╔══██╗██╔══██║██║╚██╔╝██║██╔══╝
██║  ██║███████╗██║ ╚████║██████╔╝███████╗██║  ██║    ██║     ██║  ██║██║  ██║██║ ╚═╝ ██║███████╗
╚═╝  ╚═╝╚══════╝╚═╝  ╚═══╝╚═════╝ ╚══════╝╚═╝  ╚═╝    ╚═╝     ╚═╝  ╚═╝╚═╝  ╚═╝╚═╝     ╚═╝╚══════╝ */

IndirectDrawInfo CreateIndirectDrawInfo(std::vector<RenderItem3D>& potentialRenderItems, int playerCount) {

    IndirectDrawInfo drawInfo;
    std::vector<RenderItem3D> playerRenderItems[PLAYER_COUNT];

    // Allocate memory
    drawInfo.instanceData.reserve(MAX_RENDER_OBJECTS_3D * PLAYER_COUNT);
    for (int i = 0; i < PLAYER_COUNT; i++) {
        playerRenderItems[i].reserve(MAX_RENDER_OBJECTS_3D);
        drawInfo.playerDrawCommands[i].reserve(MAX_INDIRECT_COMMANDS);
    }

    // Create draw commands for each player, frustum culling against the potential render items
    for (int playerIndex = 0; playerIndex < playerCount; playerIndex++) {
        for (int i = 0; i < potentialRenderItems.size(); i++) {

            RenderItem3D& renderItem = potentialRenderItems[i];

            /*
            glm::vec4 position = renderItem.modelMatrix[3];
            if (playerIndex == 0 && position.x > 8) {
                continue;
            }
            if (playerIndex == 2 && position.x > 4) {
                continue;
            }*/

            playerRenderItems[playerIndex].push_back(renderItem);
        }
        drawInfo.playerDrawCommands[playerIndex] = CreateMultiDrawIndirectCommands(playerRenderItems[playerIndex]);
    }

    // Calculate per player instance data offsets
    int baseInstance = 0;
    for (int playerIndex = 0; playerIndex < playerCount; playerIndex++) {
        drawInfo.instanceDataOffests[playerIndex] = baseInstance;
        baseInstance += playerRenderItems[playerIndex].size();
    }

    // Copy everything into one giant sequential vector
    for (int playerIndex = 0; playerIndex < playerCount; playerIndex++) {
        drawInfo.instanceData.insert(drawInfo.instanceData.end(), playerRenderItems[playerIndex].begin(), playerRenderItems[playerIndex].end());
    }

    return drawInfo;
}


void Renderer::RenderFrame() {

    // Global illumination
    UpdatePointCloud();

    // Update debug mesh
    UpdateDebugLinesMesh();
    UpdateDebugPointsMesh();

    // Resize render targets if required
    if (Input::KeyPressed(HELL_KEY_V)) {
        Game::NextSplitScreenMode();
        Renderer::RecreateBlurBuffers();
    }

    // Game
    if (Game::GetGameMode() == Game::GameMode::GAME) {

        RenderData renderData = CreateRenderData();
        RenderGame(renderData);
        PresentFinalImage();
    }
    // 3D editor
    else if (Game::GetGameMode() == Game::GameMode::EDITOR_3D) {

        RenderData renderData;
        RenderEditor3D(renderData);
    }
    // Top down editor
    else if (Game::GetGameMode() == Game::GameMode::EDITOR_TOP_DOWN) {

        RenderData renderData;
        RenderEditorTopDown(renderData);
    }
}

void RenderGame(RenderData& renderData) {

    if (BackEnd::GetAPI() == API::OPENGL) {
        OpenGLRenderer::RenderGame(renderData);
    }
    else if (BackEnd::GetAPI() == API::VULKAN) {
        VulkanRenderer::RenderGame(renderData);
    }
}

void RenderEditor3D(RenderData& renderData) {

    if (BackEnd::GetAPI() == API::OPENGL) {
        // TODO
    }
    else if (BackEnd::GetAPI() == API::VULKAN) {
        // TODO
    }
}

void RenderEditorTopDown(RenderData& renderData) {

    if (BackEnd::GetAPI() == API::OPENGL) {
        // TODO
    }
    else if (BackEnd::GetAPI() == API::VULKAN) {
        // TODO
    }
}

/*

███╗   ███╗██╗███████╗ ██████╗
████╗ ████║██║██╔════╝██╔════╝
██╔████╔██║██║███████╗██║
██║╚██╔╝██║██║╚════██║██║
██║ ╚═╝ ██║██║███████║╚██████╗
╚═╝     ╚═╝╚═╝╚══════╝ ╚═════╝ */

void Renderer::RenderLoadingScreen() {

    std::vector<RenderItem2D> renderItems = CreateLoadingScreenRenderItems();

    if (!BackEnd::WindowIsMinimized()) {
        if (BackEnd::GetAPI() == API::OPENGL) {
            OpenGLRenderer::RenderLoadingScreen(renderItems);
        }
        else if (BackEnd::GetAPI() == API::VULKAN) {
            VulkanRenderer::RenderLoadingScreen(renderItems);
        }
    }
    //PresentFinalImage();
}

void Renderer::HotloadShaders() {

    if (BackEnd::GetAPI() == API::OPENGL) {
        OpenGLRenderer::HotloadShaders();
    }
    else if (BackEnd::GetAPI() == API::VULKAN) {
        VulkanRenderer::HotloadShaders();
    }
}

/*
void ResizeRenderTargets() {

    SplitscreenMode splitscreenMode = Game::GetSplitscreenMode();

    int width = PRESENT_WIDTH;
    int height = PRESENT_HEIGHT;

    if (splitscreenMode == SplitscreenMode::TWO_PLAYER) {
        height *= 0.5f;
    }
    else if (splitscreenMode == SplitscreenMode::FOUR_PLAYER) {
        width *= 0.5f;
        height *= 0.5f;
    }

    if (BackEnd::GetAPI() == API::OPENGL) {
        //OpenGLRenderer::CreatePlayerRenderTargets(width, height);
    }
    else if (BackEnd::GetAPI() == API::VULKAN) {
        //VulkanRenderer::CreatePlayerRenderTargets(width, height);
    }
}*/

inline std::vector<DebugLineRenderMode> _allowedDebugLineRenderModes = {
    //PHYSX_ALL,
    //PHYSX_RAYCAST,
    //PHYSX_COLLISION,
    //RAYTRACE_LAND,
    //PHYSX_EDITOR,
    BOUNDING_BOXES,
    RTX_LAND_AABBS,
    PHYSX_COLLISION,
    //RTX_LAND_TRIS,
    //RTX_LAND_TOP_LEVEL_ACCELERATION_STRUCTURE,
    //RTX_LAND_BOTTOM_LEVEL_ACCELERATION_STRUCTURES,
    //RTX_LAND_TOP_AND_BOTTOM_LEVEL_ACCELERATION_STRUCTURES,
};

void Renderer::NextDebugLineRenderMode() {
    _debugLineRenderMode = (DebugLineRenderMode)(int(_debugLineRenderMode) + 1);
    if (_debugLineRenderMode == DEBUG_LINE_MODE_COUNT) {
        _debugLineRenderMode = (DebugLineRenderMode)0;
    }
    // If mode isn't in available modes list, then go to next
    bool allowed = false;
    for (auto& avaliableMode : _allowedDebugLineRenderModes) {
        if (_debugLineRenderMode == avaliableMode) {
            allowed = true;
            break;
        }
    }
    if (!allowed && _debugLineRenderMode != DebugLineRenderMode::SHOW_NO_LINES) {
        NextDebugLineRenderMode();
    }
}

inline static std::vector<RenderMode> _allowedRenderModes = {
    COMPOSITE,
    //DIRECT_LIGHT,
    //INDIRECT_LIGHT,
    POINT_CLOUD,
    PROPAGATION_GRID,
    POINT_CLOUD_PROPAGATION_GRID,
};

void Renderer::NextRenderMode() {
    _renderMode = (RenderMode)(int(_renderMode) + 1);
    if (_renderMode == DEBUG_LINE_MODE_COUNT) {
        _renderMode = (RenderMode)0;
    }
    // If mode isn't in available modes list, then go to next
    bool allowed = false;
    for (auto& avaliableMode : _allowedRenderModes) {
        if (_renderMode == avaliableMode) {
            allowed = true;
            break;
        }
    }
    if (!allowed && _renderMode != RenderMode::COMPOSITE) {
        NextRenderMode();
    }
}

void Renderer::PreviousRenderMode() {

    if (_renderMode == (RenderMode)(0)) {
        _renderMode = (RenderMode)((int)(RENDER_MODE_COUNT)-1);
    }
    else {
        _renderMode = (RenderMode)(int(_renderMode) - 1);
    }
    // If mode isn't in available modes list, then go to next
    bool allowed = false;
    for (auto& avaliableMode : _allowedRenderModes) {
        if (_renderMode == avaliableMode) {
            allowed = true;
            break;
        }
    }
    if (!allowed) {
        PreviousRenderMode();
    }
}


void PresentFinalImage() {

    if (BackEnd::GetAPI() == API::OPENGL) {
        OpenGLRenderer::PresentFinalImage();
    }
    else if (BackEnd::GetAPI() == API::VULKAN) {
        VulkanRenderer::PresentFinalImage();
    }
}

void Renderer::RecreateBlurBuffers() {

    if (BackEnd::GetAPI() == API::OPENGL) {
        OpenGLRenderer::RecreateBlurBuffers();
    }
    else if (BackEnd::GetAPI() == API::VULKAN) {
        VulkanRenderer::RecreateBlurBuffers();
    }
}