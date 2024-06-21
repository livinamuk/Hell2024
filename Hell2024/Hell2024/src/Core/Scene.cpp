#include "Scene.h"

#include <future>
#include <thread>

#include "../BackEnd/BackEnd.h"
#include "../Core/AssetManager.h"
#include "../Core/Game.h"
#include "../Renderer/Renderer_OLD.h"
#include "../Renderer/TextBlitter.h"
#include "../Util.hpp"
#include "../EngineState.hpp"

#include "Input.h"
#include "File.h"
#include "Player.h"
#include "Audio.hpp"



int _volumetricBloodObjectsSpawnedThisFrame = 0;

namespace Scene {

    std::vector<GameObject> g_gameObjects;
    std::vector<AnimatedGameObject> g_animatedGameObjects;
    std::vector<BulletHoleDecal> g_bulletHoleDecals;

    void CreateCeilingsHack();
    void EvaluateDebugKeyPresses();
    void ProcessBullets();
    void DestroyAllDecals();
    //void UpdateAnimatedGameObjects(float deltaTime);

    int testIndex = 0;
}

void Scene::Update(float deltaTime) {

    Game::SetPlayerGroundedStates();
    ProcessPhysicsCollisions(); // have you ever had a physics crash after you moved this before everything else???

    CheckForDirtyLights();

    if (Input::KeyPressed(HELL_KEY_N)) {
        Physics::ClearCollisionLists();
        for (GameObject& gameObject : g_gameObjects) {
            gameObject.LoadSavedState();
        }
        DestroyAllDecals();
        std::cout << "Loaded scene save state\n";
    }
    for (Door& door : _doors) {
        door.Update(deltaTime);
        door.UpdateRenderItems();
    }
    for (Window& window : _windows) {
        window.UpdateRenderItems();
    }
    ProcessBullets();

    UpdateGameObjects(deltaTime);
    //UpdateAnimatedGameObjects(deltaTime);

    for (BulletCasing& bulletCasing : _bulletCasings) {
        // TO DO: render item
        bulletCasing.Update(deltaTime);
    }
    for (Toilet& toilet : _toilets) {
        // TO DO: render item
        toilet.Update(deltaTime);
    }
    for (PickUp& pickUp : _pickUps) {
        // TO DO: render item
        pickUp.Update(deltaTime);
    }
    for (AnimatedGameObject& animatedGameObject : g_animatedGameObjects) {
        // TO DO: render item
        //animatedGameObject.Update(deltaTime);
    }

    // Update vat blood
    _volumetricBloodObjectsSpawnedThisFrame = 0;
    for (vector<VolumetricBloodSplatter>::iterator it = _volumetricBloodSplatters.begin(); it != _volumetricBloodSplatters.end();) {
         if (it->m_CurrentTime < 0.9f) {
            it->Update(deltaTime);
            ++it;
         }
         else {
             it = _volumetricBloodSplatters.erase(it);
         }
    }

    g_animatedGameObjects[testIndex].Update(deltaTime);

}



// Bullet hole decals

void Scene::CreateBulletDecal(glm::vec3 localPosition, glm::vec3 localNormal, PxRigidBody* parent, BulletHoleDecalType type) {
    g_bulletHoleDecals.emplace_back(BulletHoleDecal(localPosition, localNormal, parent, type));
}

const size_t Scene::GetBulletHoleDecalCount() {
    return g_bulletHoleDecals.size();
}

BulletHoleDecal* Scene::GetBulletHoleDecalByIndex(int32_t index) {
    if (index >= 0 && index < g_bulletHoleDecals.size()) {
        return &g_bulletHoleDecals[index];
    }
    else {
        std::cout << "Scene::GetBulletHoleDecalByIndex() called with out of range index " << index << ", size is " << GetBulletHoleDecalCount() << "\n";
        return nullptr;
    }
}

void Scene::DestroyAllDecals() {
    g_bulletHoleDecals.clear();
}

// Game Objects

int32_t Scene::CreateGameObject() {
    g_gameObjects.emplace_back();
    return (int32_t)g_gameObjects.size() - 1;
}

GameObject* Scene::GetGameObjectByIndex(int32_t index) {
    if (index >= 0 && index < g_gameObjects.size()) {
        return &g_gameObjects[index];
    }
    else {
        std::cout << "Scene::GetGameObjectByIndex() called with out of range index " << index << ", size is " << GetGameObjectCount() << "\n";
        return nullptr;
    }
}

GameObject* Scene::GetGameObjectByName(std::string name) {
    if (name != "undefined") {
        for (GameObject& gameObject : g_gameObjects) {
            if (gameObject.GetName() == name) {
                return &gameObject;
            }
        }
    }
    else {
        std::cout << "Scene::GetGameObjectByName() failed, no object with name \"" << name << "\"\n";
        return nullptr;
    }
}
const size_t Scene::GetGameObjectCount() {
    return g_gameObjects.size();
}

std::vector<GameObject>& Scene::GetGamesObjects() {
    return g_gameObjects;
}

void Scene::UpdateGameObjects(float deltaTime) {
    for (GameObject& gameObject : g_gameObjects) {
        gameObject.Update(deltaTime);
        gameObject.UpdateRenderItems();
    }
}

// Animated Game Objects

int32_t Scene::CreateAnimatedGameObject() {
    g_animatedGameObjects.emplace_back();
    return (int32_t)g_animatedGameObjects.size() - 1;
}

const size_t Scene::GetAnimatedGameObjectCount() {
    return g_animatedGameObjects.size();
}

AnimatedGameObject* Scene::GetAnimatedGameObjectByIndex(int32_t index) {
    if (index >= 0 && index < g_animatedGameObjects.size()) {
        return &g_animatedGameObjects[index];
    }
    else {
        std::cout << "Scene::GetAnimatedGameObjectByIndex() called with out of range index " << index << ", size is " << GetAnimatedGameObjectCount() << "\n";
        return nullptr;
    }
}

/*void Scene::UpdateAnimatedGameObjects(float deltaTime) {
    for (AnimatedGameObject& animatedGameObject : g_animatedGameObjects) {
        animatedGameObject.CreateSkinnedMeshRenderItems();
    }
}*/


std::vector<AnimatedGameObject>& Scene::GetAnimatedGamesObjects() {
    return g_animatedGameObjects;
}

std::vector<AnimatedGameObject*> Scene::GetAnimatedGamesObjectsToSkin() {
    std::vector<AnimatedGameObject*> objects;
    for (AnimatedGameObject& object : g_animatedGameObjects) {

        if (Game::GetSplitscreenMode() == SplitscreenMode::NONE &&
            object.GetFlag() == AnimatedGameObject::Flag::FIRST_PERSON_WEAPON &&
            object.GetPlayerIndex() != 0) {
            continue;
        }
        if (Game::GetSplitscreenMode() == SplitscreenMode::TWO_PLAYER &&
            object.GetFlag() == AnimatedGameObject::Flag::FIRST_PERSON_WEAPON &&
            object.GetPlayerIndex() > 1) {
            continue;
        }
        objects.push_back(&object);
    }
    return objects;
}

void Scene::LoadMapNEW(std::string mapPath) {

    CleanUp();
    File::LoadMap(mapPath);

    // Walls
    for (int i = 0; i < Scene::_walls.size(); i++) {
        Wall& wall = Scene::_walls[i];
        std::string name = "Wall" + std::to_string(i);
        wall.CreateVertexData();
        wall.meshIndex = AssetManager::CreateMesh(name, wall.vertices, wall.indices);
        wall.UpdateRenderItems();
        Mesh* mesh = AssetManager::GetMeshByIndex(wall.meshIndex);
    }

    // Floors
    for (int i = 0; i < Scene::_floors.size(); i++) {
        Floor& floor = Scene::_floors[i];
        std::string name = "Floor" + std::to_string(i);
        floor.CreateVertexData();
        floor.meshIndex = AssetManager::CreateMesh(name, floor.vertices, floor.indices);
        floor.UpdateRenderItem();
        Mesh* mesh = AssetManager::GetMeshByIndex(floor.meshIndex);
    }

    CreateCeilingsHack();

    // Floors
    for (int i = 0; i < Scene::_ceilings.size(); i++) {
        Ceiling& ceiling = Scene::_ceilings[i];
        std::string name = "Floor" + std::to_string(i);
        ceiling.CreateVertexData();
        ceiling.meshIndex = AssetManager::CreateMesh(name, ceiling.vertices, ceiling.indices);
        ceiling.UpdateRenderItem();
        Mesh* mesh = AssetManager::GetMeshByIndex(ceiling.meshIndex);
    }

    LoadHardCodedObjects();
    RecreateAllPhysicsObjects();
    ResetGameObjectStates();
}

// Hack To Create Ceilings From Floors
void Scene::CreateCeilingsHack() {
    Scene::_ceilings.emplace_back(0.1f, 0.1f, 5.2f, 3.1f, 2.5f, AssetManager::GetMaterialIndex("Ceiling2"));
    Scene::_ceilings.emplace_back(0.1f, 4.1f, 6.1f, 6.9f, 2.5f, AssetManager::GetMaterialIndex("Ceiling2"));
    Scene::_ceilings.emplace_back(0.1f, 3.1f, 3.7f, 4.1f, 2.5f, AssetManager::GetMaterialIndex("Ceiling2"));
    Scene::_ceilings.emplace_back(4.7f, 3.1f, 6.1f, 4.1f, 2.5f, AssetManager::GetMaterialIndex("Ceiling2"));
    int count = 0;
    for (Floor& floor : Scene::_floors) {
        count++;
        if (count == 2) {
            continue;
        }
        float minX = std::min(std::min(std::min(floor.v1.position.x, floor.v2.position.x), floor.v3.position.x), floor.v4.position.x);
        float maxX = std::max(std::max(std::max(floor.v1.position.x, floor.v2.position.x), floor.v3.position.x), floor.v4.position.x);
        float minZ = std::min(std::min(std::min(floor.v1.position.z, floor.v2.position.z), floor.v3.position.z), floor.v4.position.z);
        float maxZ = std::max(std::max(std::max(floor.v1.position.z, floor.v2.position.z), floor.v3.position.z), floor.v4.position.z);
        Scene::_ceilings.emplace_back(minX, minZ, maxX, maxZ, 2.5f, AssetManager::GetMaterialIndex("Ceiling2"));
    }
}


