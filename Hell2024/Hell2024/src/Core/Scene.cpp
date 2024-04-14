#include "Scene.h"

#include <future>
#include <thread>

#include "../API/OpenGL/GL_assetManager.h"
#include "../BackEnd/BackEnd.h"
#include "../Core/AssetManager.h"
#include "../Renderer/Renderer_OLD.h"
#include "../Renderer/TextBlitter.h"
#include "../Util.hpp"
#include "../EngineState.hpp"

#include "Input.h"
#include "File.h"
#include "Player.h"
#include "Audio.hpp"


namespace Scene {

    void CreateCeilingsHack();
    void EvaluateDebugKeyPresses();
    void SetPlayerGroundedStates();
    void ProcessBullets();

}

void Scene::Update(float deltaTime) {

    CheckForDirtyLights();

    if (Input::KeyPressed(HELL_KEY_N)) {
        for (GameObject& gameObject : _gameObjects) {
            gameObject.LoadSavedState();
        }
        std::cout << "Loaded scene save state\n";
    }

    for (GameObject& gameObject : _gameObjects) {
        gameObject.Update(deltaTime);
        gameObject.UpdateRenderItems();
    }
    for (Door& door : _doors) {
        door.Update(deltaTime);
        door.UpdateRenderItems();
    }
    for (Window& window : _windows) {
        window.UpdateRenderItems();
    }
    SetPlayerGroundedStates();
    ProcessBullets();

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
    for (AnimatedGameObject& animatedGameObject : _animatedGameObjects) {
        // TO DO: render item
        animatedGameObject.Update(deltaTime);
    }

    ProcessPhysicsCollisions();
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
    for (GameObject& gameObject : Scene::_gameObjects) {        
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

    return renderItems;
}


//                   //
//      Players      //
//                   //

int Scene::GetPlayerCount() {
    return _players.size();
}

Player* Scene::GetPlayerByIndex(int index) {
    if (index >= 0 && index < _players.size()) {
        return &_players[index];
    }
    else {
        std::cout << "Scene::GetPlayerByIndex(int index) failed because index was out of range. Size of _players is " << GetPlayerCount() << "\n";
        return nullptr;
    }
}



void Scene::EvaluateDebugKeyPresses() {

    // Debug test spawn logic (respawns at same pos/rot)
    if (Input::KeyPressed(HELL_KEY_J) && false) {
        Scene::_players[0].Respawn();
        Scene::_players[1].Respawn();
    }
    // Respawn all players at random location
    if (Input::KeyPressed(HELL_KEY_K) && false) {

        if (Scene::_players[0]._isDead) {
            Scene::_players[0].Respawn();
        }
        if (Scene::_players[1]._isDead) {
            Scene::_players[1].Respawn();
        }
    }
    // Spawn player 2 to middle of main room
    if (Input::KeyPressed(HELL_KEY_K)) {
        Scene::_players[1].Respawn();
        Scene::_players[1].SetPosition(glm::vec3(3.0f, 0.1f, 3.5f));
        Audio::PlayAudio("RE_Beep.wav", 0.75);
    }
    // Set spawn point
    if (Input::KeyPressed(HELL_KEY_K) && false) {
        SpawnPoint spawnPoint;
        spawnPoint.position = Scene::_players[0].GetFeetPosition();
        spawnPoint.rotation = Scene::_players[0].GetCameraRotation();
        _spawnPoints.push_back(spawnPoint);
        std::cout << "Position: " << Util::Vec3ToString(spawnPoint.position) << "\n";
        std::cout << "Rotation: " << Util::Vec3ToString(spawnPoint.rotation) << "\n";
    }
}


















float door2X = 2.05f;
int _volumetricBloodObjectsSpawnedThisFrame = 0;

void SetPlayerGroundedStates();
void ProcessBullets();



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
    

    for (RigidComponent& rigid : Scene::_players[0]._characterModel._ragdoll._rigidComponents) {
        PxShape* shape;
        rigid.pxRigidBody->getShapes(&shape, 1);
        shape->setFlag(PxShapeFlag::eVISUALIZATION, false);
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

    SetPlayerGroundedStates();
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
  
       
    for (AnimatedGameObject& animatedGameObject : _animatedGameObjects) {
        animatedGameObject.Update(deltaTime);
    }

    for (GameObject& gameObject : _gameObjects) {
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
        for (GameObject& gameObject : Scene::_gameObjects) {
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
        }
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
        }
    }
}