std::vector<RenderItem3D> Scene::GetAllRenderItems() {

    std::vector<RenderItem3D> renderItems;

    for (Floor& floor : Scene::_floors) {
        renderItems.push_back(floor.GetRenderItem());
    }
    for (Ceiling& ceiling : Scene::_ceilings) {
        renderItems.push_back(ceiling.GetRenderItem());
    }
    for (Wall& wall : Scene::_walls) {
        renderItems.reserve(renderItems.size() + wall.GetRenderItems().size());
        renderItems.insert(std::end(renderItems), std::begin(wall.GetRenderItems()), std::end(wall.GetRenderItems()));
    }
    for (GameObject& gameObject : Scene::g_gameObjects) {
        renderItems.reserve(renderItems.size() + gameObject.GetRenderItems().size());
        renderItems.insert(std::end(renderItems), std::begin(gameObject.GetRenderItems()), std::end(gameObject.GetRenderItems()));
    }
    for (Door& door : Scene::_doors) {
        renderItems.reserve(renderItems.size() + door.GetRenderItems().size());
        renderItems.insert(std::end(renderItems), std::begin(door.GetRenderItems()), std::end(door.GetRenderItems()));
    }
    for (Window& window : Scene::_windows) {
        renderItems.reserve(renderItems.size() + window.GetRenderItems().size());
        renderItems.insert(std::end(renderItems), std::begin(window.GetRenderItems()), std::end(window.GetRenderItems()));
    }


    // Casings
    static Material* glockCasingMaterial = AssetManager::GetMaterialByIndex(AssetManager::GetMaterialIndex("Casing9mm"));
    static Material* shotgunShellMaterial = AssetManager::GetMaterialByIndex(AssetManager::GetMaterialIndex("Shell"));
    static Material* aks74uCasingMaterial = AssetManager::GetMaterialByIndex(AssetManager::GetMaterialIndex("Casing_AkS74U"));
    static int glockCasingMeshIndex = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Casing9mm"))->GetMeshIndices()[0];
    static int shotgunShellMeshIndex = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Shell"))->GetMeshIndices()[0];
    static int aks74uCasingMeshIndex = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("CasingAKS74U"))->GetMeshIndices()[0];
    for (BulletCasing& casing : Scene::_bulletCasings) {
        RenderItem3D& renderItem = renderItems.emplace_back();
        renderItem.modelMatrix = casing.modelMatrix;
        renderItem.inverseModelMatrix = inverse(renderItem.modelMatrix);
        renderItem.castShadow = false;
        if (casing.type == GLOCK) {
            renderItem.baseColorTextureIndex = glockCasingMaterial->_basecolor;
            renderItem.rmaTextureIndex = glockCasingMaterial->_rma;
            renderItem.normalTextureIndex = glockCasingMaterial->_normal;
            renderItem.meshIndex = glockCasingMeshIndex;
        }
        if (casing.type == SHOTGUN) {
            renderItem.baseColorTextureIndex = shotgunShellMaterial->_basecolor;
            renderItem.rmaTextureIndex = shotgunShellMaterial->_rma;
            renderItem.normalTextureIndex = shotgunShellMaterial->_normal;
            renderItem.meshIndex = shotgunShellMeshIndex;
        }
        if (casing.type == AKS74U) {
            renderItem.baseColorTextureIndex = aks74uCasingMaterial->_basecolor;
            renderItem.rmaTextureIndex = aks74uCasingMaterial->_rma;
            renderItem.normalTextureIndex = aks74uCasingMaterial->_normal;
            renderItem.meshIndex = aks74uCasingMeshIndex;
        }
    }

    // Light bulbs


    for (Light& light : Scene::_lights) {


        Transform transform;
        transform.position = light.position;
        glm::mat4 lightBulbWorldMatrix = transform.to_mat4();

        static Material* light0Material = AssetManager::GetMaterialByIndex(AssetManager::GetMaterialIndex("Light"));
        static Material* wallMountedLightMaterial = AssetManager::GetMaterialByIndex(AssetManager::GetMaterialIndex("LightWall"));

        static int lightBulb0MeshIndex = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Light0_Bulb"))->GetMeshIndices()[0];
        static int lightMount0MeshIndex = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Light0_Mount"))->GetMeshIndices()[0];
        static int lightCord0MeshIndex = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Light0_Cord"))->GetMeshIndices()[0];
        static Model* wallMountedLightmodel = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("LightWallMounted"));

     //   std::cout << "MESH COUNT: " << AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("LightWallMounted"))->GetMeshIndices().size() << "\n";

        if (light.type == 0) {
            RenderItem3D& renderItem = renderItems.emplace_back();
            renderItem.meshIndex = lightBulb0MeshIndex;
            renderItem.modelMatrix = lightBulbWorldMatrix;
            renderItem.inverseModelMatrix = inverse(renderItem.modelMatrix);
            renderItem.baseColorTextureIndex = light0Material->_basecolor;
            renderItem.rmaTextureIndex = light0Material->_rma;
            renderItem.normalTextureIndex = light0Material->_normal;
            renderItem.castShadow = false;
            renderItem.useEmissiveMask = 1.0f;
            renderItem.emissiveColor = light.color;


            // Find mount position and draw it if the ray hits the ceiling
            PxU32 raycastFlags = RaycastGroup::RAYCAST_ENABLED;
            PhysXRayResult rayResult = Util::CastPhysXRay(light.position, glm::vec3(0, 1, 0), 2, raycastFlags);

            if (rayResult.hitFound) {

                Transform mountTransform;
                mountTransform.position = rayResult.hitPosition;

                RenderItem3D& renderItemMount = renderItems.emplace_back();
                renderItemMount.meshIndex = lightMount0MeshIndex;
                renderItemMount.modelMatrix = mountTransform.to_mat4();
                renderItemMount.inverseModelMatrix = inverse(renderItem.modelMatrix);
                renderItemMount.baseColorTextureIndex = light0Material->_basecolor;
                renderItemMount.rmaTextureIndex = light0Material->_rma;
                renderItemMount.normalTextureIndex = light0Material->_normal;
                renderItemMount.castShadow = false;

                Transform cordTransform;
                cordTransform.position = light.position;
                cordTransform.scale.y = abs(rayResult.hitPosition.y - light.position.y);

                RenderItem3D& renderItemCord = renderItems.emplace_back();
                renderItemCord.meshIndex = lightCord0MeshIndex;
                renderItemCord.modelMatrix = cordTransform.to_mat4();
                renderItemCord.inverseModelMatrix = inverse(renderItem.modelMatrix);
                renderItemCord.baseColorTextureIndex = light0Material->_basecolor;
                renderItemCord.rmaTextureIndex = light0Material->_rma;
                renderItemCord.normalTextureIndex = light0Material->_normal;
                renderItemCord.castShadow = false;

            }

        }
        else if (light.type == 1) {

            for (int j = 0; j < wallMountedLightmodel->GetMeshCount(); j++) {
                RenderItem3D& renderItem = renderItems.emplace_back();
                renderItem.meshIndex = wallMountedLightmodel->GetMeshIndices()[j];
                renderItem.modelMatrix = lightBulbWorldMatrix;
                renderItem.inverseModelMatrix = inverse(renderItem.modelMatrix);
                renderItem.baseColorTextureIndex = wallMountedLightMaterial->_basecolor;
                renderItem.rmaTextureIndex = wallMountedLightMaterial->_rma;
                renderItem.normalTextureIndex = wallMountedLightMaterial->_normal;
                renderItem.castShadow = false;
                renderItem.useEmissiveMask = 1.0f;
                renderItem.emissiveColor = light.color;
            }
        }

        /*
        Transform transform;
        transform.position = light.position;
        shader.SetVec3("lightColor", light.color);
        shader.SetMat4("model", transform.to_mat4());

        if (light.type == 0) {
            AssetManager::BindMaterialByIndex(AssetManager::GetMaterialIndex("Light"));

            DrawModel(AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Light0_Bulb")));

            // Find mount position
            PxU32 raycastFlags = RaycastGroup::RAYCAST_ENABLED;
            PhysXRayResult rayResult = Util::CastPhysXRay(light.position, glm::vec3(0, 1, 0), 2, raycastFlags);
            if (rayResult.hitFound) {
                Transform mountTransform;
                mountTransform.position = rayResult.hitPosition;
                shader.SetMat4("model", mountTransform.to_mat4());

                DrawModel(AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Light0_Mount")));

                // Stretch the cord
                Transform cordTransform;
                cordTransform.position = light.position;
                cordTransform.scale.y = abs(rayResult.hitPosition.y - light.position.y);
                shader.SetMat4("model", cordTransform.to_mat4());
                DrawModel(AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Light0_Cord")));

            }
        }
        else if (light.type == 1) {
            AssetManager::BindMaterialByIndex(AssetManager::GetMaterialIndex("LightWall"));
            DrawModel(AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("LightWallMounted")));
        }*/
    }



    return renderItems;
}


std::vector<RenderItem3D> Scene::CreateDecalRenderItems() {

    static Material* bulletHolePlasterMaterial = AssetManager::GetMaterialByIndex(AssetManager::GetMaterialIndex("BulletHole_Plaster"));
    static Material* bulletHoleGlassMaterial = AssetManager::GetMaterialByIndex(AssetManager::GetMaterialIndex("BulletHole_Glass"));

    std::vector<RenderItem3D> renderItems;
    renderItems.reserve(g_bulletHoleDecals.size());

    // Wall bullet decals
    for (BulletHoleDecal& decal : g_bulletHoleDecals) {
        if (decal.GetType() == BulletHoleDecalType::REGULAR) {
            RenderItem3D& renderItem = renderItems.emplace_back();
            renderItem.modelMatrix = decal.GetModelMatrix();
            renderItem.inverseModelMatrix = inverse(renderItem.modelMatrix);
            renderItem.baseColorTextureIndex = bulletHolePlasterMaterial->_basecolor;
            renderItem.rmaTextureIndex = bulletHolePlasterMaterial->_rma;
            renderItem.normalTextureIndex = bulletHolePlasterMaterial->_normal;
            renderItem.meshIndex = AssetManager::GetQuadMeshIndex();
        }
    }

    // Glass bullet decals
    for (BulletHoleDecal& decal : g_bulletHoleDecals) {
        if (decal.GetType() == BulletHoleDecalType::GLASS) {
            RenderItem3D& renderItem = renderItems.emplace_back();
            renderItem.modelMatrix = decal.GetModelMatrix();
            renderItem.inverseModelMatrix = inverse(renderItem.modelMatrix);
            renderItem.baseColorTextureIndex = bulletHoleGlassMaterial->_basecolor;
            renderItem.rmaTextureIndex = bulletHoleGlassMaterial->_rma;
            renderItem.normalTextureIndex = bulletHoleGlassMaterial->_normal;
            renderItem.meshIndex = AssetManager::GetQuadMeshIndex();
        }
    }

    //int baseVertex = AssetManager::GetMeshByIndex(AssetManager::GetQuadMeshIndex())->baseVertex;
    //int vertexCount = AssetManager::GetMeshByIndex(AssetManager::GetQuadMeshIndex())->indexCount;
    //std::cout << "base vertex in real life:" << baseVertex << "\n";
    //std::cout << "index count in real life:" << vertexCount << "\n";

    return renderItems;
}

void Scene::EvaluateDebugKeyPresses() {


    // Set spawn point
    /*if (Input::KeyPressed(HELL_KEY_K) && false) {
        SpawnPoint spawnPoint;
        spawnPoint.position = Scene::Game::_players[0].GetFeetPosition();
        spawnPoint.rotation = Scene::Game::_players[0].GetCameraRotation();
        _spawnPoints.push_back(spawnPoint);
        std::cout << "Position: " << Util::Vec3ToString(spawnPoint.position) << "\n";
        std::cout << "Rotation: " << Util::Vec3ToString(spawnPoint.rotation) << "\n";
    }*/
}


















float door2X = 2.05f;

void Scene::CreateVolumetricBlood(glm::vec3 position, glm::vec3 rotation, glm::vec3 front) {
    // Spawn a max of 4 per frame
    if (_volumetricBloodObjectsSpawnedThisFrame < 4)
        _volumetricBloodSplatters.push_back(VolumetricBloodSplatter(position, rotation, front));
    _volumetricBloodObjectsSpawnedThisFrame++;
}

void Scene::Update_OLD(float deltaTime) {

    EvaluateDebugKeyPresses();


    // OLD SHIT BELOW

    _volumetricBloodObjectsSpawnedThisFrame = 0;

    for (VolumetricBloodSplatter& volumetricBloodSplatter : _volumetricBloodSplatters) {
        volumetricBloodSplatter.Update(deltaTime);
    }

    for (vector<VolumetricBloodSplatter>::iterator it = _volumetricBloodSplatters.begin(); it != _volumetricBloodSplatters.end();) {
        if (it->m_CurrentTime > 0.9f)
            it = _volumetricBloodSplatters.erase(it);
        else
            ++it;
    }






    static int i = 0;
    i++;

    // Move light source 3 to the lamp
    GameObject* lamp = GetGameObjectByName("Lamp");
    if (lamp && i > 2) {
        glm::mat4 lampMatrix = lamp->GetModelMatrix();
        Transform globeTransform;
        globeTransform.position = glm::vec3(0, 0.45f, 0);
        glm::mat4 worldSpaceMatrix = lampMatrix * globeTransform.to_mat4();
        Scene::_lights[3].position = Util::GetTranslationFromMatrix(worldSpaceMatrix);
    }
   // Scene::_lights[3].isDirty = true;


    GameObject* ak = GetGameObjectByName("AKS74U_Carlos");
   // ak->_transform.rotation = glm::vec3(0, 0, 0);
    if (Input::KeyPressed(HELL_KEY_SPACE) && false) {
        std::cout << "updating game object: " << ak->_name << "\n";
        //ak->_editorRaycastBody->setGlobalPose(PxTransform(Util::GlmMat4ToPxMat44(ak->GetModelMatrix())));
        Transform transform = ak->_transform;
		PxQuat quat = Util::GlmQuatToPxQuat(glm::quat(transform.rotation));
		PxTransform trans = PxTransform(PxVec3(transform.position.x, transform.position.y, transform.position.z), quat);
		//ak->_editorRaycastBody->setGlobalPose(PxTransform(Util::GlmMat4ToPxMat44(trans.to_mat4())));
		//ak->_editorRaycastBody->setGlobalPose(trans);
    }

    Game::SetPlayerGroundedStates();
    ProcessBullets();

    for (BulletCasing& bulletCasing : _bulletCasings) {
        bulletCasing.Update(deltaTime);
    }

    for (Toilet& toilet : _toilets) {
        toilet.Update(deltaTime);
    }

	for (PickUp& pickUp : _pickUps) {
        pickUp.Update(deltaTime);
	}

    if (Input::KeyPressed(HELL_KEY_T)) {
        for (Light& light : Scene::_lights) {
            light.isDirty = true;
        }
    }

       /*
    for (AnimatedGameObject& animatedGameObject : g_animatedGameObjects) {
        animatedGameObject.Update(deltaTime);
    }*/

    for (GameObject& gameObject : g_gameObjects) {
        gameObject.Update(deltaTime);
    }

    for (Door& door : _doors) {
        door.Update(deltaTime);
    }

    UpdateRTInstanceData();
    ProcessPhysicsCollisions();
}

void Scene::CheckForDirtyLights() {
    for (Light& light : Scene::_lights) {
        light.isDirty = false;
        for (GameObject& gameObject : Scene::g_gameObjects) {
            if (gameObject.HasMovedSinceLastFrame()) {
                if (Util::AABBInSphere(gameObject._aabb, light.position, light.radius)) {
                    light.isDirty = true;
                    break;
                }
                //std::cout << gameObject.GetName() << " has moved apparently\n";
            }
        }

        if (!light.isDirty) {
            for (Door& door : Scene::_doors) {
                if (door.HasMovedSinceLastFrame()) {
                    if (Util::AABBInSphere(door._aabb, light.position, light.radius)) {
                        light.isDirty = true;
                        break;
                    }
                }
            }
        }   /*
        if (!light.isDirty) {
            for (Toilet& toilet : Scene::_toilets) {
                if (toilet.lid.HasMovedSinceLastFrame()) {
                    if (Util::AABBInSphere(toilet.lid._aabb, light.position, light.radius)) {
                        light.isDirty = true;
                        break;
                    }
                }
                else if (toilet.seat.HasMovedSinceLastFrame()) {
                    if (Util::AABBInSphere(toilet.seat._aabb, light.position, light.radius)) {
                        light.isDirty = true;
                        break;
                    }
                }
            }
        }*/
    }
}