void Scene::SetPlayerGroundedStates() {
    for (Player& player : Scene::_players) {
        player._isGrounded = false;
        for (auto& report : Physics::_characterCollisionReports) {
            if (report.characterController == player._characterController && report.hitNormal.y > 0.5f) {
                player._isGrounded = true;
            }
        }
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
                                if (parentPlayerHit != &Scene::_players[0]) {
                                    Scene::_players[0]._killCount++;

                                    /*glm::vec3 deathPosition = {parentPlayerHit->GetFeetPosition().x, 0.1, parentPlayerHit->GetFeetPosition().z};
                                    deathPosition += (Scene::_players[0]._movementVector * 0.25f);
                                    AnimatedGameObject* dyingGuy = Scene::GetAnimatedGameObjectByName("DyingGuy");
                                    dyingGuy->SetRotationY(Scene::_players[0].GetViewRotation().y);
                                    dyingGuy->SetPosition(deathPosition);
                                    dyingGuy->PlayAnimation("DyingGuy_Death", 1.0f);*/
                                }
                                if (parentPlayerHit != &Scene::_players[1]) {
                                    Scene::_players[1]._killCount++;
                                    /*
                                    glm::vec3 deathPosition = { parentPlayerHit->GetFeetPosition().x, 0.1, parentPlayerHit->GetFeetPosition().z };
                                    deathPosition += (Scene::_players[1]._movementVector * 0.25f);
                                    AnimatedGameObject* dyingGuy = Scene::GetAnimatedGameObjectByName("DyingGuy");
                                    dyingGuy->SetRotationY(Scene::_players[1].GetViewRotation().y);
                                    dyingGuy->SetPosition(deathPosition);
                                    dyingGuy->PlayAnimation("DyingGuy_Death", 1.0f);*/
                                }

                                for (RigidComponent& rigidComponent : Scene::_players[1]._characterModel._ragdoll._rigidComponents) {
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
					Decal decal(localPosition, localNormal, parent, Decal::Type::GLASS);
                    Scene::_decals.push_back(decal);

					// Back glass bullet decal
					localNormal = glm::inverse(parentMatrix) * glm::vec4(rayResult.surfaceNormal * glm::vec3(-1) - (rayResult.surfaceNormal * glm::vec3(0.001)), 0.0);
					Decal decal2(localPosition, localNormal, parent, Decal::Type::GLASS);
                    Scene::_decals.push_back(decal2);

					// Glass projectile
					for (int i = 0; i < 2; i++) {
						Transform transform;
						if (i == 1) {
							transform.position = rayResult.hitPosition + (rayResult.surfaceNormal * glm::vec3(-0.13));
						}
						else {
							transform.position = rayResult.hitPosition + (rayResult.surfaceNormal * glm::vec3(0.03));
						}
                        // this code below is for the ugly as fuck glass shards
                        // come up with something better pls
						/*PhysicsFilterData filterData;
						filterData.raycastGroup = RaycastGroup::RAYCAST_DISABLED;
						filterData.collisionGroup = CollisionGroup::NO_COLLISION;
						filterData.collidesWith = CollisionGroup::ENVIROMENT_OBSTACLE;
						PxShape* shape = Physics::CreateBoxShape(0.008f, 0.008f, 0.008f);
						PxRigidDynamic* body = Physics::CreateRigidDynamic(transform, filterData, shape);
						glm::vec3 forceGLM = -rayResult.rayDirection;
						if (i == 1) {
							forceGLM *= glm::vec3(-1);
						}
						forceGLM.x += Util::RandomFloat(0, 0.5f);
						forceGLM.y += Util::RandomFloat(0, 0.5f);
						forceGLM.z += Util::RandomFloat(0, 0.5f);
						PxVec3 force = Util::GlmVec3toPxVec3(forceGLM) * 0.001f;;
						body->addForce(force);
						body->setAngularVelocity(PxVec3(Util::RandomFloat(0.0f, 50.0f), Util::RandomFloat(0.0f, 50.0f), Util::RandomFloat(0.0f, 50.0f)));
					    

						BulletCasing bulletCasing;
						bulletCasing.type = MP7;
						bulletCasing.rigidBody = body;
						Scene::_bulletCasings.push_back(bulletCasing);
						//std::cout << "shard spawned\n";
                        */
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
                        Decal decal(localPosition, localNormal, parent, Decal::Type::REGULAR);
                        Scene::_decals.push_back(decal);
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
        /*
        PhysicsFilterData magFilterData;
        magFilterData.raycastGroup = RAYCAST_DISABLED;
        magFilterData.collisionGroup = CollisionGroup::GENERIC_BOUNCEABLE;
        magFilterData.collidesWith = CollisionGroup(ENVIROMENT_OBSTACLE | GENERIC_BOUNCEABLE);
        float magDensity = 750.0f;
        GameObject& mag2 = Scene::_gameObjects.emplace_back();
        mag2.SetModel("AKS74UMag");
        mag2.SetName("AKS74UMag_TEST");
        mag2.SetMeshMaterial("AKS74U_3");
        mag2.SetPosition(3.8f, 5.7f, 3.75f);
        mag2.CreateRigidBody(mag2._transform.to_mat4(), false);
        mag2.SetRaycastShapeFromModel(AssetManager::GetModel("AKS74UMag"));
        mag2.AddCollisionShapeFromConvexMesh(&AssetManager::GetModel("AKS74UMag_ConvexMesh")->_meshes[0], magFilterData, glm::vec3(1));
        mag2.SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
        mag2.UpdateRigidBodyMassAndInertia(magDensity);
        mag2.CreateEditorPhysicsObject();

        GameObject& mag = Scene::_gameObjects.emplace_back();
        mag.SetModel("AKS74UMag");
        mag.SetName("AKS74UMag_TEST2");
        mag.SetMeshMaterial("AKS74U_3");
        mag.SetPosition(4.0f, 5.7f, 3.75f);
        mag.CreateRigidBody(mag._transform.to_mat4(), false);
        mag.SetRaycastShapeFromModel(AssetManager::GetModel("AKS74UMag"));
        mag.AddCollisionShapeFromConvexMesh(&AssetManager::GetModel("AKS74UMag_ConvexMesh")->_meshes[0], magFilterData, glm::vec3(1));
        mag.SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
        mag.UpdateRigidBodyMassAndInertia(magDensity);
        mag.CreateEditorPhysicsObject();

        GameObject& mag3 = Scene::_gameObjects.emplace_back();
        mag3.SetModel("AKS74UMag");
        mag3.SetName("TEST_MAG");
        mag3.SetMeshMaterial("AKS74U_3");
        mag3.SetPosition(4.0f, 5.7f, 3.75f);
        mag3.CreateRigidBody(mag3._transform.to_mat4(), false);
        mag3.SetRaycastShapeFromModel(AssetManager::GetModel("AKS74UMag"));
        mag3.AddCollisionShapeFromConvexMesh(&AssetManager::GetModel("AKS74UMag_ConvexMesh")->_meshes[0], magFilterData, glm::vec3(1));
        mag3.SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
        mag3.UpdateRigidBodyMassAndInertia(magDensity);
        mag3.CreateEditorPhysicsObject();
        */


	/*	GameObject& mag2 = _gameObjects.emplace_back();
        mag2.SetPosition(3.8f, 5.7f, 3.75f);
        mag2.SetRotationX(-1.7f);
        mag2.SetRotationY(0.0f);
        mag2.SetRotationZ(-1.6f);
        mag2.SetScale(1.00f);
        mag2.SetModel("AKS74UMag");
        mag2.SetName("TEST_MAG");
        mag2.SetMeshMaterial("AKS74U_3");

        PhysicsFilterData magFilterData;
        magFilterData.raycastGroup = RAYCAST_DISABLED;
        magFilterData.collisionGroup = CollisionGroup::GENERIC_BOUNCEABLE;
        magFilterData.collidesWith = CollisionGroup(ENVIROMENT_OBSTACLE | GENERIC_BOUNCEABLE);
        float magDensity = 750.0f;

        mag2.CreateRigidBody(mag2._transform.to_mat4(), false);
        mag2.SetRaycastShapeFromModel(AssetManager::GetModel("AKS74UMag"));
        mag2.AddCollisionShapeFromConvexMesh(&AssetManager::GetModel("AKS74UMag_ConvexMesh")->_meshes[0], magFilterData, glm::vec3(1));
        mag2.SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
        mag2.UpdateRigidBodyMassAndInertia(magDensity);
        mag2.CreateEditorPhysicsObject();*/

     //   mag2.CreateRigidBody(mag.GetGameWorldMatrix(), false);
      //  mag2.SetRaycastShapeFromModel(AssetManager::GetModel("AKS74UMag"));
       // mag2.AddCollisionShapeFromConvexMesh(&AssetManager::GetModel("AKS74UMag_ConvexMesh")->_meshes[0], magFilterData);
       // mag2.SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
       /// mag2.UpdateRigidBodyMassAndInertia(magDensity);


        /*
		GameObject& mag = _gameObjects.emplace_back();
        mag.SetPosition(3.8f, 0.7f, 3.75f);
        mag.SetRotationX(-1.7f);
        mag.SetRotationY(0.0f);
        mag.SetRotationZ(-1.6f);
        mag.SetModel("AKS74UMag");
        mag.SetName("AKS74UMag");
        mag.SetMeshMaterial("AKS74U_3");
        mag.CreateRigidBody(mag.GetGameWorldMatrix(), false);
        mag.SetRaycastShapeFromModel(AssetManager::GetModel("AKS74UMag"));
        mag.AddCollisionShapeFromConvexMesh(&AssetManager::GetModel("AKS74UMag_ConvexMesh")->_meshes[0], magFilterData);
        mag.SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
        mag.UpdateRigidBodyMassAndInertia(magDensity);
        */

        PhysicsFilterData akFilterData;
        akFilterData.raycastGroup = RAYCAST_ENABLED;
        akFilterData.collisionGroup = CollisionGroup::GENERIC_BOUNCEABLE;
        akFilterData.collidesWith = CollisionGroup(ENVIROMENT_OBSTACLE | GENERIC_BOUNCEABLE);

        GameObject& aks74u = _gameObjects.emplace_back();
        aks74u.SetPosition(1.8f, 1.7f, 0.75f);
        aks74u.SetRotationX(-1.7f);
        aks74u.SetRotationY(0.0f);
        aks74u.SetRotationZ(-1.6f);
        //aks74u.SetModel("AKS74U_Carlos_ConvexMesh");
        aks74u.SetModel("AKS74U_Carlos");
        aks74u.SetName("AKS74U_Carlos");
        aks74u.SetMeshMaterial("Ceiling");
        aks74u.SetMeshMaterialByMeshName("FrontSight_low", "AKS74U_0");
        aks74u.SetMeshMaterialByMeshName("Receiver_low", "AKS74U_1");
        aks74u.SetMeshMaterialByMeshName("BoltCarrier_low", "AKS74U_1");
        aks74u.SetMeshMaterialByMeshName("SafetySwitch_low", "AKS74U_1");
        aks74u.SetMeshMaterialByMeshName("Pistol_low", "AKS74U_2");
        aks74u.SetMeshMaterialByMeshName("Trigger_low", "AKS74U_2");
        aks74u.SetMeshMaterialByMeshName("MagRelease_low", "AKS74U_2");
        aks74u.SetMeshMaterialByMeshName("Magazine_Housing_low", "AKS74U_3");
        aks74u.SetMeshMaterialByMeshName("BarrelTip_low", "AKS74U_4");
        aks74u.SetPickUpType(PickUpType::AKS74U);
        aks74u.SetWakeOnStart(true);


        // physics shit for ak weapon pickup
        PhysicsFilterData filterData666;
        filterData666.raycastGroup = RAYCAST_DISABLED;
        filterData666.collisionGroup = CollisionGroup::GENERIC_BOUNCEABLE;
        filterData666.collidesWith = (CollisionGroup)(ENVIROMENT_OBSTACLE | GENERIC_BOUNCEABLE);
        //aks74u.CreateRigidBody(aks74u._transform.to_mat4(), false);
        aks74u.SetKinematic(false);
        aks74u.AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("AKS74U_Carlos_ConvexMesh"), filterData666); 
        aks74u.SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("AKS74U_Carlos"));
        aks74u.SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
        aks74u.UpdateRigidBodyMassAndInertia(50.0f);



        GameObject& shotgunPickup = _gameObjects.emplace_back();
        //shotgunPickup.SetPosition(0.2f, 0.65f, 2.1f);        
        //shotgunPickup.SetRotationX(-1.55f);
        //shotgunPickup.SetRotationY(0.2f);
        //shotgunPickup.SetRotationZ(0.175f + HELL_PI);

        shotgunPickup.SetPosition(11.07, 0.65f, 4.025f);
        shotgunPickup.SetRotationX(1.5916);
        shotgunPickup.SetRotationY(3.4f);
        shotgunPickup.SetRotationZ(-0.22);

        shotgunPickup.SetModel("Shotgun_Isolated");
        shotgunPickup.SetName("Shotgun_Pickup");
        shotgunPickup.SetMeshMaterial("Shotgun");
        shotgunPickup.SetPickUpType(PickUpType::SHOTGUN);
        //shotgunPickup.CreateRigidBody(shotgunPickup._transform.to_mat4(), false);
        shotgunPickup.SetKinematic(false);
        shotgunPickup.AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("Shotgun_Isolated_ConvexMesh"), filterData666);
        shotgunPickup.SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("Shotgun_Isolated"));
        shotgunPickup.SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
        shotgunPickup.UpdateRigidBodyMassAndInertia(75.0f);
        shotgunPickup.PutRigidBodyToSleep();



        GameObject& scopePickUp = _gameObjects.emplace_back();
        scopePickUp.SetPosition(9.07f, 0.979f, 7.96f);
        //scopePickUp.SetRotationX(-1.55f);
        //scopePickUp.SetRotationY(0.2f);
        //scopePickUp.SetRotationZ(0.175f + HELL_PI);
        scopePickUp.SetModel("ScopePickUp");
        scopePickUp.SetName("ScopePickUp");
        scopePickUp.SetMeshMaterial("Shotgun");
        scopePickUp.SetPickUpType(PickUpType::AKS74U_SCOPE);
        scopePickUp.SetKinematic(false);
        scopePickUp.AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("ScopePickUp_ConvexMesh"), filterData666);
        scopePickUp.SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("ScopePickUp"));
        scopePickUp.SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
        scopePickUp.UpdateRigidBodyMassAndInertia(750.0f);
        scopePickUp.PutRigidBodyToSleep();

        GameObject& glockAmmo = _gameObjects.emplace_back();
        glockAmmo.SetPosition(0.40f, 0.78f, 4.45f);
        glockAmmo.SetRotationY(HELL_PI * 0.4f);
        glockAmmo.SetModel("GlockAmmoBox");
        glockAmmo.SetName("GlockAmmo_PickUp");
        glockAmmo.SetMeshMaterial("GlockAmmoBox");
        glockAmmo.SetPickUpType(PickUpType::GLOCK_AMMO);
        glockAmmo.SetKinematic(false);
        glockAmmo.AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("GlockAmmoBox_ConvexMesh"), filterData666);
        glockAmmo.SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("GlockAmmoBox_ConvexMesh"));
        glockAmmo.SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
        glockAmmo.UpdateRigidBodyMassAndInertia(150.0f);
        glockAmmo.PutRigidBodyToSleep();

        GameObject& pictureFrame = _gameObjects.emplace_back();
        pictureFrame.SetPosition(0.1f, 1.5f, 2.5f);
        pictureFrame.SetScale(0.01f);
        pictureFrame.SetRotationY(HELL_PI / 2);
		pictureFrame.SetModel("PictureFrame_1");
		pictureFrame.SetMeshMaterial("LongFrame");
		pictureFrame.SetName("PictureFrame");

        float cushionHeight = 0.555f;
        Transform shapeOffset;
        shapeOffset.position.y = cushionHeight * 0.5f;
        shapeOffset.position.z = 0.5f;
        PxShape* sofaShapeBigCube = Physics::CreateBoxShape(1, cushionHeight * 0.5f, 0.4f, shapeOffset);
        PhysicsFilterData sofaFilterData;
        sofaFilterData.raycastGroup = RAYCAST_DISABLED;
        sofaFilterData.collisionGroup = CollisionGroup::ENVIROMENT_OBSTACLE;
        sofaFilterData.collidesWith = (CollisionGroup)(GENERIC_BOUNCEABLE | BULLET_CASING | PLAYER | RAGDOLL);

        GameObject& sofa = _gameObjects.emplace_back();
        sofa.SetPosition(2.0f, 0.1f, 0.1f);
        sofa.SetName("Sofa");
        sofa.SetModel("Sofa_Cushionless");
        sofa.SetMeshMaterial("Sofa");
        sofa.SetKinematic(true);
        sofa.SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("Sofa_Cushionless"));
        sofa.AddCollisionShape(sofaShapeBigCube, sofaFilterData);
        sofa.AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaBack_ConvexMesh"), filterData666);
        sofa.AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaLeftArm_ConvexMesh"), filterData666);
        sofa.AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaRightArm_ConvexMesh"), filterData666);
        sofa.SetModelMatrixMode(ModelMatrixMode::GAME_TRANSFORM);

        PhysicsFilterData cushionFilterData;
        cushionFilterData.raycastGroup = RAYCAST_DISABLED;
        cushionFilterData.collisionGroup = CollisionGroup::GENERIC_BOUNCEABLE;
        cushionFilterData.collidesWith = CollisionGroup(ENVIROMENT_OBSTACLE | GENERIC_BOUNCEABLE);
        float cushionDensity = 20.0f;

        GameObject& cushion0 = _gameObjects.emplace_back();
        cushion0.SetPosition(2.0f, 0.1f, 0.1f);
        cushion0.SetModel("SofaCushion0");
        cushion0.SetMeshMaterial("Sofa");
        cushion0.SetName("SofaCushion0");
        cushion0.SetKinematic(false);
        cushion0.SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaCushion0"));
        cushion0.AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaCushion0_ConvexMesh"), filterData666);
        cushion0.SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
        cushion0.UpdateRigidBodyMassAndInertia(cushionDensity);

        GameObject& cushion1 = _gameObjects.emplace_back();
        cushion1.SetPosition(2.0f, 0.1f, 0.1f);
        cushion1.SetModel("SofaCushion1");
        cushion1.SetName("SofaCushion1");
        cushion1.SetMeshMaterial("Sofa");
        cushion1.SetKinematic(false);
        cushion1.SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaCushion0"));
        cushion1.AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaCushion1_ConvexMesh"), filterData666);
        cushion1.SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
        cushion1.UpdateRigidBodyMassAndInertia(cushionDensity);

        GameObject& cushion2 = _gameObjects.emplace_back();
        cushion2.SetPosition(2.0f, 0.1f, 0.1f);
        cushion2.SetModel("SofaCushion2");
        cushion2.SetName("SofaCushion2");
        cushion2.SetMeshMaterial("Sofa");
        cushion2.SetKinematic(false);
        cushion2.SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaCushion2"));
        cushion2.AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaCushion2_ConvexMesh"), filterData666);
        cushion2.SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
        cushion2.UpdateRigidBodyMassAndInertia(cushionDensity);

        GameObject& cushion3 = _gameObjects.emplace_back();
        cushion3.SetPosition(2.0f, 0.1f, 0.1f);
        cushion3.SetModel("SofaCushion3");
        cushion3.SetName("SofaCushion3");
        cushion3.SetMeshMaterial("Sofa");
        cushion3.SetKinematic(false);
        cushion3.SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaCushion3"));
        cushion3.AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaCushion3_ConvexMesh"), filterData666);
        cushion3.SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
        cushion3.UpdateRigidBodyMassAndInertia(cushionDensity);

        GameObject& cushion4 = _gameObjects.emplace_back();
        cushion4.SetPosition(2.0f, 0.1f, 0.1f);
        cushion4.SetModel("SofaCushion4");
        cushion4.SetName("SofaCushion4");
        cushion4.SetMeshMaterial("Sofa");
        cushion4.SetKinematic(false);
        cushion4.SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaCushion4"));
        cushion4.AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaCushion4_ConvexMesh"), filterData666);
        cushion4.SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
        cushion4.UpdateRigidBodyMassAndInertia(15.0f);

        GameObject& tree = _gameObjects.emplace_back();
        tree.SetPosition(0.75f, 0.1f, 6.2f);
        tree.SetModel("ChristmasTree");
        tree.SetName("ChristmasTree");
        tree.SetMeshMaterial("Tree");
        tree.SetMeshMaterialByMeshName("Balls", "Gold");



        {
            PhysicsFilterData filterData;
            filterData.raycastGroup = RAYCAST_ENABLED;
            filterData.collisionGroup = CollisionGroup::NO_COLLISION;
            filterData.collidesWith = CollisionGroup::NO_COLLISION;



            GameObject& smallChestOfDrawers = _gameObjects.emplace_back();
            smallChestOfDrawers.SetModel("SmallChestOfDrawersFrame");
            smallChestOfDrawers.SetMeshMaterial("Drawers");
            smallChestOfDrawers.SetName("SmallDrawersHis");
            smallChestOfDrawers.SetPosition(0.1f, 0.1f, 4.45f);
            smallChestOfDrawers.SetRotationY(NOOSE_PI / 2);
            smallChestOfDrawers.SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallChestOfDrawersFrame"));
            smallChestOfDrawers.SetOpenState(OpenState::NONE, 0, 0, 0);
            smallChestOfDrawers.SetAudioOnOpen("DrawerOpen.wav", 1.0f);
            smallChestOfDrawers.SetAudioOnClose("DrawerOpen.wav", 1.0f);

            PhysicsFilterData filterData3;
            filterData3.raycastGroup = RAYCAST_DISABLED;
            filterData3.collisionGroup = CollisionGroup::ENVIROMENT_OBSTACLE;
            filterData3.collidesWith = CollisionGroup(GENERIC_BOUNCEABLE | BULLET_CASING | PLAYER | RAGDOLL);
            smallChestOfDrawers.SetKinematic(true);
            smallChestOfDrawers.AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallChestOfDrawersFrame_ConvexMesh"), filterData3);
            smallChestOfDrawers.AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallChestOfDrawersFrame_ConvexMesh1"), filterData3);
            smallChestOfDrawers.AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallChestOfDrawersFrameLeftSide_ConvexMesh"), filterData3);
            smallChestOfDrawers.AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallChestOfDrawersFrameRightSide_ConvexMesh"), filterData3);

            GameObject& smallChestOfDrawers2 = _gameObjects.emplace_back();
            smallChestOfDrawers2.SetModel("SmallChestOfDrawersFrame");
            smallChestOfDrawers2.SetMeshMaterial("Drawers");
            smallChestOfDrawers2.SetName("SmallDrawersHers");
            smallChestOfDrawers2.SetPosition(8.9, 0.1f, 8.3f);
            smallChestOfDrawers2.SetRotationY(NOOSE_PI);
            smallChestOfDrawers2.SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallChestOfDrawersFrame"));
            smallChestOfDrawers2.SetOpenState(OpenState::NONE, 0, 0, 0);
            smallChestOfDrawers2.SetAudioOnOpen("DrawerOpen.wav", 1.0f);
            smallChestOfDrawers2.SetAudioOnClose("DrawerOpen.wav", 1.0f);
            smallChestOfDrawers2.SetKinematic(true);
            smallChestOfDrawers2.AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallChestOfDrawersFrame_ConvexMesh"), filterData3);
            smallChestOfDrawers2.AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallChestOfDrawersFrame_ConvexMesh1"), filterData3);
            smallChestOfDrawers2.AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallChestOfDrawersFrameLeftSide_ConvexMesh"), filterData3);
            smallChestOfDrawers2.AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallChestOfDrawersFrameRightSide_ConvexMesh"), filterData3);
            

            PhysicsFilterData filterData4;
            filterData4.raycastGroup = RAYCAST_DISABLED;
            filterData4.collisionGroup = CollisionGroup::ENVIROMENT_OBSTACLE;
            filterData4.collidesWith = CollisionGroup(PLAYER);
           // smallChestOfDrawers.AddCollisionShapeFromConvexMesh(&AssetManager::GetModel("SmallChestOfDrawersFrameFrontSide_ConvexMesh")->_meshes[0], filterData4);


            PhysicsFilterData filterData2;
            filterData2.raycastGroup = RAYCAST_DISABLED;
            filterData2.collisionGroup = CollisionGroup::GENERIC_BOUNCEABLE;
            filterData2.collidesWith = CollisionGroup(ENVIROMENT_OBSTACLE | GENERIC_BOUNCEABLE);

            GameObject& lamp2 = _gameObjects.emplace_back();
          //  lamp.SetModel("LampFullNoGlobe");
            lamp2.SetModel("Lamp");
            lamp2.SetName("Lamp");
            lamp2.SetMeshMaterial("Lamp");
            lamp2.SetPosition(glm::vec3(0.25f, 0.88, 0.105f) + glm::vec3(0.1f, 0.1f, 4.45f));
            lamp2.SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("LampFull"));
            lamp2.SetKinematic(false);
            lamp2.AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("LampConvexMesh_0"), filterData666);
            lamp2.AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("LampConvexMesh_1"), filterData666);
            lamp2.AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("LampConvexMesh_2"), filterData666);
            lamp2.SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
            lamp2.UpdateRigidBodyMassAndInertia(20.0f);

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


            GameObject& smallChestOfDrawer_1 = _gameObjects.emplace_back();
            smallChestOfDrawer_1.SetModel("SmallDrawerTop");
            smallChestOfDrawer_1.SetMeshMaterial("Drawers");
            smallChestOfDrawer_1.SetParentName("SmallDrawersHis");
            smallChestOfDrawer_1.SetName("TopDraw");
            smallChestOfDrawer_1.SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f);
            smallChestOfDrawer_1.SetOpenAxis(OpenAxis::TRANSLATE_Z);
            smallChestOfDrawer_1.SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallDrawerTop"));
            smallChestOfDrawer_1.SetKinematic(true);
            smallChestOfDrawer_1.AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallDrawerTop_ConvexMesh0"), filterData666);
            smallChestOfDrawer_1.AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallDrawerTop_ConvexMesh1"), filterData666);
            smallChestOfDrawer_1.AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallDrawerTop_ConvexMesh2"), filterData666);
            smallChestOfDrawer_1.AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallDrawerTop_ConvexMesh3"), filterData666);
            smallChestOfDrawer_1.AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallDrawerTop_ConvexMesh4"), filterData666);


            GameObject& smallChestOfDrawer_2 = _gameObjects.emplace_back();
            smallChestOfDrawer_2.SetModel("SmallDrawerSecond");
            smallChestOfDrawer_2.SetMeshMaterial("Drawers");
            smallChestOfDrawer_2.SetParentName("SmallDrawersHis");
			smallChestOfDrawer_2.SetName("SecondDraw");
			smallChestOfDrawer_2.SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f);
            smallChestOfDrawer_2.SetOpenAxis(OpenAxis::TRANSLATE_Z);
            smallChestOfDrawer_2.SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallDrawerSecond"));

            GameObject& smallChestOfDrawer_3 = _gameObjects.emplace_back();
            smallChestOfDrawer_3.SetModel("SmallDrawerThird");
            smallChestOfDrawer_3.SetMeshMaterial("Drawers");
			smallChestOfDrawer_3.SetParentName("SmallDrawersHis");
			smallChestOfDrawer_3.SetName("ThirdDraw");
			smallChestOfDrawer_3.SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f);
            smallChestOfDrawer_3.SetOpenAxis(OpenAxis::TRANSLATE_Z);
            smallChestOfDrawer_3.SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallDrawerThird"));

            GameObject& smallChestOfDrawer_4 = _gameObjects.emplace_back();
            smallChestOfDrawer_4.SetModel("SmallDrawerFourth");
            smallChestOfDrawer_4.SetMeshMaterial("Drawers");
			smallChestOfDrawer_4.SetParentName("SmallDrawersHis");
			smallChestOfDrawer_4.SetName("ForthDraw");
			smallChestOfDrawer_4.SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f);
            smallChestOfDrawer_4.SetOpenAxis(OpenAxis::TRANSLATE_Z);
            smallChestOfDrawer_4.SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("SmallDrawerFourth"));
        }



        for (int y = 0; y < 12; y++) {

            GameObject* cube = &_gameObjects.emplace_back();
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
        }

    }

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
    
    if (false) {
        AnimatedGameObject& aks = _animatedGameObjects.emplace_back(AnimatedGameObject());
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
    }

    /////////////////////
    //                 //
    //      GLOCK      //

    if (false) {
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
    }

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
    for (GameObject& gameObject : _gameObjects) {
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

void Scene::CreatePlayers() {
    _players.clear();
    _players.push_back(Player());
	if (EngineState::GetPlayerCount() == 2) {
		_players.push_back(Player());
    }

    _players[0]._keyboardIndex = 0;
    _players[1]._keyboardIndex = 1;
    _players[0]._mouseIndex = 0;
    _players[1]._mouseIndex = 1;

    PxU32 p1RagdollCollisionGroupFlags = RaycastGroup::PLAYER_1_RAGDOLL;
    PxU32 p2RagdollCollisionGroupFlags = RaycastGroup::PLAYER_2_RAGDOLL;

    _players[0]._characterModel.LoadRagdoll("UnisexGuy3.rag", p1RagdollCollisionGroupFlags);
    _players[1]._characterModel.LoadRagdoll("UnisexGuy3.rag", p2RagdollCollisionGroupFlags);

    _players[0]._interactFlags = RaycastGroup::RAYCAST_ENABLED;
    _players[0]._interactFlags &= ~RaycastGroup::PLAYER_1_RAGDOLL;

    _players[1]._interactFlags = RaycastGroup::RAYCAST_ENABLED;
    _players[1]._interactFlags &= ~RaycastGroup::PLAYER_2_RAGDOLL;

    _players[0]._bulletFlags = RaycastGroup::RAYCAST_ENABLED | RaycastGroup::PLAYER_2_RAGDOLL;
    _players[1]._bulletFlags = RaycastGroup::RAYCAST_ENABLED | RaycastGroup::PLAYER_1_RAGDOLL;

    _players[0]._playerName = "Orion";
    _players[1]._playerName = "CrustyAssCracker";
}


void Scene::CleanUp() {
    for (Door& door : _doors) {
        door.CleanUp();
    }
    for (BulletCasing& bulletCasing : _bulletCasings) {
        bulletCasing.CleanUp();
    }
    for (Decal& decal : _decals) {
        decal.CleanUp();
    }
    for (GameObject& gameObject : _gameObjects) {
        gameObject.CleanUp();
    }
    for (Toilet& toilet: _toilets) {
        toilet.CleanUp();
    }
    for (Window& window : _windows) {
        window.CleanUp();
    }
    for (AnimatedGameObject& animatedGameObject : _animatedGameObjects) {
        animatedGameObject.DestroyRagdoll();
    }

    _toilets.clear();
    _bloodDecals.clear();
    _spawnPoints.clear();
    _bulletCasings.clear();
    _decals.clear();
    _walls.clear();
    _floors.clear();
	_ceilings.clear();
	_doors.clear();
    _windows.clear();
	_gameObjects.clear();
    _animatedGameObjects.clear();
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
                CloudPoint cloudPoint;
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
                CloudPoint cloudPoint;
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
                CloudPoint cloudPoint;
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

GameObject* Scene::GetGameObjectByName(std::string name) {
    if (name == "undefined") {
        return nullptr;
    }
    for (GameObject& gameObject : _gameObjects) {
        if (gameObject.GetName() == name) {
            return &gameObject;
        }
    }
    std::cout << "Scene::GetGameObjectByName() failed, no object with name \"" << name << "\"\n";
    return nullptr;
}

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
}

std::vector<AnimatedGameObject>& Scene::GetAnimatedGameObjects() {
    return _animatedGameObjects;
}

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

    std::cout << "size was: " << _decals.size() << "\n";

    for (int i = 0; i < _decals.size(); i++) {
        PxRigidBody* decalParentRigid = _decals[i].parent;
        if (decalParentRigid == (void*)window->raycastBody //||
           // decalParentRigid == (void*)window->raycastBodyTop
            ) {
            _decals.erase(_decals.begin() + i);
            i--;
            std::cout << "removed decal " << i << " size is now: " << _decals.size() << "\n";
        }
    }
}