void Scene::ProcessBullets() {

    bool fleshWasHit = false;
    bool glassWasHit = false;
	for (int i = 0; i < Scene::_bullets.size(); i++) {
		Bullet& bullet = Scene::_bullets[i];
        PxU32 raycastFlags = bullet.raycastFlags;// RaycastGroup::RAYCAST_ENABLED;

		PhysXRayResult rayResult = Util::CastPhysXRay(bullet.spawnPosition, bullet.direction, 1000, raycastFlags);
		if (rayResult.hitFound) {
			PxRigidDynamic* actor = (PxRigidDynamic*)rayResult.hitActor;
			if (actor->userData) {


				PhysicsObjectData* physicsObjectData = (PhysicsObjectData*)actor->userData;

                // A ragdoll was hit
                if (physicsObjectData->type == RAGDOLL_RIGID) {
                    if (actor->userData) {


                        // Spawn volumetric blood
                        glm::vec3 position = rayResult.hitPosition;
                        glm::vec3 rotation = glm::vec3(0, 0, 0);
                        glm::vec3 front = bullet.direction * glm::vec3(-1);
                        Scene::CreateVolumetricBlood(position, rotation, -bullet.direction);

                        // Spawn blood decal
                        static int counter = 0;
                        int type = counter;
                        Transform transform;
                        transform.position.x = rayResult.hitPosition.x;
                        transform.position.y = 0.101f;
                        transform.position.z = rayResult.hitPosition.z;
                        transform.rotation.y = bullet.parentPlayersViewRotation.y + HELL_PI;

                        static int typeCounter = 0;
                        Scene::_bloodDecals.push_back(BloodDecal(transform, typeCounter));
                        BloodDecal* decal = &Scene::_bloodDecals.back();

                        typeCounter++;
                        if (typeCounter == 4) {
                            typeCounter = 0;
                        }
                        /*
                        s_remainingBloodDecalsAllowedThisFrame--;
                        counter++;
                        if (counter > 3)
                            counter = 0;
                            */





                        Player* parentPlayerHit = (Player*)physicsObjectData->parent;
                        if (!parentPlayerHit->_isDead) {






                            parentPlayerHit->GiveDamageColor();
                            parentPlayerHit->_health -= 15;
                            parentPlayerHit->_health = std::max(0, parentPlayerHit->_health);

                            if (actor->getName() == "RAGDOLL_HEAD") {
                                parentPlayerHit->_health = 0;
                            }
                            else if (actor->getName() == "RAGDOLL_NECK") {
                                parentPlayerHit->_health = 0;
                            }
                            else {
                                fleshWasHit = true;
                            }

                            // Did it kill them?
                            if (parentPlayerHit->_health == 0) {

                                parentPlayerHit->Kill();
                                if (parentPlayerHit != Game::GetPlayerByIndex(0)) {
                                    Game::GetPlayerByIndex(0)->_killCount++;
                                }
                                if (parentPlayerHit != Game::GetPlayerByIndex(1)) {
                                    Game::GetPlayerByIndex(1)->_killCount++;
                                }

                                AnimatedGameObject* hitCharacterModel = GetAnimatedGameObjectByIndex(parentPlayerHit->GetCharacterModelAnimatedGameObjectIndex());

                                for (RigidComponent& rigidComponent : hitCharacterModel->_ragdoll._rigidComponents) {
                                    float strength = 75;
                                    if (bullet.type == SHOTGUN) {
                                        strength = 20;
                                    }
                                    strength *= rigidComponent.mass * 1.5f;
                                    PxVec3 force = PxVec3(bullet.direction.x, bullet.direction.y, bullet.direction.z) * strength;
                                    rigidComponent.pxRigidBody->addForce(force);
                                }
                            }
                        }
                    }





                    float strength = 75;
                    if (bullet.type == SHOTGUN) {
                        strength = 20;
                    }
                    strength *= 20000;
                   // std::cout << "you shot a ragdoll rigid\n";
                    PxVec3 force = PxVec3(bullet.direction.x, bullet.direction.y, bullet.direction.z) * strength;
                    actor->addForce(force);
                }


                if (physicsObjectData->type == GAME_OBJECT) {
					GameObject* gameObject = (GameObject*)physicsObjectData->parent;
                    float force = 75;
                    if (bullet.type == SHOTGUN) {
                        force = 20;
                        //std::cout << "spawned a shotgun bullet\n";
                    }
					gameObject->AddForceToCollisionObject(bullet.direction, force);
				}




				if (physicsObjectData->type == GLASS) {
                    glassWasHit = true;
					//std::cout << "you shot glass\n";
					Bullet newBullet;
					newBullet.direction = bullet.direction;
					newBullet.spawnPosition = rayResult.hitPosition + (bullet.direction * glm::vec3(0.5f));
                    newBullet.raycastFlags = bullet.raycastFlags;
                    Scene::_bullets.push_back(newBullet);

					// Front glass bullet decal
					PxRigidBody* parent = actor;
					glm::mat4 parentMatrix = Util::PxMat44ToGlmMat4(actor->getGlobalPose());
					glm::vec3 localPosition = glm::inverse(parentMatrix) * glm::vec4(rayResult.hitPosition + (rayResult.surfaceNormal * glm::vec3(0.001)), 1.0);
					glm::vec3 localNormal = glm::inverse(parentMatrix) * glm::vec4(rayResult.surfaceNormal, 0.0);
                    Scene::CreateBulletDecal(localPosition, localNormal, parent, BulletHoleDecalType::GLASS);

					// Back glass bullet decal
					localNormal = glm::inverse(parentMatrix) * glm::vec4(rayResult.surfaceNormal * glm::vec3(-1) - (rayResult.surfaceNormal * glm::vec3(0.001)), 0.0);
                    Scene::CreateBulletDecal(localPosition, localNormal, parent, BulletHoleDecalType::GLASS);

					// Glass projectile
					for (int i = 0; i < 2; i++) {
						Transform transform;
						if (i == 1) {
							transform.position = rayResult.hitPosition + (rayResult.surfaceNormal * glm::vec3(-0.13));
						}
						else {
							transform.position = rayResult.hitPosition + (rayResult.surfaceNormal * glm::vec3(0.03));
						}
					}
				}
				else if (physicsObjectData->type != RAGDOLL_RIGID) {
                    bool doIt = true;
                    if (physicsObjectData->type == GAME_OBJECT) {
                        GameObject* gameObject = (GameObject*)physicsObjectData->parent;
                        if (gameObject->GetPickUpType() != PickUpType::NONE) {
                            doIt = false;
                        }
                    }
                    if (doIt) {
                        // Bullet decal
                        PxRigidBody* parent = actor;
                        glm::mat4 parentMatrix = Util::PxMat44ToGlmMat4(actor->getGlobalPose());
                        glm::vec3 localPosition = glm::inverse(parentMatrix) * glm::vec4(rayResult.hitPosition + (rayResult.surfaceNormal * glm::vec3(0.001)), 1.0);
                        glm::vec3 localNormal = glm::inverse(parentMatrix) * glm::vec4(rayResult.surfaceNormal, 0.0);
                        Scene::CreateBulletDecal(localPosition, localNormal, parent, BulletHoleDecalType::REGULAR);
                    }
				}
			}
		}
	}
    Scene::_bullets.clear();

    if (glassWasHit) {
        Audio::PlayAudio("GlassImpact.wav", 3.0f);
    }
    if (fleshWasHit) {
        int random_number = std::rand() % 8 + 1;
        std::string file = "FLY_Bullet_Impact_Flesh_0" + std::to_string(random_number) + ".wav";
        Audio::PlayAudio(file.c_str(), 0.9f);
    }
}
void Scene::LoadHardCodedObjects() {

    _volumetricBloodSplatters.clear();

    _toilets.push_back(Toilet(glm::vec3(8.3, 0.1, 2.8), 0));
    _toilets.push_back(Toilet(glm::vec3(11.2f, 0.1f, 3.65f), HELL_PI * 0.5f));

    if (true) {

        CreateGameObject();
        GameObject* aks74u = GetGameObjectByIndex(GetGameObjectCount()-1);
        aks74u->SetPosition(1.8f, 1.7f, 0.75f);
        aks74u->SetRotationX(-1.7f);
        aks74u->SetRotationY(0.0f);
        aks74u->SetRotationZ(-1.6f);
        aks74u->SetModel("AKS74U_Carlos");
        aks74u->SetName("AKS74U_Carlos");
        aks74u->SetMeshMaterial("Ceiling");
        aks74u->SetMeshMaterialByMeshName("FrontSight_low", "AKS74U_0");
        aks74u->SetMeshMaterialByMeshName("Receiver_low", "AKS74U_1");
        aks74u->SetMeshMaterialByMeshName("BoltCarrier_low", "AKS74U_1");
        aks74u->SetMeshMaterialByMeshName("SafetySwitch_low", "AKS74U_1");
        aks74u->SetMeshMaterialByMeshName("Pistol_low", "AKS74U_2");
        aks74u->SetMeshMaterialByMeshName("Trigger_low", "AKS74U_2");
        aks74u->SetMeshMaterialByMeshName("MagRelease_low", "AKS74U_2");
        aks74u->SetMeshMaterialByMeshName("Magazine_Housing_low", "AKS74U_3");
        aks74u->SetMeshMaterialByMeshName("BarrelTip_low", "AKS74U_4");
        aks74u->SetPickUpType(PickUpType::AKS74U);
        aks74u->SetWakeOnStart(true);
        aks74u->SetCollisionType(CollisionType::PICKUP);
        aks74u->SetKinematic(false);
        aks74u->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("AKS74U_Carlos_ConvexMesh"));
        aks74u->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("AKS74U_Carlos"));
        aks74u->SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
        aks74u->UpdateRigidBodyMassAndInertia(50.0f);



        CreateGameObject();
        GameObject* shotgunPickup = GetGameObjectByIndex(GetGameObjectCount() - 1);
        shotgunPickup->SetPosition(11.07, 0.65f, 4.025f);
        shotgunPickup->SetRotationX(1.5916);
        shotgunPickup->SetRotationY(3.4f);
        shotgunPickup->SetRotationZ(-0.22);
        shotgunPickup->SetModel("Shotgun_Isolated");
        shotgunPickup->SetName("Shotgun_Pickup");
        shotgunPickup->SetMeshMaterial("Shotgun");
        shotgunPickup->SetPickUpType(PickUpType::SHOTGUN);
        shotgunPickup->SetKinematic(false);
        shotgunPickup->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("Shotgun_Isolated_ConvexMesh"));
        shotgunPickup->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("Shotgun_Isolated"));
        shotgunPickup->SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
        shotgunPickup->UpdateRigidBodyMassAndInertia(75.0f);
        shotgunPickup->PutRigidBodyToSleep();
        shotgunPickup->SetCollisionType(CollisionType::PICKUP);


        CreateGameObject();
        GameObject* scopePickUp = GetGameObjectByIndex(GetGameObjectCount() - 1);
        scopePickUp->SetPosition(9.07f, 0.979f, 7.96f);
        scopePickUp->SetModel("ScopePickUp");
        scopePickUp->SetName("ScopePickUp");
        scopePickUp->SetMeshMaterial("Shotgun");
        scopePickUp->SetPickUpType(PickUpType::AKS74U_SCOPE);
        scopePickUp->SetKinematic(false);
        scopePickUp->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("ScopePickUp_ConvexMesh"));
        scopePickUp->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("ScopePickUp"));
        scopePickUp->SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
        scopePickUp->UpdateRigidBodyMassAndInertia(750.0f);
        scopePickUp->PutRigidBodyToSleep();
        scopePickUp->SetCollisionType(CollisionType::PICKUP);



        /*

        CreateGameObject();
        GameObject* tokarevPickup = GetGameObjectByIndex(GetGameObjectCount() - 1);
        tokarevPickup->SetPosition(2,1.6,3);
        tokarevPickup->SetModel("Tokarev");
        tokarevPickup->SetName("Tokarev");
        tokarevPickup->SetMeshMaterial("Tokarev", 0);
        tokarevPickup->SetMeshMaterial("Gold", 1);
        tokarevPickup->SetMeshMaterial("Ceiling", 2);
        tokarevPickup->SetScale(3);

        std::cout << "\n";
        std::cout << "\n";
    std::cout << "MESH COUNT: " << tokarevPickup->model->GetMeshCount() << "\n";

    std::cout << "\n";
    std::cout << "\n";*/





        // tokarevPickup->SetKinematic(false);
        // tokarevPickup->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("ScopePickUp_ConvexMesh"));
        // tokarevPickup->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("ScopePickUp"));
        // tokarevPickup->SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
        // tokarevPickup->UpdateRigidBodyMassAndInertia(750.0f);
        // tokarevPickup->PutRigidBodyToSleep();
        // tokarevPickup->SetCollisionType(CollisionType::PICKUP);


   /*   CreateGameObject();
        GameObject* glockAmmo = GetGameObjectByIndex(GetGameObjectCount() - 1);
        glockAmmo->SetPosition(0.40f, 0.78f, 4.45f);
        glockAmmo->SetRotationY(HELL_PI * 0.4f);
        glockAmmo->SetModel("GlockAmmoBox");
        glockAmmo->SetName("GlockAmmo_PickUp");
        glockAmmo->SetMeshMaterial("GlockAmmoBox");
        glockAmmo->SetPickUpType(PickUpType::GLOCK_AMMO);
        glockAmmo->SetKinematic(false);
        glockAmmo->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("GlockAmmoBox_ConvexMesh"));
        glockAmmo->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("GlockAmmoBox_ConvexMesh"));
        glockAmmo->SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
        glockAmmo->UpdateRigidBodyMassAndInertia(150.0f);
        glockAmmo->PutRigidBodyToSleep();
        glockAmmo->SetCollisionType(CollisionType::PICKUP);*/

        Game::SpawnPickup(PickUpType::GLOCK_AMMO, glm::vec3(0.40f, 0.78f, 4.45f), glm::vec3(0, HELL_PI * 0.4f, 0), true);

        for (int x = 0; x < 10; x++) {
            for (int z = 0; z < 10; z++) {

                glm::vec3 position = glm::vec3(1 + x * 0.4f, 0.5f, 1 + z * 0.4f);
                glm::vec3 rotation = glm::vec3(0, Util::RandomFloat(0, HELL_PI * 2), 0);


                int random_number = std::rand() % 2;
                if (random_number == 0) {
                    Game::SpawnAmmo("Glock", position, rotation, true);
                }
                else {
                    Game::SpawnAmmo("Tokarev", position, rotation, true);
                }
            }
        }


        CreateGameObject();
        GameObject* pictureFrame = GetGameObjectByIndex(GetGameObjectCount() - 1);
        pictureFrame->SetPosition(0.1f, 1.5f, 2.5f);
        pictureFrame->SetScale(0.01f);
        pictureFrame->SetRotationY(HELL_PI / 2);
		pictureFrame->SetModel("PictureFrame_1");
		pictureFrame->SetMeshMaterial("LongFrame");
		pictureFrame->SetName("PictureFrame");

        float cushionHeight = 0.555f;
        Transform shapeOffset;
        shapeOffset.position.y = cushionHeight * 0.5f;
        shapeOffset.position.z = 0.5f;
        PxShape* sofaShapeBigCube = Physics::CreateBoxShape(1, cushionHeight * 0.5f, 0.4f, shapeOffset);
        PhysicsFilterData sofaFilterData;
        sofaFilterData.raycastGroup = RAYCAST_DISABLED;
        sofaFilterData.collisionGroup = CollisionGroup::ENVIROMENT_OBSTACLE;
        sofaFilterData.collidesWith = (CollisionGroup)(GENERIC_BOUNCEABLE | BULLET_CASING | PLAYER | RAGDOLL);

        CreateGameObject();
        GameObject* sofa = GetGameObjectByIndex(GetGameObjectCount() - 1);
        sofa->SetPosition(2.0f, 0.1f, 0.1f);
        sofa->SetName("Sofa");
        sofa->SetModel("Sofa_Cushionless");
        sofa->SetMeshMaterial("Sofa");
        sofa->SetKinematic(true);
        sofa->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("Sofa_Cushionless"));
        sofa->AddCollisionShape(sofaShapeBigCube, sofaFilterData);
        sofa->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaBack_ConvexMesh"));
        sofa->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaLeftArm_ConvexMesh"));
        sofa->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaRightArm_ConvexMesh"));
        sofa->SetModelMatrixMode(ModelMatrixMode::GAME_TRANSFORM);
        sofa->SetCollisionType(CollisionType::STATIC_ENVIROMENT);

        PhysicsFilterData cushionFilterData;
        cushionFilterData.raycastGroup = RAYCAST_DISABLED;
        cushionFilterData.collisionGroup = CollisionGroup::GENERIC_BOUNCEABLE;
        cushionFilterData.collidesWith = CollisionGroup(ENVIROMENT_OBSTACLE | GENERIC_BOUNCEABLE);
        float cushionDensity = 20.0f;

        CreateGameObject();
        GameObject* cushion0 = GetGameObjectByIndex(GetGameObjectCount() - 1);
        cushion0->SetPosition(2.0f, 0.1f, 0.1f);
        cushion0->SetModel("SofaCushion0");
        cushion0->SetMeshMaterial("Sofa");
        cushion0->SetName("SofaCushion0");
        cushion0->SetKinematic(false);
        cushion0->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaCushion0"));
        cushion0->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaCushion0_ConvexMesh"));
        cushion0->SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
        cushion0->UpdateRigidBodyMassAndInertia(cushionDensity);
        cushion0->SetCollisionType(CollisionType::BOUNCEABLE);

        CreateGameObject();
        GameObject* cushion1 = GetGameObjectByIndex(GetGameObjectCount() - 1);
        cushion1->SetPosition(2.0f, 0.1f, 0.1f);
        cushion1->SetModel("SofaCushion1");
        cushion1->SetName("SofaCushion1");
        cushion1->SetMeshMaterial("Sofa");
        cushion1->SetKinematic(false);
        cushion1->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaCushion0"));
        cushion1->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaCushion1_ConvexMesh"));
        cushion1->SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
        cushion1->UpdateRigidBodyMassAndInertia(cushionDensity);
        cushion1->SetCollisionType(CollisionType::BOUNCEABLE);

        CreateGameObject();
        GameObject* cushion2 = GetGameObjectByIndex(GetGameObjectCount() - 1);
        cushion2->SetPosition(2.0f, 0.1f, 0.1f);
        cushion2->SetModel("SofaCushion2");
        cushion2->SetName("SofaCushion2");
        cushion2->SetMeshMaterial("Sofa");
        cushion2->SetKinematic(false);
        cushion2->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaCushion2"));
        cushion2->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaCushion2_ConvexMesh"));
        cushion2->SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
        cushion2->UpdateRigidBodyMassAndInertia(cushionDensity);
        cushion2->SetCollisionType(CollisionType::BOUNCEABLE);

        CreateGameObject();
        GameObject* cushion3 = GetGameObjectByIndex(GetGameObjectCount() - 1);
        cushion3->SetPosition(2.0f, 0.1f, 0.1f);
        cushion3->SetModel("SofaCushion3");
        cushion3->SetName("SofaCushion3");
        cushion3->SetMeshMaterial("Sofa");
        cushion3->SetKinematic(false);
        cushion3->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaCushion3"));
        cushion3->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaCushion3_ConvexMesh"));
        cushion3->SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
        cushion3->UpdateRigidBodyMassAndInertia(cushionDensity);
        cushion3->SetCollisionType(CollisionType::BOUNCEABLE);

        CreateGameObject();
        GameObject* cushion4 = GetGameObjectByIndex(GetGameObjectCount() - 1);
        cushion4->SetPosition(2.0f, 0.1f, 0.1f);
        cushion4->SetModel("SofaCushion4");
        cushion4->SetName("SofaCushion4");
        cushion4->SetMeshMaterial("Sofa");
        cushion4->SetKinematic(false);
        cushion4->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaCushion4"));
        cushion4->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaCushion4_ConvexMesh"));
        cushion4->SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
        cushion4->UpdateRigidBodyMassAndInertia(15.0f);
        cushion4->SetCollisionType(CollisionType::BOUNCEABLE);

        CreateGameObject();
        GameObject* tree = GetGameObjectByIndex(GetGameObjectCount() - 1);
        tree->SetPosition(0.75f, 0.1f, 6.2f);
        tree->SetModel("ChristmasTree");
        tree->SetName("ChristmasTree");
        tree->SetMeshMaterial("Tree");
        tree->SetMeshMaterialByMeshName("Balls", "Gold");



        {
            PhysicsFilterData filterData;
            filterData.raycastGroup = RAYCAST_ENABLED;
            filterData.collisionGroup = CollisionGroup::NO_COLLISION;
            filterData.collidesWith = CollisionGroup::NO_COLLISION;



            CreateGameObject();
            GameObject* smallChestOfDrawers = GetGameObjectByIndex(GetGameObjectCount() - 1);
            smallChestOfDrawers->SetModel("SmallChestOfDrawersFrame");
            smallChestOfDrawers->SetMeshMaterial("Drawers");
            smallChestOfDrawers->SetName("SmallDrawersHis");
            smallChestOfDrawers->SetPosition(0.1f, 0.1f, 4.45f);
            smallChestOfDrawers->SetRotationY(NOOSE_PI / 2);
            smallChestOfDrawers->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallChestOfDrawersFrame"));
            smallChestOfDrawers->SetOpenState(OpenState::NONE, 0, 0, 0);
            smallChestOfDrawers->SetAudioOnOpen("DrawerOpen.wav", 1.0f);
            smallChestOfDrawers->SetAudioOnClose("DrawerOpen.wav", 1.0f);

            PhysicsFilterData filterData3;
            filterData3.raycastGroup = RAYCAST_DISABLED;
            filterData3.collisionGroup = CollisionGroup::ENVIROMENT_OBSTACLE;
            filterData3.collidesWith = CollisionGroup(GENERIC_BOUNCEABLE | BULLET_CASING | PLAYER | RAGDOLL);
            smallChestOfDrawers->SetKinematic(true);
            smallChestOfDrawers->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallChestOfDrawersFrame_ConvexMesh"));
            smallChestOfDrawers->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallChestOfDrawersFrame_ConvexMesh1"));
            smallChestOfDrawers->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallChestOfDrawersFrameLeftSide_ConvexMesh"));
            smallChestOfDrawers->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallChestOfDrawersFrameRightSide_ConvexMesh"));
            smallChestOfDrawers->SetCollisionType(CollisionType::STATIC_ENVIROMENT);

            CreateGameObject();
            GameObject* smallChestOfDrawers2 = GetGameObjectByIndex(GetGameObjectCount() - 1);
            smallChestOfDrawers2->SetModel("SmallChestOfDrawersFrame");
            smallChestOfDrawers2->SetMeshMaterial("Drawers");
            smallChestOfDrawers2->SetName("SmallDrawersHers");
            smallChestOfDrawers2->SetPosition(8.9, 0.1f, 8.3f);
            smallChestOfDrawers2->SetRotationY(NOOSE_PI);
            smallChestOfDrawers2->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallChestOfDrawersFrame"));
            smallChestOfDrawers2->SetOpenState(OpenState::NONE, 0, 0, 0);
            smallChestOfDrawers2->SetAudioOnOpen("DrawerOpen.wav", 1.0f);
            smallChestOfDrawers2->SetAudioOnClose("DrawerOpen.wav", 1.0f);
            smallChestOfDrawers2->SetKinematic(true);
            smallChestOfDrawers2->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallChestOfDrawersFrame_ConvexMesh"));
            smallChestOfDrawers2->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallChestOfDrawersFrame_ConvexMesh1"));
            smallChestOfDrawers2->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallChestOfDrawersFrameLeftSide_ConvexMesh"));
            smallChestOfDrawers2->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallChestOfDrawersFrameRightSide_ConvexMesh"));
            smallChestOfDrawers2->SetCollisionType(CollisionType::STATIC_ENVIROMENT);


            CreateGameObject();
            GameObject* lamp2 = GetGameObjectByIndex(GetGameObjectCount() - 1);
            //lamp.SetModel("LampFullNoGlobe");
            lamp2->SetModel("Lamp");
            lamp2->SetName("Lamp");
            lamp2->SetMeshMaterial("Lamp");
            lamp2->SetPosition(glm::vec3(0.25f, 0.88, 0.105f) + glm::vec3(0.1f, 0.1f, 4.45f));
            lamp2->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("LampFull"));
            lamp2->SetKinematic(false);
            lamp2->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("LampConvexMesh_0"));
            lamp2->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("LampConvexMesh_1"));
            lamp2->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("LampConvexMesh_2"));
            lamp2->SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
            lamp2->UpdateRigidBodyMassAndInertia(20.0f);
            lamp2->SetCollisionType(CollisionType::BOUNCEABLE);

       /*    GameObject& toilet = _gameObjects.emplace_back();
            toilet.SetPosition(11.2f, 0.1f, 3.65f);
            toilet.SetModel("Toilet");
            toilet.SetName("Toilet");
            toilet.SetMeshMaterial("Toilet");
            toilet.SetRaycastShapeFromModel(OpenGLAssetManager::GetModel("Toilet"));
            toilet.SetRotationY(HELL_PI * 0.5f);
            toilet.SetKinematic(true);
            toilet.AddCollisionShapeFromConvexMesh(&OpenGLAssetManager::GetModel("Toilet_ConvexMesh")->_meshes[0], sofaFilterData);
            toilet.AddCollisionShapeFromConvexMesh(&OpenGLAssetManager::GetModel("Toilet_ConvexMesh1")->_meshes[0], sofaFilterData);
            toilet.SetModelMatrixMode(ModelMatrixMode::GAME_TRANSFORM);


            GameObject& toiletSeat = _gameObjects.emplace_back();
            toiletSeat.SetModel("ToiletSeat");
            toiletSeat.SetPosition(0, 0.40727, -0.2014);
            toiletSeat.SetName("ToiletSeat");
            toiletSeat.SetMeshMaterial("Toilet");
            toiletSeat.SetParentName("Toilet");
            toiletSeat.SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f);
            toiletSeat.SetOpenAxis(OpenAxis::ROTATION_NEG_X);
            toiletSeat.SetRaycastShapeFromModel(OpenGLAssetManager::GetModel("ToiletSeat"));

            GameObject& toiletLid = _gameObjects.emplace_back();
            toiletLid.SetPosition(0, 0.40727, -0.2014);
            toiletLid.SetModel("ToiletLid");
            toiletLid.SetName("ToiletLid");
            toiletLid.SetMeshMaterial("Toilet");
            toiletLid.SetParentName("Toilet");
            toiletLid.SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f);
            toiletLid.SetOpenAxis(OpenAxis::ROTATION_POS_X);
            toiletLid.SetRaycastShapeFromModel(OpenGLAssetManager::GetModel("ToiletLid"));
            */

            //  lamp.userData = new PhysicsObjectData(PhysicsObjectType::GAME_OBJECT, &_gameObjects[_gameObjects.size()-1]);


            CreateGameObject();
            GameObject* smallChestOfDrawer_1 = GetGameObjectByIndex(GetGameObjectCount() - 1);
            smallChestOfDrawer_1->SetModel("SmallDrawerTop");
            smallChestOfDrawer_1->SetMeshMaterial("Drawers");
            smallChestOfDrawer_1->SetParentName("SmallDrawersHis");
            smallChestOfDrawer_1->SetName("TopDraw");
            smallChestOfDrawer_1->SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f);
            smallChestOfDrawer_1->SetOpenAxis(OpenAxis::TRANSLATE_Z);
            smallChestOfDrawer_1->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallDrawerTop"));
            smallChestOfDrawer_1->SetKinematic(true);
            smallChestOfDrawer_1->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallDrawerTop_ConvexMesh0"));
            smallChestOfDrawer_1->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallDrawerTop_ConvexMesh1"));
            smallChestOfDrawer_1->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallDrawerTop_ConvexMesh2"));
            smallChestOfDrawer_1->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallDrawerTop_ConvexMesh3"));
            smallChestOfDrawer_1->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallDrawerTop_ConvexMesh4"));
            smallChestOfDrawer_1->SetCollisionType(CollisionType::STATIC_ENVIROMENT);

            CreateGameObject();
            GameObject* smallChestOfDrawer_2 = GetGameObjectByIndex(GetGameObjectCount() - 1);
            smallChestOfDrawer_2->SetModel("SmallDrawerSecond");
            smallChestOfDrawer_2->SetMeshMaterial("Drawers");
            smallChestOfDrawer_2->SetParentName("SmallDrawersHis");
			smallChestOfDrawer_2->SetName("SecondDraw");
			smallChestOfDrawer_2->SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f);
            smallChestOfDrawer_2->SetOpenAxis(OpenAxis::TRANSLATE_Z);
            smallChestOfDrawer_2->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallDrawerSecond"));

            CreateGameObject();
            GameObject* smallChestOfDrawer_3 = GetGameObjectByIndex(GetGameObjectCount() - 1);
            smallChestOfDrawer_3->SetModel("SmallDrawerThird");
            smallChestOfDrawer_3->SetMeshMaterial("Drawers");
			smallChestOfDrawer_3->SetParentName("SmallDrawersHis");
			smallChestOfDrawer_3->SetName("ThirdDraw");
			smallChestOfDrawer_3->SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f);
            smallChestOfDrawer_3->SetOpenAxis(OpenAxis::TRANSLATE_Z);
            smallChestOfDrawer_3->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallDrawerThird"));

            CreateGameObject();
            GameObject* smallChestOfDrawer_4 = GetGameObjectByIndex(GetGameObjectCount() - 1);
            smallChestOfDrawer_4->SetModel("SmallDrawerFourth");
            smallChestOfDrawer_4->SetMeshMaterial("Drawers");
            smallChestOfDrawer_4->SetParentName("SmallDrawersHis");
            smallChestOfDrawer_4->SetName("ForthDraw");
            smallChestOfDrawer_4->SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f);
            smallChestOfDrawer_4->SetOpenAxis(OpenAxis::TRANSLATE_Z);
            smallChestOfDrawer_4->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallDrawerFourth"));
        }



        for (int y = 0; y < 12; y++) {

            CreateGameObject();
            GameObject* cube = GetGameObjectByIndex(GetGameObjectCount() - 1);
            float halfExtent = 0.1f;
            cube->SetPosition(2.0f, y * halfExtent * 2 + 0.2f, 3.5f);
            cube->SetModel("ChristmasPresent");
            cube->SetName("Present");

            if (y == 0 || y == 4 || y == 8) {
                cube->SetMeshMaterial("PresentA");
            }
            if (y == 1 || y == 5 || y == 9) {
                cube->SetMeshMaterial("PresentB");
            }
            if (y == 2 || y == 6 || y == 10) {
                cube->SetMeshMaterial("PresentC");
            }
            if (y == 3 || y == 7 || y == 11) {
                cube->SetMeshMaterial("PresentD");
            }

            cube->SetMeshMaterialByMeshName("Bow", "Gold");

            Transform transform;
            transform.position = glm::vec3(2.0f, y * halfExtent * 2 + 0.2f, 3.5f);

            PxShape* collisionShape = Physics::CreateBoxShape(halfExtent, halfExtent, halfExtent);
            PxShape* raycastShape = Physics::CreateBoxShape(halfExtent, halfExtent, halfExtent);

            PhysicsFilterData filterData;
            filterData.raycastGroup = RAYCAST_DISABLED;
            filterData.collisionGroup = CollisionGroup::GENERIC_BOUNCEABLE;
            filterData.collidesWith = (CollisionGroup)(ENVIROMENT_OBSTACLE | GENERIC_BOUNCEABLE | RAGDOLL);

            cube->SetKinematic(false);
            cube->AddCollisionShape(collisionShape, filterData);
            cube->SetRaycastShape(raycastShape);
            cube->SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
            cube->UpdateRigidBodyMassAndInertia(20.0f);
            cube->SetCollisionType(CollisionType::BOUNCEABLE);
        }

    }


    // GO HERE

    if (false) {
        testIndex = CreateAnimatedGameObject();
        AnimatedGameObject& glock = g_animatedGameObjects[testIndex];
        glock.SetFlag(AnimatedGameObject::Flag::NONE);
        glock.SetPlayerIndex(1);
        //glock.SetSkinnedModel("Glock");
        glock.SetSkinnedModel("Tokarev");
        glock.SetName("Tokarev");
        // glock.SetAllMeshMaterials("Tokarev");
        glock.SetAnimationModeToBindPose();
        glock.SetMeshMaterialByMeshName("ArmsMale", "Hands");
        glock.SetMeshMaterialByMeshName("ArmsFemale", "FemaleArms");
        glock.SetMeshMaterialByMeshName("TokarevBody", "Tokarev");
        glock.SetMeshMaterialByMeshName("TokarevMag", "TokarevMag");
        glock.SetMeshMaterialByMeshName("TokarevGripPolymer", "TokarevGrip");
        glock.SetMeshMaterialByMeshName("TokarevGripPolyWood", "TokarevGrip");
        //glock.PlayAndLoopAnimation("Glock_Walk", 1.0f);
        glock.PlayAndLoopAnimation("Tokarev_Reload", 1.0f);
        glock.SetPosition(glm::vec3(2.5f, 1.5f, 3));
        glock.SetScale(0.01);
    }
    if (false) {
        testIndex = CreateAnimatedGameObject();
        AnimatedGameObject& glock = g_animatedGameObjects[testIndex];
        glock.SetFlag(AnimatedGameObject::Flag::NONE);
        glock.SetPlayerIndex(1);
        glock.SetSkinnedModel("Glock");
        glock.SetName("Glock");
        glock.SetAllMeshMaterials("Glock");
        glock.SetAnimationModeToBindPose();
        glock.PlayAndLoopAnimation("Glock_Reload", 1.0f);
        glock.SetPosition(glm::vec3(2.5f, 1.5f, 3));
        glock.SetScale(0.01);
    }




    /*
    glock.SetName("Glock");
    glock.SetSkinnedModel("Glock");
    glock.SetAnimationModeToBindPose();
    glock.SetFlag(AnimatedGameObject::Flag::CHARACTER_MODEL);
    //glock.PlayAndLoopAnimation("Glock_Idle", 1.0f);
    glock.SetAllMeshMaterials("Glock");
    glock.SetScale(0.01f);
    glock.SetPosition(glm::vec3(2.5f, 1.5f, 3));
    glock.SetRotationY(HELL_PI * 0.5f);*/


   /* auto glock = GetAnimatedGameObjectByName("Glock");
    glock->SetScale(0.01f);
    glock->SetScale(0.00f);
    glock->SetPosition(glm::vec3(1.3f, 1.1, 3.5f));

    auto mp7 = GetAnimatedGameObjectByName("MP7_test");
    mp7->SetScale(0.01f);
    mp7->SetScale(0.00f);
    mp7->SetPosition(glm::vec3(2.3f, 1.1, 3.5f));
    */

    /*AnimatedGameObject& mp7 = _animatedGameObjects.emplace_back(AnimatedGameObject());
    mp7.SetName("MP7");
    mp7.SetSkinnedModel("MP7_test");
    mp7.SetAnimatedTransformsToBindPose();
    mp7.PlayAndLoopAnimation("MP7_ReloadTest", 1.0f);
    mp7.SetMaterial("Glock");
    mp7.SetMeshMaterial("manniquen1_2.001", "Hands");
    mp7.SetMeshMaterial("manniquen1_2", "Hands");
    mp7.SetMeshMaterial("SK_FPSArms_Female.001", "FemaleArms");
    mp7.SetMeshMaterial("SK_FPSArms_Female", "FemaleArms");
    mp7.SetScale(0.01f);
    mp7.SetPosition(glm::vec3(2.5f, 1.5f, 4));
    mp7.SetRotationY(HELL_PI * 0.5f);*/


    //////////////////////
    //                  //
    //      AKS74U      //

   /* if (false) {
        AnimatedGameObject& aks = g_animatedGameObjects.emplace_back(AnimatedGameObject());
        aks.SetName("AKS74U_TEST");
        aks.SetSkinnedModel("AKS74U");
        aks.SetAnimationModeToBindPose();
        //aks.PlayAndLoopAnimation("AKS74U_ReloadEmpty", 0.2f);
        aks.PlayAndLoopAnimation("AKS74U_Idle", 1.0f);
        aks.SetMeshMaterialByMeshName("manniquen1_2.001", "Hands");
        aks.SetMeshMaterialByMeshName("manniquen1_2", "Hands");
        aks.SetMeshMaterialByMeshName("SK_FPSArms_Female.001", "FemaleArms");
        aks.SetMeshMaterialByMeshName("SK_FPSArms_Female", "FemaleArms");
        aks.SetMeshMaterialByMeshIndex(2, "AKS74U_3");
        aks.SetMeshMaterialByMeshIndex(3, "AKS74U_3"); // possibly incorrect. this is the follower
        aks.SetMeshMaterialByMeshIndex(4, "AKS74U_1");
        aks.SetMeshMaterialByMeshIndex(5, "AKS74U_4");
        aks.SetMeshMaterialByMeshIndex(6, "AKS74U_0");
        aks.SetMeshMaterialByMeshIndex(7, "AKS74U_2");
        aks.SetMeshMaterialByMeshIndex(8, "AKS74U_1");  // Bolt_low. Possibly wrong
        aks.SetMeshMaterialByMeshIndex(9, "AKS74U_3"); // possibly incorrect.
        aks.SetScale(0.01f);
        aks.SetPosition(glm::vec3(2.5f, 1.5f, 3));
        aks.SetRotationY(HELL_PI * 0.5f);
    }*/

    /////////////////////
    //                 //
    //      GLOCK      //

    /*if (false) {
        AnimatedGameObject& glock = _animatedGameObjects.emplace_back(AnimatedGameObject());
        glock.SetName("GLOCK_TEST");
        glock.SetSkinnedModel("Glock");
        glock.SetAnimationModeToBindPose();
        glock.PlayAndLoopAnimation("Glock_Reload", 0.2f);
        glock.PlayAndLoopAnimation("AKS74U_Idle", 1.0f);
        glock.SetAllMeshMaterials("Glock");
        glock.SetScale(0.01f);
        glock.SetPosition(glm::vec3(2.5f, 1.5f, 3));
        glock.SetRotationY(HELL_PI * 0.5f);
    }*/

    /*AnimatedGameObject& nurse = _animatedGameObjects.emplace_back(AnimatedGameObject());
    nurse.SetName("NURSEGUY");
    nurse.SetSkinnedModel("NurseGuy");
    nurse.SetAnimatedTransformsToBindPose();
    //nurse.PlayAndLoopAnimation("NurseGuy_Glock_Idle", 1.0f);
    nurse.SetMaterial("Glock");
    //  glock.SetScale(0.01f);
    nurse.SetPosition(glm::vec3(1.5f, 0.1f, 3));
    // glock.SetRotationY(HELL_PI * 0.5f);
    */

    /*
    AnimatedGameObject& dyingGuy = _animatedGameObjects.emplace_back(AnimatedGameObject());
    dyingGuy.SetName("DyingGuy");
    dyingGuy.SetSkinnedModel("DyingGuy");
    dyingGuy.SetAnimationModeToBindPose();
    dyingGuy.PlayAndLoopAnimation("DyingGuy_Death", 1.0f);
    dyingGuy.SetMaterial("Glock");
    dyingGuy.SetPosition(glm::vec3(1.5f, -10.1f, 3));
    dyingGuy.SetRotationX(HELL_PI * 0.5f);
    dyingGuy.SetMeshMaterial("CC_Base_Body", "UniSexGuyBody");
    dyingGuy.SetMeshMaterial("CC_Base_Eye", "UniSexGuyBody");
    dyingGuy.SetMeshMaterial("Biker_Jeans", "UniSexGuyJeans");
    dyingGuy.SetMeshMaterial("CC_Base_Eye", "UniSexGuyEyes");
    dyingGuy.SetMeshMaterial("Glock", "Glock");
    dyingGuy.SetMeshMaterial("SM_Knife_01", "Knife");
    dyingGuy.SetMeshMaterial("Shotgun_Mesh", "Shotgun");
    dyingGuy.SetMeshMaterialByIndex(13, "UniSexGuyHead");*/

    /*

    AnimatedGameObject& unisexGuy = _animatedGameObjects.emplace_back(AnimatedGameObject());
    unisexGuy.SetName("UNISEXGUY");
    unisexGuy.SetSkinnedModel("UniSexGuyScaled");
    //unisexGuy.SetAnimationModeToBindPose();
    unisexGuy.PlayAndLoopAnimation("Character_Glock_Walk", 1.0f);
    //unisexGuy.PlayAndLoopAnimation("Character_Glock_Walk", 1.0f);
    //unisexGuy.PlayAndLoopAnimation("UnisexGuy_Death", 1.0f);
    //unisexGuy.SetRotationX(HELL_PI * 0.5f);
    unisexGuy.SetMaterial("NumGrid");
    unisexGuy.SetPosition(glm::vec3(3.0f, 0.1f, 3));
    unisexGuy.SetMeshMaterial("CC_Base_Body", "UniSexGuyBody");
    unisexGuy.SetMeshMaterial("CC_Base_Eye", "UniSexGuyBody");
    unisexGuy.SetMeshMaterial("Biker_Jeans", "UniSexGuyJeans");
    unisexGuy.SetMeshMaterial("CC_Base_Eye", "UniSexGuyEyes");
    unisexGuy.SetMeshMaterial("Glock", "Glock");
    unisexGuy.SetMeshMaterial("SM_Knife_01", "Knife");
    unisexGuy.SetMeshMaterial("Shotgun_Mesh", "Shotgun");
    unisexGuy.SetMeshMaterialByIndex(13, "UniSexGuyHead");



    PxU32 ragdollCollisionGroupFlags = RaycastGroup::RAYCAST_ENABLED;
    unisexGuy.LoadRagdoll("UnisexGuy4.rag", ragdollCollisionGroupFlags);

    unisexGuy.PrintMeshNames();

    unisexGuy.AddSkippedMeshIndexByName("Glock");
    unisexGuy.AddSkippedMeshIndexByName("SM_Knife_01");
    unisexGuy.AddSkippedMeshIndexByName("Shotgun_Mesh");


    unisexGuy.SetMeshMaterial("FrontSight_low", "AKS74U_0");
    unisexGuy.SetMeshMaterial("Receiver_low", "AKS74U_1");
    unisexGuy.SetMeshMaterial("BoltCarrier_low", "AKS74U_1");
    unisexGuy.SetMeshMaterial("SafetySwitch_low", "AKS74U_0");
    unisexGuy.SetMeshMaterial("MagRelease_low", "AKS74U_0");
    unisexGuy.SetMeshMaterial("Pistol_low", "AKS74U_2");
    unisexGuy.SetMeshMaterial("Trigger_low", "AKS74U_1");
    unisexGuy.SetMeshMaterial("Magazine_Housing_low", "AKS74U_3");
    unisexGuy.SetMeshMaterial("BarrelTip_low", "AKS74U_4");


    unisexGuy.AddSkippedMeshIndexByName("FrontSight_low");
    unisexGuy.AddSkippedMeshIndexByName("Receiver_low");
    unisexGuy.AddSkippedMeshIndexByName("BoltCarrier_low");
    unisexGuy.AddSkippedMeshIndexByName("SafetySwitch_low");
    unisexGuy.AddSkippedMeshIndexByName("MagRelease_low");
    unisexGuy.AddSkippedMeshIndexByName("Pistol_low");
    unisexGuy.AddSkippedMeshIndexByName("Trigger_low");
    unisexGuy.AddSkippedMeshIndexByName("Magazine_Housing_low");
    unisexGuy.AddSkippedMeshIndexByName("BarrelTip_low");
   */

    /*
    AnimatedGameObject& glock = _animatedGameObjects.emplace_back(AnimatedGameObject());
    glock.SetName("Glock");
    glock.SetSkinnedModel("Glock");
    glock.SetAnimatedTransformsToBindPose();
    glock.PlayAndLoopAnimation("Glock_Idle", 1.0f);
    glock.SetMaterial("Glock");
    glock.SetMeshMaterial("manniquen1_2.001", "Hands");
    glock.SetMeshMaterial("manniquen1_2", "Hands");
    glock.SetMeshMaterial("SK_FPSArms_Female.001", "FemaleArms");
    glock.SetMeshMaterial("SK_FPSArms_Female", "FemaleArms");
    glock.SetScale(0.01f);
    glock.SetPosition(glm::vec3(2.5f, 1.5f, 3));
    glock.SetRotationY(HELL_PI * 0.5f);

    */

    /*
    AnimatedGameObject & shotgun = _animatedGameObjects.emplace_back(AnimatedGameObject());
    shotgun.SetName("ShotgunTest");
    shotgun.SetSkinnedModel("Shotgun");
    shotgun.SetAnimatedTransformsToBindPose();
    shotgun.PlayAndLoopAnimation("Shotgun_Fire", 0.5f);
    shotgun.SetMaterial("Shotgun");
    shotgun.SetMeshMaterial("Arms", "Hands");
  //  glock.SetMeshMaterial("manniquen1_2", "Hands");
  //  glock.SetMeshMaterial("SK_FPSArms_Female.001", "FemaleArms");
 //   glock.SetMeshMaterial("SK_FPSArms_Female", "FemaleArms");
    shotgun.SetScale(0.02f);
    shotgun.SetPosition(glm::vec3(2.5f, 1.5f, 4));
    shotgun.SetRotationY(HELL_PI * 0.5f);
    shotgun.SetMeshMaterialByIndex(2, "Shell");
    */
    /*
    for (auto& mesh : shotgun._skinnedModel->m_meshEntries) {
        std::cout << "shit         shit      " << mesh.Name << "\n";
   }

   */

    /* THIS IS THE AK U WERE TESTING WITH ON STREAM WHEN DOING THE STILL UNFINISHED AK MAG DROP THING
        AnimatedGameObject& aks74u = _animatedGameObjects.emplace_back(AnimatedGameObject());

        aks74u.SetName("AKS74U");
        aks74u.SetSkinnedModel("AKS74U");
        aks74u.SetMeshMaterial("manniquen1_2.001", "Hands");
        aks74u.SetMeshMaterial("manniquen1_2", "Hands");
        aks74u.SetMeshMaterialByIndex(2, "AKS74U_3");
        aks74u.SetMeshMaterialByIndex(3, "AKS74U_3"); // possibly incorrect. this is the follower
        aks74u.SetMeshMaterialByIndex(4, "AKS74U_1");
        aks74u.SetMeshMaterialByIndex(5, "AKS74U_4");
        aks74u.SetMeshMaterialByIndex(6, "AKS74U_0");
        aks74u.SetMeshMaterialByIndex(7, "AKS74U_2");
        aks74u.SetMeshMaterialByIndex(8, "AKS74U_1");  // Bolt_low. Possibly wrong
        aks74u.SetMeshMaterialByIndex(9, "AKS74U_3"); // possibly incorrect.


		aks74u.SetMeshMaterialByIndex(1, "AKS74U_3");
	//	aks74u.PlayAndLoopAnimation("AKS74U_ReloadEmpty", 0.1f);
		aks74u.PlayAndLoopAnimation("AKS74U_Idle", 0.1f);
        aks74u.SetScale(0.01f);
        aks74u.SetPosition(glm::vec3(2.5f, 1.5f, 3));
        aks74u.SetRotationY(HELL_PI * 0.5f);   */

        /*

        AnimatedGameObject& enemy2 = _animatedGameObjects.emplace_back(AnimatedGameObject());
        enemy2.SetName("Wife");
        enemy2.SetSkinnedModel("UniSexGuy2");
        enemy2.SetMeshMaterial("CC_Base_Body", "UniSexGuyBody");
        enemy2.SetMeshMaterial("CC_Base_Eye", "UniSexGuyBody");
        enemy2.SetMeshMaterial("Biker_Jeans", "UniSexGuyJeans");
        enemy2.SetMeshMaterial("CC_Base_Eye", "UniSexGuyEyes");
        enemy2.SetMeshMaterialByIndex(1, "UniSexGuyHead");
        enemy2.PlayAndLoopAnimation("Character_Glock_Idle", 1.0f);
        enemy2.SetScale(0.01f);
        enemy2.SetPosition(glm::vec3(2.5f, 0.1f, 2));
        enemy2.SetRotationX(HELL_PI / 2);
        */
        /*

        AnimatedGameObject& aks74u = _animatedGameObjects.emplace_back(AnimatedGameObject());
        aks74u.SetName("AKS74U");
        aks74u.SetSkinnedModel("AKS74U");
        aks74u.PlayAnimation("AKS74U_Idle", 0.5f);

       /* AnimatedGameObject& glock = _animatedGameObjects.emplace_back(AnimatedGameObject());
        glock.SetName("Shotgun");
        glock.SetSkinnedModel("Shotgun");
        glock.PlayAndLoopAnimation("Shotgun_Walk", 1.0f);*/


        /*AnimatedGameObject& glock = _animatedGameObjects.emplace_back(AnimatedGameObject());
         glock.SetName("Glock");
         glock.SetSkinnedModel("Glock");
         glock.PlayAndLoopAnimation("Glock_Walk", 1.0f);
         glock.SetScale(0.04f);
         glock.SetPosition(glm::vec3(2, 1.2, 3.5));
         */


    /*
	AnimatedGameObject& aks74u = _animatedGameObjects.emplace_back(AnimatedGameObject());
	aks74u.SetName("Shotgun");
	aks74u.SetSkinnedModel("Shotgun");
	aks74u.PlayAnimation("Shotgun_Idle", 0.5f);
	aks74u.SetMaterial("Hands");
	aks74u.SetPosition(glm::vec3(2, 1.75f, 3.5));
	aks74u.SetScale(0.01f);
    */

    /*
	aks74u.SetMeshMaterialByIndex(2, "AKS74U_3");
	aks74u.SetMeshMaterialByIndex(3, "AKS74U_3"); // possibly incorrect. this is the follower
	aks74u.SetMeshMaterialByIndex(4, "AKS74U_1");
	aks74u.SetMeshMaterialByIndex(5, "AKS74U_4");
	aks74u.SetMeshMaterialByIndex(6, "AKS74U_0");
	aks74u.SetMeshMaterialByIndex(7, "AKS74U_2");
	aks74u.SetMeshMaterialByIndex(8, "AKS74U_1");  // Bolt_low. Possibly wrong
	aks74u.SetMeshMaterialByIndex(9, "AKS74U_3"); // possibly incorrect.
    */

    /*
   auto _skinnedModel = AssetManager::GetSkinnedModel("AKS74U");
	for (int i = 0; i < _skinnedModel->m_meshEntries.size(); i++) {
		auto& mesh = _skinnedModel->m_meshEntries[i];
        std::cout << i << ": " << mesh.Name << "\n";
		//if (mesh.Name == meshName) {
		//	_materialIndices[i] = AssetManager::GetMaterialIndex(materialName);
			//return;
		//}
	}*/
}

void Scene::ResetGameObjectStates() {
    for (GameObject& gameObject : g_gameObjects) {
        gameObject.LoadSavedState();
    }
}

void Scene::LoadMap(std::string mapPath) {

    CleanUp();
    File::LoadMap(mapPath);

    for (Wall& wall : _walls) {
        wall.CreateVertexData();
        wall.CreateMeshGL();
    }

    for (Floor& floor : _floors) {
        floor.CreateVertexData();
        floor.CreateMeshGL();
    }

    CreateCeilingsHack();

    for (Ceiling& ceiling : _ceilings) {
        ceiling.CreateVertexData();
        ceiling.CreateMeshGL();
    }

    LoadHardCodedObjects();

    RecreateDataStructures();
    ResetGameObjectStates();
}



void Scene::SaveMap(std::string mapPath) {
	File::SaveMap(mapPath);
}


void Scene::CleanUp() {
    for (Door& door : _doors) {
        door.CleanUp();
    }
    for (BulletCasing& bulletCasing : _bulletCasings) {
        bulletCasing.CleanUp();
    }
    for (GameObject& gameObject : g_gameObjects) {
        gameObject.CleanUp();
    }
    for (Toilet& toilet: _toilets) {
        toilet.CleanUp();
    }
    for (Window& window : _windows) {
        window.CleanUp();
    }
    for (AnimatedGameObject& animatedGameObject : g_animatedGameObjects) {
        animatedGameObject.DestroyRagdoll();
    }

    _toilets.clear();
    _bloodDecals.clear();
    _spawnPoints.clear();
    _bulletCasings.clear();
    g_bulletHoleDecals.clear();
    _walls.clear();
    _floors.clear();
	_ceilings.clear();
	_doors.clear();
    _windows.clear();
	g_gameObjects.clear();
    g_animatedGameObjects.clear();
    _pickUps.clear();
    _lights.clear();

	if (_sceneTriangleMesh) {
		_sceneTriangleMesh->release();
		_sceneShape->release();
    }
}