void Scene::ProcessPhysicsCollisions() {

    /* if (Input::KeyPressed(HELL_KEY_9)) {
         for (int i = 0; i < 555; i++) {
             Audio::PlayAudio("BulletCasingBounce.wav", 1.0);
         }
     }*/
    bool playedShellSound = false;

    for (CollisionReport& report : Physics::GetCollisions()) {

        PxRigidActor* actorA = (PxRigidActor*)report.rigidA;
        PxRigidActor* actorB = (PxRigidActor*)report.rigidB;
        PxShape* shapeA;
        PxShape* shapeB;
        actorA->getShapes(&shapeA, 1);
        actorB->getShapes(&shapeB, 1);
        CollisionGroup groupA = (CollisionGroup)shapeA->getQueryFilterData().word1;
        CollisionGroup groupB = (CollisionGroup)shapeB->getQueryFilterData().word1;

        if (groupA & BULLET_CASING) {
            //BulletCasing* casing = (BulletCasing*)actorA->userData;
            //casing->CollisionResponse();
            playedShellSound = true;
        }
        else if (groupB & BULLET_CASING) {
            //BulletCasing* casing = (BulletCasing*)actorB->userData;
            //casing->CollisionResponse();
            playedShellSound = true;
        }

        if (playedShellSound) {

          //  if (actorA->userData == (void*)&CasingType::SHOTGUN_SHELL) {

            if (actorA->userData == (void*)&EngineState::weaponNamePointers[SHOTGUN]) {
                Audio::PlayAudio("ShellFloorBounce.wav", Util::RandomFloat(0.1f, 0.2f));
                break;
            }
            else {
                Audio::PlayAudio("BulletCasingBounce.wav", Util::RandomFloat(0.2f, 0.3f));
                break;
            }
        }
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

    std::vector<Triangle> triangles;

    RTMesh& mesh = Scene::_rtMesh[0]; // This is the main world
    for (unsigned int i = mesh.baseVertex; i < mesh.baseVertex + mesh.vertexCount; i += 3) {
        Triangle triangle;
        triangle.p1 = Scene::_rtVertices[i + 0];
        triangle.p2 = Scene::_rtVertices[i + 1];
        triangle.p3 = Scene::_rtVertices[i + 2];
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
    }
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