void Scene::AddLight(Light& light) {
    _lights.push_back(light);
}

void Scene::AddDoor(Door& door) {
    _doors.push_back(door);
}

void Scene::CreatePointCloud() {

    float pointSpacing = Renderer_OLD::GetPointCloudSpacing();;

    _cloudPoints.clear();

    //int rayCount = 0;

    for (auto& wall : _walls) {
        Line line;
        line.p1 = Point(wall.begin, YELLOW);
        line.p2 = Point(wall.end, YELLOW);
        Line line2;
        line2.p1 = Point(wall.begin + glm::vec3(0, wall.height, 0), YELLOW);
        line2.p2 = Point(wall.end + glm::vec3(0, wall.height, 0), YELLOW);
        Line line3;
        line3.p1 = Point(wall.begin + glm::vec3(0, 0, 0), YELLOW);
        line3.p2 = Point(wall.begin + glm::vec3(0, wall.height, 0), YELLOW);
        Line line4;
        line4.p1 = Point(wall.end + glm::vec3(0, 0, 0), YELLOW);
        line4.p2 = Point(wall.end + glm::vec3(0, wall.height, 0), YELLOW);
        glm::vec3 dir = glm::normalize(wall.end - wall.begin);
        float wallLength = distance(wall.begin, wall.end);
        for (float x = pointSpacing * 0.5f; x < wallLength; x += pointSpacing) {
            glm::vec3 pos = wall.begin + (dir * x);
            for (float y = pointSpacing * 0.5f; y < wall.height; y += pointSpacing) {
                CloudPointOld cloudPoint;
                cloudPoint.position = glm::vec4(pos, 0);
                cloudPoint.position.y = wall.begin.y + y;
                glm::vec3 wallVector = glm::normalize(wall.end - wall.begin);
                cloudPoint.normal = glm::vec4(-wallVector.z, 0, wallVector.x, 0);
                _cloudPoints.push_back(cloudPoint);
            }
        }
    }

    for (auto& floor : _floors) {

        //inline bool PointIn2DTriangle(glm::vec2 pt, glm::vec2 v1, glm::vec2 v2, glm::vec2 v3) {

        float minX = std::min(std::min(std::min(floor.v1.position.x, floor.v2.position.x), floor.v3.position.x), floor.v4.position.x);
        float minZ = std::min(std::min(std::min(floor.v1.position.z, floor.v2.position.z), floor.v3.position.z), floor.v4.position.z);
        float maxX = std::max(std::max(std::max(floor.v1.position.x, floor.v2.position.x), floor.v3.position.x), floor.v4.position.x);
        float maxZ = std::max(std::max(std::max(floor.v1.position.z, floor.v2.position.z), floor.v3.position.z), floor.v4.position.z);
        float floorHeight = floor.v1.position.y;

        //float floorWidth = floor.x2 - floor.x1;
        //float floorDepth = floor.z2 - floor.z1;
        for (float x = minX + (pointSpacing * 0.5f); x < maxX; x += pointSpacing) {
            for (float z = minZ + (pointSpacing * 0.5f); z < maxZ; z += pointSpacing) {
                CloudPointOld cloudPoint;
                cloudPoint.position = glm::vec4(x, floorHeight, z, 0);
                cloudPoint.normal = glm::vec4(NRM_Y_UP, 0);
                _cloudPoints.push_back(cloudPoint);
            }
        }

        /*
        for (float x = pointSpacing * 0.5f; x < floorWidth; x += pointSpacing) {
            for (float z = pointSpacing * 0.5f; z < floorDepth; z += pointSpacing) {
                CloudPoint cloudPoint;
                cloudPoint.position = glm::vec4(floor.x1 + x, floor.height, floor.z1 + z, 0);
                cloudPoint.normal = glm::vec4(NRM_Y_UP, 0);
                _cloudPoints.push_back(cloudPoint);
            }
        }*/
    }

    for (auto& ceiling : _ceilings) {
        Line line;
        line.p1 = Point(glm::vec3(ceiling.x1, ceiling.height, ceiling.z1), YELLOW);
        line.p2 = Point(glm::vec3(ceiling.x1, ceiling.height, ceiling.z2), YELLOW);
        Line line2;
        line2.p1 = Point(glm::vec3(ceiling.x1, ceiling.height, ceiling.z1), YELLOW);
        line2.p2 = Point(glm::vec3(ceiling.x2, ceiling.height, ceiling.z1), YELLOW);
        Line line3;
        line3.p1 = Point(glm::vec3(ceiling.x2, ceiling.height, ceiling.z2), YELLOW);
        line3.p2 = Point(glm::vec3(ceiling.x2, ceiling.height, ceiling.z1), YELLOW);
        Line line4;
        line4.p1 = Point(glm::vec3(ceiling.x2, ceiling.height, ceiling.z2), YELLOW);
        line4.p2 = Point(glm::vec3(ceiling.x1, ceiling.height, ceiling.z2), YELLOW);
        float ceilingWidth = ceiling.x2 - ceiling.x1;
        float ceilingDepth = ceiling.z2 - ceiling.z1;
        for (float x = pointSpacing * 0.5f; x < ceilingWidth; x += pointSpacing) {
            for (float z = pointSpacing * 0.5f; z < ceilingDepth; z += pointSpacing) {
                CloudPointOld cloudPoint;
                cloudPoint.position = glm::vec4(ceiling.x1 + x, ceiling.height, ceiling.z1 + z, 0);
                cloudPoint.normal = glm::vec4(NRM_Y_DOWN, 0);
                _cloudPoints.push_back(cloudPoint);
            }
        }
    }

    // Now remove any points that overlap doors

    for (int i = 0; i < _cloudPoints.size(); i++) {
        glm::vec2 p = { _cloudPoints[i].position.x, _cloudPoints[i].position.z };
        for (Door& door : Scene::_doors) {
            // Ignore if is point is above or below door
            if (_cloudPoints[i].position.y < door.position.y ||
                _cloudPoints[i].position.y > door.position.y + DOOR_HEIGHT) {
                continue;
            }
            // Check if it is inside the fucking door
            glm::vec2 p3 = { door.GetFloorplanVertFrontLeft().x, door.GetFloorplanVertFrontLeft().z };
            glm::vec2 p2 = { door.GetFloorplanVertFrontRight().x, door.GetFloorplanVertFrontRight().z };
            glm::vec2 p1 = { door.GetFloorplanVertBackRight().x, door.GetFloorplanVertBackRight().z };
            glm::vec2 p4 = { door.GetFloorplanVertBackLeft().x, door.GetFloorplanVertBackLeft().z };
            glm::vec2 p5 = { door.GetFloorplanVertFrontRight().x, door.GetFloorplanVertFrontRight().z };
            glm::vec2 p6 = { door.GetFloorplanVertBackRight().x, door.GetFloorplanVertBackRight().z };
            if (Util::PointIn2DTriangle(p, p1, p2, p3) || Util::PointIn2DTriangle(p, p4, p5, p6)) {
                _cloudPoints.erase(_cloudPoints.begin() + i);
                i--;
                break;
            }
        }

    }
}

void Scene::AddWall(Wall& wall) {
    _walls.push_back(wall);
}

void Scene::AddFloor(Floor& floor) {
    _floors.push_back(floor);
}

void Scene::LoadLightSetup(int index) {

    if (index == 2) {
        _lights.clear();
        Light lightD;
        lightD.position = glm::vec3(2.8, 2.2, 3.6);
        lightD.strength = 0.3f;
        lightD.radius = 7;
        _lights.push_back(lightD);

        Light lightB;
        lightB.position = glm::vec3(2.05, 2, 9.0);
        lightB.radius = 3.0f;
        lightB.strength = 5.0f * 0.25f;;
        lightB.radius = 4;
        lightB.color = RED;
        _lights.push_back(lightB);

        Light lightA;
        lightA.position = glm::vec3(11, 2.0, 6.0);
        lightA.strength = 1.0f * 0.25f;
        lightA.radius = 2;
        lightA.color = LIGHT_BLUE;
        _lights.push_back(lightA);

        Light lightC;
        lightC.position = glm::vec3(8.75, 2.2, 1.55);
        lightC.strength = 0.3f;
		lightC.radius = 6;
        //lightC.color = RED;
        //lightC.strength = 5.0f * 0.25f;;
        _lights.push_back(lightC);
    }
}



/*
AnimatedGameObject* Scene::GetAnimatedGameObjectByName(std::string name) {
    if (name == "undefined") {
        return nullptr;
    }
    for (AnimatedGameObject& gameObject : _animatedGameObjects) {
        if (gameObject.GetName() == name) {
            return &gameObject;
        }
    }
    std::cout << "Scene::GetAnimatedGameObjectByName() failed, no object with name \"" << name << "\"\n";
    return nullptr;
}*/

/*
std::vector<AnimatedGameObject>& Scene::GetAnimatedGameObjects() {
    return _animatedGameObjects;
}*/

void Scene::UpdateRTInstanceData() {

    _rtInstances.clear();
    RTInstance& houseInstance = _rtInstances.emplace_back(RTInstance());
    houseInstance.meshIndex = 0;
    houseInstance.modelMatrix = glm::mat4(1);
    houseInstance.inverseModelMatrix = glm::inverse(glm::mat4(1));

    for (Door& door : _doors) {
        RTInstance& doorInstance = _rtInstances.emplace_back(RTInstance());
        doorInstance.meshIndex = 1;
        doorInstance.modelMatrix = door.GetDoorModelMatrix();
        doorInstance.inverseModelMatrix = glm::inverse(doorInstance.modelMatrix);
    }
}

void Scene::RecreateAllPhysicsObjects() {

    std::vector<PxVec3> vertices;
    std::vector<unsigned int> indices;
    for (Wall& wall : _walls) {

        if (wall.begin.y > 2.0f) {
            continue;
        }

        for (Vertex& vertex : wall.vertices) {
            vertices.push_back(PxVec3(vertex.position.x, vertex.position.y, vertex.position.z));
            indices.push_back(indices.size());
        }
    }
    for (Floor& floor : _floors) {
        for (Vertex& vertex : floor.vertices) {
            vertices.push_back(PxVec3(vertex.position.x, vertex.position.y, vertex.position.z));
            indices.push_back(indices.size());
        }
    }
    for (Ceiling& ceiling : _ceilings) {
        for (Vertex& vertex : ceiling.vertices) {
            vertices.push_back(PxVec3(vertex.position.x, vertex.position.y, vertex.position.z));
            indices.push_back(indices.size());
        }
    }

    // commenting the shit below out prevented the crash it mentions, but who knows if that is gonna break something elsewhere

	/*if (_sceneTriangleMesh) {
		_sceneTriangleMesh->release();
	}
	if (_sceneShape) {
        _sceneShape->release(); // crashed here once? // again again./ figure out
	}*/

    if (vertices.size()) {;
        Transform transform;
        PhysicsFilterData filterData;
        filterData.raycastGroup = RAYCAST_ENABLED;
        filterData.collisionGroup = ENVIROMENT_OBSTACLE;
        filterData.collidesWith = (CollisionGroup)(GENERIC_BOUNCEABLE | BULLET_CASING | PLAYER | RAGDOLL);
		_sceneTriangleMesh = Physics::CreateTriangleMesh(vertices.size(), vertices.data(), vertices.size() / 3, indices.data());
		PxShapeFlags shapeFlags(PxShapeFlag::eSIMULATION_SHAPE);
        _sceneShape = Physics::CreateShapeFromTriangleMesh(_sceneTriangleMesh, shapeFlags);
        _sceneRigidDynamic = Physics::CreateRigidStatic(transform, filterData, _sceneShape);
		_sceneRigidDynamic->userData = new PhysicsObjectData(PhysicsObjectType::SCENE_MESH, nullptr);
    }

	for (Door& door : _doors) {
		door.CreatePhysicsObject();
	}
	for (Window& window : _windows) {
        window.CreatePhysicsObjects();
	}
}

void Scene::RemoveAllDecalsFromWindow(Window* window) {

    std::cout << "size was: " << g_bulletHoleDecals.size() << "\n";

    for (int i = 0; i < g_bulletHoleDecals.size(); i++) {
        PxRigidBody* decalParentRigid = g_bulletHoleDecals[i].GetPxRigidBodyParent();
        if (decalParentRigid == (void*)window->raycastBody //||
           // decalParentRigid == (void*)window->raycastBodyTop
            ) {
            g_bulletHoleDecals.erase(g_bulletHoleDecals.begin() + i);
            i--;
            std::cout << "removed decal " << i << " size is now: " << g_bulletHoleDecals.size() << "\n";
        }
    }
}

void Scene::ProcessPhysicsCollisions() {

    bool bulletCasingCollision = false;
    bool shotgunShellCollision = false;

    for (CollisionReport& report : Physics::GetCollisions()) {

        if (!report.rigidA || !report.rigidB) {
            if (!report.rigidA)
                std::cout << "report.rigidA was nullptr, ESCAPING!\n";
            if (!report.rigidB)
                std::cout << "report.rigidB was nullptr, ESCAPING!\n";
            continue;
        }

        const char* nameA = report.rigidA->getName();
        const char* nameB = report.rigidB->getName();
        if (nameA == "BulletCasing" ||
            nameB == "BulletCasing") {
            bulletCasingCollision = true;
        }
        if (nameA == "ShotgunShell" ||
            nameB == "ShotgunShell") {
            shotgunShellCollision = true;
        }
    }
    if (bulletCasingCollision) {
        Audio::PlayAudio("BulletCasingBounce.wav", Util::RandomFloat(0.2f, 0.3f));
    }
    if (shotgunShellCollision) {
        Audio::PlayAudio("ShellFloorBounce.wav", Util::RandomFloat(0.1f, 0.2f));
    }

    Physics::ClearCollisionLists();
}


void Scene::RecreateDataStructures() {

    // Remove all scene physx objects, if they exist
    if (_sceneTriangleMesh) {
        _sceneTriangleMesh->release();
    }
    if (_sceneShape) {
        _sceneShape->release();
    }
    if (_sceneRigidDynamic) {
        _sceneRigidDynamic->release();
    }

    CreateMeshData();
    CreatePointCloud();
    UpdateRTInstanceData();
    Renderer_OLD::CreatePointCloudBuffer();
    Renderer_OLD::CreateTriangleWorldVertexBuffer();
    RecreateAllPhysicsObjects();
    CalculateLightBoundingVolumes();
}

/*bool AnyHit(glm::vec3 origin, glm::vec3 direction, float distance) {

    RTMesh& mesh = Scene::_rtMesh[0]; // This is the main world

    std::cout << " mesh.baseVertex: " << mesh.baseVertex << "\n";
    std::cout << " mesh.vertexCount: " << mesh.vertexCount << "\n";

    for (unsigned int i = mesh.baseVertex; i < mesh.baseVertex + mesh.vertexCount; i += 3) {
        glm::vec3 p1 = Scene::_rtVertices[i + 0];
        glm::vec3 p2 = Scene::_rtVertices[i + 1];
        glm::vec3 p3 = Scene::_rtVertices[i + 2];
        auto result = Util::RayTriangleIntersectTest(p1, p2, p3, origin, direction);
        if (result.found && result.distance < distance) {
            return true;
        }
    }
    return false;
}*/



void Scene::CalculateLightBoundingVolumes() {

    return;
    /*
    std::vector<Triangle> triangles;

    RTMesh& mesh = Scene::_rtMesh[0]; // This is the main world
    for (unsigned int i = mesh.baseVertex; i < mesh.baseVertex + mesh.vertexCount; i += 3) {
        Triangle triangle;
        triangle.v0 = Scene::_rtVertices[i + 0];
        triangle.v1 = Scene::_rtVertices[i + 1];
        triangle.v2 = Scene::_rtVertices[i + 2];
        triangles.push_back(triangle);
    }

    //for (int i = 0; i < Scene::_lights.size(); i++) {
    {
        int i = 0;

        Light& light = Scene::_lights[i];
        light.boundingVolumes.clear();

        // Find the room the light is in
        for (int j = 0; j < Scene::_floors.size(); j++) {
            Floor& floor = _floors[j];
            if (floor.PointIsAboveThisFloor(light.position)) {
                std::cout << "LIGHT " << i << " is in room " << j << "\n";
                break;
            }
        }
        // Find which rooms it can see into
        for (int k = 0; k < Scene::_windows.size(); k++) {

            Window& window = _windows[k];

            glm::vec3 origin = light.position;
            glm::vec3 direction = glm::normalize(window.GetWorldSpaceCenter() - light.position);
            float distance = glm::distance(window.GetWorldSpaceCenter(), light.position);
            distance = std::min(distance, light.radius);
            if (Util::RayTracing::AnyHit(triangles, origin, direction, 0.01f, distance)) {
                //std::cout << " -can NOT see through window " << k << "\n";
            }
            else {
                std::cout << " -can see through window " << k << "\n";
                // Find which room this is
                glm::vec3 queryPoint = window.position + (direction * glm::vec3(0.2f));
                for (int j = 0; j < Scene::_floors.size(); j++) {
                    Floor& floor = _floors[j];
                    if (floor.PointIsAboveThisFloor(queryPoint)) {
                        std::cout << "  and into room " << j << "\n";
                        break;
                    }
                }
            }
        }

        for (int k = 0; k < Scene::_doors.size(); k++) {

            Door& door = _doors[k];

            glm::vec3 origin = light.position;
            glm::vec3 direction = glm::normalize(door.GetWorldDoorWayCenter() - light.position);
            float distance = glm::distance(door.GetWorldDoorWayCenter(), light.position);

            if (distance > light.radius) {
                if (Util::RayTracing::AnyHit(triangles, origin, direction, 0.01f, distance)) {
                    //
                }
                else {
                    std::cout << " -can see through door " << k << "\n";
                    // Find which room this is
                    glm::vec3 queryPoint = door.position + (direction * glm::vec3(0.2f));
                    for (int j = 0; j < Scene::_floors.size(); j++) {
                        Floor& floor = _floors[j];
                        if (floor.PointIsAboveThisFloor(queryPoint)) {
                            std::cout << "  and into room " << j << "\n";
                            break;
                        }
                    }
                }
            }
        }
    }*/
}

void Scene::CreateMeshData() {

    //for (Wall& wall : _walls) {
    //    wall.CreateMesh();
    //}
    //for (Floor& floor : _floors) {
    //    floor.CreateMeshGL();
    // }

	_rtVertices.clear();
	_rtMesh.clear();

    // RT vertices and mesh
    int baseVertex = 0;

    // House
    {
        for (Wall& wall : _walls) {
            //wall.CreateMesh();
            for (Vertex& vert : wall.vertices) {
                _rtVertices.push_back(vert.position);
            }
        }
        for (Floor& floor : _floors) {
            //wall.CreateMesh();
            for (Vertex& vert : floor.vertices) {
                _rtVertices.push_back(vert.position);
            }
        }
        for (Ceiling& ceiling : _ceilings) {
            //wall.CreateMesh();
            for (Vertex& vert : ceiling.vertices) {
                _rtVertices.push_back(vert.position);
            }
        }
        RTMesh mesh;
        mesh.vertexCount = _rtVertices.size() - baseVertex;
        mesh.baseVertex = baseVertex;
        _rtMesh.push_back(mesh);
    }

    // Door
    {
        baseVertex = _rtVertices.size();

        const float doorWidth = -0.794f;
        const float doorDepth = -0.0379f;
        const float doorHeight = 2.0f;

        // Front
        glm::vec3 pos0 = glm::vec3(0, 0, 0);
        glm::vec3 pos1 = glm::vec3(0.0, 0, doorWidth);
        // Back
        glm::vec3 pos2 = glm::vec3(doorDepth, 0, 0);
        glm::vec3 pos3 = glm::vec3(doorDepth, 0, doorWidth);

        _rtVertices.push_back(pos0);
        _rtVertices.push_back(pos1);
        _rtVertices.push_back(pos1 + glm::vec3(0, doorHeight, 0));

        _rtVertices.push_back(pos1 + glm::vec3(0, doorHeight, 0));
        _rtVertices.push_back(pos0 + glm::vec3(0, doorHeight, 0));
        _rtVertices.push_back(pos0);

        _rtVertices.push_back(pos3 + glm::vec3(0, doorHeight, 0));
        _rtVertices.push_back(pos3);
        _rtVertices.push_back(pos2);

        _rtVertices.push_back(pos2);
        _rtVertices.push_back(pos2 + glm::vec3(0, doorHeight, 0));
        _rtVertices.push_back(pos3 + glm::vec3(0, doorHeight, 0));

        RTMesh mesh;
        mesh.vertexCount = _rtVertices.size() - baseVertex;
        mesh.baseVertex = baseVertex;
        _rtMesh.push_back(mesh);
    }
}
