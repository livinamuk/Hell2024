#include "Scene.h"

#include <future>
#include <thread>

#include "../Util.hpp"
#include "../EngineState.hpp"
#include "AssetManager.h"
#include "../Renderer/Renderer.h"
#include "Input.h"
#include "File.h"
 #include "Player.h"
#include "Audio.hpp"
#include "TextBlitter.h"

float door2X = 2.05f;

void SetPlayerGroundedStates();
void ProcessBullets();

void Scene::Update(float deltaTime) {

    if (Input::KeyPressed(HELL_KEY_K)) {
        SpawnPoint spawnPoint;
        spawnPoint.position = Scene::_players[0].GetFeetPosition();
        spawnPoint.rotation = Scene::_players[0].GetCameraRotation();
        _spawnPoints.push_back(spawnPoint);

        std::cout << "Position: " << Util::Vec3ToString(spawnPoint.position) << "\n";
        std::cout << "Rotation: " << Util::Vec3ToString(spawnPoint.rotation) << "\n";
    }


    if (Input::KeyPressed(HELL_KEY_1)) {
        _players[0]._keyboardIndex = 0;
        _players[1]._keyboardIndex = 1;
        _players[0]._mouseIndex = 0;
        _players[1]._mouseIndex = 1;
    }
    if (Input::KeyPressed(HELL_KEY_2)) {
        _players[1]._keyboardIndex = 0;
        _players[0]._keyboardIndex = 1;
        _players[1]._mouseIndex = 0;
        _players[0]._mouseIndex = 1;
    }

  /*  if (Input::KeyPressed(HELL_KEY_SPACE)) {

        std::cout << "\n";

        for (GameObject& gameObject : _gameObjects) {
            std::cout << gameObject.GetName() << "\n";
            for (Mesh& mesh : gameObject._model->_meshes) {
                std::cout << "-" << mesh.indices.size() << "\n";
            }
        }
    }*/

    /*
    GameObject* mag = Scene::GetGameObjectByName("AKS74UMag_TEST");
    GameObject* mag2 = Scene::GetGameObjectByName("AKS74UMag_TEST2");

    static bool test = false;
    if (Input::RightMousePressed()) {
        test = !test;
    }

    if (mag && test) {
        glm::mat4 physMatrix = Util::PxMat44ToGlmMat4(mag->_collisionBody->getGlobalPose());
        AnimatedGameObject* ak2 = Scene::GetAnimatedGameObjectByName("AKS74U_TEST");
        glm::mat4 matrix = ak2->GetBoneWorldMatrixFromBoneName("Magazine");
        if (matrix == glm::mat4(1)) {
            std::cout << "bailing\n";
            return;
        }
        glm::mat4 magWorldMatrix = ak2->GetModelMatrix() * matrix;
        Transform scaleMat;
        scaleMat.scale = glm::vec3(1.0f / ak2->GetScale().x);
        glm::vec3 pos = magWorldMatrix[3];
        glm::quat rot = glm::quat_cast(scaleMat.to_mat4() * magWorldMatrix);
        PxVec3 pxPos = Util::GlmVec3toPxVec3(pos);
        PxQuat pxRot = Util::GlmQuatToPxQuat(rot);

        PxTransform transform(pxPos, pxRot);
        mag->_collisionBody->setGlobalPose(transform);
        mag->PutRigidBodyToSleep();
        //std::cout << "\n" << "Pos: " << Util::Vec3ToString(pos) << "\n";
        //std::cout << "" << "Rot: " << Util::QuatToString(rot) << "\n";
        PxMat44 matrix2 = Util::GlmMat4ToPxMat44(magWorldMatrix);
    }



    if (mag && test && Scene::_players[0].GetCurrentWeaponIndex() == AKS74U) {

        glm::mat4 physMatrix = Util::PxMat44ToGlmMat4(mag->_collisionBody->getGlobalPose());

        AnimatedGameObject* ak2 = &Scene::_players[0].GetFirstPersonWeapon();
        glm::mat4 matrix = ak2->GetBoneWorldMatrixFromBoneName("Magazine");

        if (matrix == glm::mat4(1)) {
            std::cout << "bailing\n";
            return;
        }

        //  std::cout << "\n" << Util::Mat4ToString(matrix) << "\n";

        glm::mat4 magWorldMatrix = ak2->GetModelMatrix() * matrix;// *scaleMat.to_mat4();
        // glm::mat4 magWorldMatrix = ak2->GetModelMatrix();


        Transform scaleMat;
        //scaleMat.scale = glm::vec3(100);
        scaleMat.scale = glm::vec3(1.0f / ak2->GetScale().x);

        glm::quat rot = glm::quat_cast(scaleMat.to_mat4() * magWorldMatrix);

        Transform transform2;
        transform2.position = Scene::_players[0].GetCameraForward() * glm::vec3(-0.25f);
        magWorldMatrix = transform2.to_mat4() * magWorldMatrix;

        glm::vec3 pos = magWorldMatrix[3];


        PxVec3 pxPos = Util::GlmVec3toPxVec3(pos);
        PxQuat pxRot = Util::GlmQuatToPxQuat(rot);

        PxTransform transform(pxPos, pxRot);
        mag->_collisionBody->setGlobalPose(transform);
        mag->PutRigidBodyToSleep();


      //  std::cout << "\n" << "Pos: " << Util::Vec3ToString(pos) << "\n";
      //  std::cout << "" << "Rot: " << Util::QuatToString(rot) << "\n";

        PxMat44 matrix2 = Util::GlmMat4ToPxMat44(magWorldMatrix);
        //mag->_collisionBody->setGlobalPose(PxTransform(matrix2));

    }*/


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
		ak->_editorRaycastBody->setGlobalPose(trans);

    }

    


    SetPlayerGroundedStates();
    ProcessBullets();

    // fix this
	Renderer::_shadowMapsAreDirty = true;
	
    for (BulletCasing& bulletCasing : _bulletCasings) {
        bulletCasing.Update(deltaTime);
    }

	for (PickUp& pickUp : _pickUps) {
        pickUp.Update(deltaTime);

	}

	

    if (Input::KeyPressed(HELL_KEY_T)) {
        for (Light& light : Scene::_lights) {
            light.isDirty = true;
        }
    }
  
    // Are lights dirty? occurs when a door opens within their radius
    // Which triggers update of the point cloud, and then propagation grid

    /*for (const auto& door : _doors) {
        if (door.state != Door::State::OPENING && door.state != Door::State::CLOSING) continue;

        for (auto& light : _lights) {
            if (light.isDirty) continue;
            if (Util::DistanceSquared(door.position, light.position) < (light.radius + DOOR_WIDTH) * (light.radius + DOOR_WIDTH)) {
                light.isDirty = true;
            }
        }
    }*/
       
    for (AnimatedGameObject& animatedGameObject : _animatedGameObjects) {
        animatedGameObject.Update(deltaTime);
    }

    for (GameObject& gameObject : _gameObjects) {
        gameObject.Update(deltaTime);
    }

    //Flicker light 2
    if (_lights.size() > 2) {
        static float totalTime = 0;
        float frequency = 20.f;
        float amplitude = 0.5f;
        totalTime += deltaTime;
      //  Scene::_lights[2].strength = 1.0f + sin(totalTime * frequency) * amplitude;
    }

    // Move light 0 in a figure 8
    /*
    static bool figure8Light = false;
    if (Input::KeyPressed(HELL_KEY_P)) {
        figure8Light = !figure8Light;
		Audio::PlayAudio(AUDIO_SELECT, 1.0f);
        _lights[0].isDirty = true;
    }

    glm::vec3 lightPos = glm::vec3(2.8, 2.2, 3.6);

    Light& light = _lights[0];
    if (figure8Light) {
        static float time = 0;
        time += (deltaTime / 2);
        glm::vec3 newPos = lightPos;
        lightPos.x = lightPos.x + (cos(time)) * 2;
        lightPos.y = lightPos.y;
        lightPos.z = lightPos.z + (sin(2 * time) / 2) * 2;
        light.isDirty = true;
    }
    light.position = lightPos;
    */

    for (Door& door : _doors) {
        door.Update(deltaTime);
    }


    // Running Guy
    /*auto enemy = GetAnimatedGameObjectByName("Enemy");
    enemy->PlayAndLoopAnimation("UnisexGuyIdle", 1.0f);
    enemy->SetScale(0.00975f);
    enemy->SetScale(0.00f);
    enemy->SetPosition(glm::vec3(1.3f, 0.1, 3.5f));
    enemy->SetRotationX(HELL_PI / 2);
    enemy->SetRotationY(HELL_PI / 2);

    auto glock = GetAnimatedGameObjectByName("Shotgun");
    glock->SetScale(0.01f);
    glock->SetScale(0.00f);
    glock->SetPosition(glm::vec3(1.3f, 1.1, 3.5f));*/


    UpdateRTInstanceData();
    ProcessPhysicsCollisions();
}

void Scene::CheckIfLightsAreDirty() {
    for (Light& light : Scene::_lights) {
        light.isDirty = false;
        for (GameObject& gameObject : Scene::_gameObjects) {
            if (gameObject.HasMovedSinceLastFrame()) {
                if (Util::AABBInSphere(gameObject._aabb, light.position, light.radius)) {
                    light.isDirty = true;
                    break;
                }
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
    }
}

void Scene::Update3DEditorScene() {

	for (GameObject& gameObject : _gameObjects) {
		gameObject.UpdateEditorPhysicsObject();
	}

	Physics::GetEditorScene()->simulate(1 / 60.0f);
	Physics::GetEditorScene()->fetchResults(true);

}


void SetPlayerGroundedStates() {
    for (Player& player : Scene::_players) {
        player._isGrounded = false;
        for (auto& report : Physics::_characterCollisionReports) {
            if (report.characterController == player._characterController && report.hitNormal.y > 0.5f) {
                player._isGrounded = true;
            }
        }
    }
}

void ProcessBullets() {

    bool glassWasHit = false;
	for (int i = 0; i < Scene::_bullets.size(); i++) {
		Bullet& bullet = Scene::_bullets[i];
		PhysXRayResult rayResult = Util::CastPhysXRay(bullet.spawnPosition, bullet.direction, 1000);
		if (rayResult.hitFound) {
			PxRigidDynamic* actor = (PxRigidDynamic*)rayResult.hitActor;
			if (actor->userData) {


				PhysicsObjectData* physicsObjectData = (PhysicsObjectData*)actor->userData;
				
                if (physicsObjectData->type == RAGDOLL_RIGID) {
                    float strength = 750000;
                    if (bullet.type == SHOTGUN) {
                        strength = 200000;
                    }
                    std::cout << "you shot a ragdoll rigid\n";
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
                    Scene::_bullets.push_back(newBullet);

					// Front glass bullet decal
					PxRigidBody* parent = actor;
					glm::mat4 parentMatrix = Util::PxMat44ToGlmMat4(actor->getGlobalPose());
					glm::vec3 localPosition = glm::inverse(parentMatrix) * glm::vec4(rayResult.hitPosition + (rayResult.surfaceNormal * glm::vec3(00)), 1.0);
					glm::vec3 localNormal = glm::inverse(parentMatrix) * glm::vec4(rayResult.surfaceNormal, 0.0);
					Decal decal(localPosition, localNormal, parent, Decal::Type::GLASS);
                    Scene::_decals.push_back(decal);

					// Back glass bullet decal
					localNormal = glm::inverse(parentMatrix) * glm::vec4(rayResult.surfaceNormal * glm::vec3(-1), 0.0);
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
				else {
					// Bullet decal
					PxRigidBody* parent = actor;
					glm::mat4 parentMatrix = Util::PxMat44ToGlmMat4(actor->getGlobalPose());
					glm::vec3 localPosition = glm::inverse(parentMatrix) * glm::vec4(rayResult.hitPosition, 1.0);
					glm::vec3 localNormal = glm::inverse(parentMatrix) * glm::vec4(rayResult.surfaceNormal, 0.0);
					Decal decal(localPosition, localNormal, parent, Decal::Type::REGULAR);
                    Scene::_decals.push_back(decal);
				}
			}
		}
	}
    Scene::_bullets.clear();
    if (glassWasHit) {
        Audio::PlayAudio("GlassImpact.wav", 3.0f);
    }
}
void Scene::LoadHardCodedObjects() {

/*PickUp& glockAmmoA = _pickUps.emplace_back();
	//glockAmmoA.position = glm::vec3(2.0f, 0.1f, 3.6f);
	glockAmmoA.position = glm::vec3(0.0f, 0.676f, 0.3f);
    glockAmmoA.rotation.y = HELL_PI * 0.4f;
    glockAmmoA.type = PickUp::Type::GLOCK_AMMO;
    glockAmmoA.parentGameObjectName = "TopDraw";
    */
    
   // _ceilings.emplace_back(door2X - 0.8f, 7.0f, door2X + 0.8f, 9.95f, 2.5f, AssetManager::GetMaterialIndex("Ceiling"));

    // ceilings   

    _ceilings.emplace_back(0.1f, 0.1f, 5.2f, 3.1f, 2.5f, AssetManager::GetMaterialIndex("Ceiling"));
    _ceilings.emplace_back(0.1f, 4.1f, 6.1f, 6.9f, 2.5f, AssetManager::GetMaterialIndex("Ceiling"));
    _ceilings.emplace_back(0.1f, 3.1f, 3.7f, 4.1f, 2.5f, AssetManager::GetMaterialIndex("Ceiling"));
    _ceilings.emplace_back(4.7f, 3.1f, 6.1f, 4.1f, 2.5f, AssetManager::GetMaterialIndex("Ceiling"));
 //   _ceilings.emplace_back(5.3f, 0.1f, 11.3f, 3.0f, 2.5f, AssetManager::GetMaterialIndex("Ceiling"));

    for (Floor& floor : Scene::_floors) {
        float minX = std::min(std::min(std::min(floor.v1.position.x, floor.v2.position.x), floor.v3.position.x), floor.v4.position.x);
        float maxX = std::max(std::max(std::max(floor.v1.position.x, floor.v2.position.x), floor.v3.position.x), floor.v4.position.x);
        float minZ = std::min(std::min(std::min(floor.v1.position.z, floor.v2.position.z), floor.v3.position.z), floor.v4.position.z);
        float maxZ = std::max(std::max(std::max(floor.v1.position.z, floor.v2.position.z), floor.v3.position.z), floor.v4.position.z);
        _ceilings.emplace_back(minX, minZ, maxX, maxZ, 2.5f, AssetManager::GetMaterialIndex("Ceiling"));
    }


   // std::cout << Scene::_floors.size() << " FLOOOOOOOOOOORS\n";

   // LoadLightSetup(2);

    /*
	GameObject& shard = _gameObjects.emplace_back();
    shard.SetPosition(3.8f, 1.4f, 3.75f);
    shard.SetModel("GlassShard");
    shard.SetScale(100.0f);
    shard.SetMeshMaterial("BulletHole_Glass");

    */


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

        // physics shit for ak weapon pickup
        PhysicsFilterData filterData666;
        filterData666.raycastGroup = RAYCAST_DISABLED;
        filterData666.collisionGroup = CollisionGroup::GENERIC_BOUNCEABLE;
        filterData666.collidesWith = (CollisionGroup)(ENVIROMENT_OBSTACLE | GENERIC_BOUNCEABLE);
        aks74u.CreateRigidBody(aks74u._transform.to_mat4(), false);
        aks74u.AddCollisionShapeFromConvexMesh(&AssetManager::GetModel("AKS74U_Carlos_ConvexMesh")->_meshes[0], filterData666);
        aks74u.SetRaycastShapeFromModel(AssetManager::GetModel("AKS74U_Carlos"));
        aks74u.SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
        aks74u.UpdateRigidBodyMassAndInertia(50.0f);



        GameObject& shotgunPickup = _gameObjects.emplace_back();
        shotgunPickup.SetPosition(0.2f, 0.65f, 2.1f);
        shotgunPickup.SetRotationX(-1.55f);
        shotgunPickup.SetRotationY(0.2f);
        shotgunPickup.SetRotationZ(0.175f + HELL_PI);
        shotgunPickup.SetModel("Shotgun_Isolated");
        shotgunPickup.SetName("Shotgun_Pickup");
        shotgunPickup.SetMeshMaterial("Shotgun");
        shotgunPickup.SetPickUpType(PickUpType::SHOTGUN);
        shotgunPickup.CreateRigidBody(shotgunPickup._transform.to_mat4(), false);
        shotgunPickup.AddCollisionShapeFromConvexMesh(&AssetManager::GetModel("Shotgun_Isolated_ConvexMesh")->_meshes[0], filterData666);
        shotgunPickup.SetRaycastShapeFromModel(AssetManager::GetModel("Shotgun_Isolated"));
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
        scopePickUp.CreateRigidBody(scopePickUp._transform.to_mat4(), false);
        scopePickUp.AddCollisionShapeFromConvexMesh(&AssetManager::GetModel("ScopePickUp_ConvexMesh")->_meshes[0], filterData666);
        scopePickUp.SetRaycastShapeFromModel(AssetManager::GetModel("ScopePickUp"));
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
        glockAmmo.CreateRigidBody(glockAmmo._transform.to_mat4(), false);
        glockAmmo.AddCollisionShapeFromConvexMesh(&AssetManager::GetModel("GlockAmmoBox_ConvexMesh")->_meshes[0], filterData666);
        glockAmmo.SetRaycastShapeFromModel(AssetManager::GetModel("GlockAmmoBox_ConvexMesh"));
        glockAmmo.SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
        glockAmmo.UpdateRigidBodyMassAndInertia(150.0f);
        glockAmmo.PutRigidBodyToSleep();





        /*	PickUp& glockAmmoA = _pickUps.emplace_back();
            //glockAmmoA.position = glm::vec3(2.0f, 0.1f, 3.6f);
            glockAmmoA.position = glm::vec3(0.0f, 0.676f, 0.3f);
            glockAmmoA.rotation.y = HELL_PI * 0.4f;
            glockAmmoA.type = PickUp::Type::GLOCK_AMMO;
            glockAmmoA.parentGameObjectName = "TopDraw";







      /* GameObject& shotgunPickup = _gameObjects.emplace_back();
        shotgunPickup.SetPosition(1.8f, 1.7f, 0.75f);
        shotgunPickup.SetRotationX(-1.7f);
        shotgunPickup.SetRotationY(0.0f);
        shotgunPickup.SetRotationZ(-1.6f);
        shotgunPickup.SetModel("Shotgun_Isolated");
        shotgunPickup.SetName("Shotgun_Pickup");
        shotgunPickup.SetMeshMaterial("Glock");
        shotgunPickup.SetPickUpType(PickUpType::AKS74U);

        */



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
        sofaFilterData.collidesWith = (CollisionGroup)(GENERIC_BOUNCEABLE | BULLET_CASING | PLAYER);

        GameObject& sofa = _gameObjects.emplace_back();
        sofa.SetPosition(2.0f, 0.1f, 0.1f);
        sofa.SetName("Sofa");
        sofa.SetModel("Sofa_Cushionless");
        sofa.SetMeshMaterial("Sofa");
        sofa.CreateRigidBody(sofa.GetGameWorldMatrix(), true);
        sofa.SetRaycastShapeFromModel(AssetManager::GetModel("Sofa_Cushionless"));
        sofa.AddCollisionShape(sofaShapeBigCube, sofaFilterData);
        sofa.AddCollisionShapeFromConvexMesh(&AssetManager::GetModel("SofaBack_ConvexMesh")->_meshes[0], sofaFilterData);
        sofa.AddCollisionShapeFromConvexMesh(&AssetManager::GetModel("SofaLeftArm_ConvexMesh")->_meshes[0], sofaFilterData);
        sofa.AddCollisionShapeFromConvexMesh(&AssetManager::GetModel("SofaRightArm_ConvexMesh")->_meshes[0], sofaFilterData);
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
        cushion0.CreateRigidBody(cushion0.GetGameWorldMatrix(), false);
        cushion0.SetRaycastShapeFromModel(AssetManager::GetModel("SofaCushion0"));
        cushion0.AddCollisionShapeFromConvexMesh(&AssetManager::GetModel("SofaCushion0_ConvexMesh")->_meshes[0], cushionFilterData);
        cushion0.SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
        cushion0.UpdateRigidBodyMassAndInertia(cushionDensity);

        GameObject& cushion1 = _gameObjects.emplace_back();
        cushion1.SetPosition(2.0f, 0.1f, 0.1f);
        cushion1.SetModel("SofaCushion1");
        cushion1.SetName("SofaCushion1");
        cushion1.SetMeshMaterial("Sofa");
        cushion1.CreateRigidBody(cushion1.GetGameWorldMatrix(), false);
        cushion1.SetRaycastShapeFromModel(AssetManager::GetModel("SofaCushion1"));
        cushion1.AddCollisionShapeFromConvexMesh(&AssetManager::GetModel("SofaCushion1_ConvexMesh")->_meshes[0], cushionFilterData);
        cushion1.SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
        cushion1.UpdateRigidBodyMassAndInertia(cushionDensity);

        GameObject& cushion2 = _gameObjects.emplace_back();
        cushion2.SetPosition(2.0f, 0.1f, 0.1f);
        cushion2.SetModel("SofaCushion2");
        cushion2.SetName("SofaCushion2");
        cushion2.SetMeshMaterial("Sofa");
        cushion2.CreateRigidBody(cushion2.GetGameWorldMatrix(), false);
        cushion2.SetRaycastShapeFromModel(AssetManager::GetModel("SofaCushion2"));
        cushion2.AddCollisionShapeFromConvexMesh(&AssetManager::GetModel("SofaCushion2_ConvexMesh")->_meshes[0], cushionFilterData);
        cushion2.SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
        cushion2.UpdateRigidBodyMassAndInertia(cushionDensity);

        GameObject& cushion3 = _gameObjects.emplace_back();
        cushion3.SetPosition(2.0f, 0.1f, 0.1f);
        cushion3.SetModel("SofaCushion3");
        cushion3.SetName("SofaCushion3");
        cushion3.SetMeshMaterial("Sofa");
        cushion3.CreateRigidBody(cushion3.GetGameWorldMatrix(), false);
        cushion3.SetRaycastShapeFromModel(AssetManager::GetModel("SofaCushion3"));
        cushion3.AddCollisionShapeFromConvexMesh(&AssetManager::GetModel("SofaCushion3_ConvexMesh")->_meshes[0], cushionFilterData);
        cushion3.SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
        cushion3.UpdateRigidBodyMassAndInertia(cushionDensity);

        GameObject& cushion4 = _gameObjects.emplace_back();
        cushion4.SetPosition(2.0f, 0.1f, 0.1f);
        cushion4.SetModel("SofaCushion4");
        cushion4.SetName("SofaCushion4");
        cushion4.SetMeshMaterial("Sofa");
        cushion4.CreateRigidBody(cushion4.GetGameWorldMatrix(), false);
        cushion4.SetRaycastShapeFromModel(AssetManager::GetModel("SofaCushion4"));
        cushion4.AddCollisionShapeFromConvexMesh(&AssetManager::GetModel("SofaCushion4_ConvexMesh")->_meshes[0], cushionFilterData);
        cushion4.SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
        cushion4.UpdateRigidBodyMassAndInertia(15.0f);

        GameObject& tree = _gameObjects.emplace_back();
        tree.SetPosition(0.75f, 0.1f, 6.2f);
        tree.SetModel("ChristmasTree");
        tree.SetName("ChristmasTree");
        tree.SetMeshMaterial("Tree");

        GameObject& toilet = _gameObjects.emplace_back();
        toilet.SetPosition(11.2f, 0.1f, 3.65f);
        toilet.SetModel("Toilet");
        toilet.SetName("Toilet");
        toilet.SetMeshMaterial("Toilet");
        toilet.SetRaycastShapeFromModel(AssetManager::GetModel("Toilet"));
        toilet.SetRotationY(HELL_PI * 0.5f);


        GameObject& toiletSeat = _gameObjects.emplace_back();
        toiletSeat.SetModel("ToiletSeat");
        toiletSeat.SetPosition(0, 0.40727, -0.2014);
        toiletSeat.SetName("ToiletSeat");
        toiletSeat.SetMeshMaterial("Toilet");
        toiletSeat.SetParentName("Toilet");
        toiletSeat.SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f);
        toiletSeat.SetOpenAxis(OpenAxis::ROTATION_NEG_X);
        toiletSeat.SetRaycastShapeFromModel(AssetManager::GetModel("ToiletSeat"));
    
        GameObject& toiletLid = _gameObjects.emplace_back();
        toiletLid.SetPosition(0, 0.40727, -0.2014);
        toiletLid.SetModel("ToiletLid");
        toiletLid.SetName("ToiletLid");
        toiletLid.SetMeshMaterial("Toilet");
        toiletLid.SetParentName("Toilet");
        toiletLid.SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f);
        toiletLid.SetOpenAxis(OpenAxis::ROTATION_POS_X);
        toiletLid.SetRaycastShapeFromModel(AssetManager::GetModel("ToiletLid"));


        //tree.CreateRigidBody(sofa.GetGameWorldMatrix(), true);


        /*
        GameObject& scope = _gameObjects.emplace_back();
        scope.SetPosition(3.75f, 1.6f, 3.2f);
        scope.SetModel("ScopeACOG");
        scope.SetName("ScopeACOG");
        scope.SetMeshMaterial("Gold");
        scope.SetScale(0.01f);*/

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
            smallChestOfDrawers.SetRaycastShapeFromModel(AssetManager::GetModel("SmallChestOfDrawersFrame"));
            smallChestOfDrawers.SetOpenState(OpenState::NONE, 0, 0, 0);
            smallChestOfDrawers.SetAudioOnOpen("DrawerOpen.wav", 1.0f);
            smallChestOfDrawers.SetAudioOnClose("DrawerOpen.wav", 1.0f);

            PhysicsFilterData filterData3;
            filterData3.raycastGroup = RAYCAST_DISABLED;
            filterData3.collisionGroup = CollisionGroup::ENVIROMENT_OBSTACLE;
            filterData3.collidesWith = CollisionGroup(GENERIC_BOUNCEABLE | BULLET_CASING | PLAYER);
            smallChestOfDrawers.CreateRigidBody(smallChestOfDrawers.GetGameWorldMatrix(), true);
            smallChestOfDrawers.AddCollisionShapeFromConvexMesh(&AssetManager::GetModel("SmallChestOfDrawersFrame_ConvexMesh")->_meshes[0], filterData3);
            smallChestOfDrawers.AddCollisionShapeFromConvexMesh(&AssetManager::GetModel("SmallChestOfDrawersFrameLeftSide_ConvexMesh")->_meshes[0], filterData3);
            smallChestOfDrawers.AddCollisionShapeFromConvexMesh(&AssetManager::GetModel("SmallChestOfDrawersFrameRightSide_ConvexMesh")->_meshes[0], filterData3);


            GameObject& smallChestOfDrawers2 = _gameObjects.emplace_back();
            smallChestOfDrawers2.SetModel("SmallChestOfDrawersFrame");
            smallChestOfDrawers2.SetMeshMaterial("Drawers");
            smallChestOfDrawers2.SetName("SmallDrawersHers");
            smallChestOfDrawers2.SetPosition(8.9, 0.1f, 8.3f);
            smallChestOfDrawers2.SetRotationY(NOOSE_PI);
            smallChestOfDrawers2.SetRaycastShapeFromModel(AssetManager::GetModel("SmallChestOfDrawersFrame"));
            smallChestOfDrawers2.SetOpenState(OpenState::NONE, 0, 0, 0);
            smallChestOfDrawers2.SetAudioOnOpen("DrawerOpen.wav", 1.0f);
            smallChestOfDrawers2.SetAudioOnClose("DrawerOpen.wav", 1.0f);
            smallChestOfDrawers2.CreateRigidBody(smallChestOfDrawers2.GetGameWorldMatrix(), true);
            smallChestOfDrawers2.AddCollisionShapeFromConvexMesh(&AssetManager::GetModel("SmallChestOfDrawersFrame_ConvexMesh")->_meshes[0], filterData3);
            smallChestOfDrawers2.AddCollisionShapeFromConvexMesh(&AssetManager::GetModel("SmallChestOfDrawersFrameLeftSide_ConvexMesh")->_meshes[0], filterData3);
            smallChestOfDrawers2.AddCollisionShapeFromConvexMesh(&AssetManager::GetModel("SmallChestOfDrawersFrameRightSide_ConvexMesh")->_meshes[0], filterData3);
           


            PhysicsFilterData filterData4;
            filterData4.raycastGroup = RAYCAST_DISABLED;
            filterData4.collisionGroup = CollisionGroup::ENVIROMENT_OBSTACLE;
            filterData4.collidesWith = CollisionGroup(PLAYER);
           // smallChestOfDrawers.AddCollisionShapeFromConvexMesh(&AssetManager::GetModel("SmallChestOfDrawersFrameFrontSide_ConvexMesh")->_meshes[0], filterData4);


            PhysicsFilterData filterData2;
            filterData2.raycastGroup = RAYCAST_DISABLED;
            filterData2.collisionGroup = CollisionGroup::GENERIC_BOUNCEABLE;
            filterData2.collidesWith = CollisionGroup(ENVIROMENT_OBSTACLE | GENERIC_BOUNCEABLE);

            GameObject& lamp = _gameObjects.emplace_back();
          //  lamp.SetModel("LampFullNoGlobe");
            lamp.SetModel("Lamp");
			lamp.SetName("Lamp");
            lamp.SetMeshMaterial("Lamp");
            lamp.SetPosition(-.105f, 0.88, 0.25f);
            lamp.SetParentName("SmallDrawersHis");

            lamp.SetRaycastShapeFromModel(AssetManager::GetModel("LampFull"));
            lamp.CreateRigidBody(lamp.GetGameWorldMatrix(), false);

            lamp.AddCollisionShapeFromConvexMesh(&AssetManager::GetModel("LampConvexMesh_0")->_meshes[0], filterData2);
            lamp.AddCollisionShapeFromConvexMesh(&AssetManager::GetModel("LampConvexMesh_1")->_meshes[0], filterData2);
            lamp.AddCollisionShapeFromConvexMesh(&AssetManager::GetModel("LampConvexMesh_2")->_meshes[0], filterData2);
            lamp.SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
            lamp.UpdateRigidBodyMassAndInertia(20.0f);


            //  lamp.userData = new PhysicsObjectData(PhysicsObjectType::GAME_OBJECT, &_gameObjects[_gameObjects.size()-1]);


            GameObject& smallChestOfDrawer_1 = _gameObjects.emplace_back();
            smallChestOfDrawer_1.SetModel("SmallDrawerTop");
            smallChestOfDrawer_1.SetMeshMaterial("Drawers");
            smallChestOfDrawer_1.SetParentName("SmallDrawersHis");
            smallChestOfDrawer_1.SetName("TopDraw");
            smallChestOfDrawer_1.SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f);
            smallChestOfDrawer_1.SetOpenAxis(OpenAxis::TRANSLATE_Z);
            smallChestOfDrawer_1.SetRaycastShapeFromModel(AssetManager::GetModel("SmallDrawerTop"));
            smallChestOfDrawer_1.CreateRigidBody(smallChestOfDrawer_1.GetGameWorldMatrix(), true);
            smallChestOfDrawer_1.AddCollisionShapeFromConvexMesh(&AssetManager::GetModel("SmallDrawerTop_ConvexMesh0")->_meshes[0], filterData2);
            smallChestOfDrawer_1.AddCollisionShapeFromConvexMesh(&AssetManager::GetModel("SmallDrawerTop_ConvexMesh1")->_meshes[0], filterData2);
            smallChestOfDrawer_1.AddCollisionShapeFromConvexMesh(&AssetManager::GetModel("SmallDrawerTop_ConvexMesh2")->_meshes[0], filterData2);
            smallChestOfDrawer_1.AddCollisionShapeFromConvexMesh(&AssetManager::GetModel("SmallDrawerTop_ConvexMesh3")->_meshes[0], filterData2);
            smallChestOfDrawer_1.AddCollisionShapeFromConvexMesh(&AssetManager::GetModel("SmallDrawerTop_ConvexMesh4")->_meshes[0], filterData2);



            GameObject& smallChestOfDrawer_2 = _gameObjects.emplace_back();
            smallChestOfDrawer_2.SetModel("SmallDrawerSecond");
            smallChestOfDrawer_2.SetMeshMaterial("Drawers");
            smallChestOfDrawer_2.SetParentName("SmallDrawersHis");
			smallChestOfDrawer_2.SetName("SecondDraw");
			smallChestOfDrawer_2.SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f);
			smallChestOfDrawer_2.SetOpenAxis(OpenAxis::TRANSLATE_Z);
            smallChestOfDrawer_2.SetRaycastShapeFromModel(AssetManager::GetModel("SmallDrawerSecond"));

            GameObject& smallChestOfDrawer_3 = _gameObjects.emplace_back();
            smallChestOfDrawer_3.SetModel("SmallDrawerThird");
            smallChestOfDrawer_3.SetMeshMaterial("Drawers");
			smallChestOfDrawer_3.SetParentName("SmallDrawersHis");
			smallChestOfDrawer_3.SetName("ThirdDraw");
			smallChestOfDrawer_3.SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f);
			smallChestOfDrawer_3.SetOpenAxis(OpenAxis::TRANSLATE_Z);
            smallChestOfDrawer_3.SetRaycastShapeFromModel(AssetManager::GetModel("SmallDrawerThird"));

            GameObject& smallChestOfDrawer_4 = _gameObjects.emplace_back();
            smallChestOfDrawer_4.SetModel("SmallDrawerFourth");
            smallChestOfDrawer_4.SetMeshMaterial("Drawers");
			smallChestOfDrawer_4.SetParentName("SmallDrawersHis");
			smallChestOfDrawer_4.SetName("ForthDraw");
			smallChestOfDrawer_4.SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f);
			smallChestOfDrawer_4.SetOpenAxis(OpenAxis::TRANSLATE_Z);
            smallChestOfDrawer_4.SetRaycastShapeFromModel(AssetManager::GetModel("SmallDrawerFourth"));
        }



        for (int y = 0; y < 12; y++) {

            GameObject* cube = &_gameObjects.emplace_back();
            float halfExtent = 0.1f;
            cube->SetPosition(2.0f, y * halfExtent * 2 + 0.1f, 3.5f);
            cube->SetModel("SmallCube");
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


            Transform transform;
            transform.position = glm::vec3(2.0f, y * halfExtent * 2 + 0.1f, 3.5f);

            PxShape* collisionShape = Physics::CreateBoxShape(halfExtent, halfExtent, halfExtent);
            PxShape* raycastShape = Physics::CreateBoxShape(halfExtent, halfExtent, halfExtent);

            PhysicsFilterData filterData;
            filterData.raycastGroup = RAYCAST_DISABLED;
            filterData.collisionGroup = CollisionGroup::GENERIC_BOUNCEABLE;
            filterData.collidesWith = (CollisionGroup)(ENVIROMENT_OBSTACLE | GENERIC_BOUNCEABLE);

            cube->CreateRigidBody(transform.to_mat4(), false);
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
        aks.SetAnimatedTransformsToBindPose();
        //aks.PlayAndLoopAnimation("AKS74U_ReloadEmpty", 0.2f);
        aks.PlayAndLoopAnimation("AKS74U_Idle", 1.0f);
        aks.SetMeshMaterial("manniquen1_2.001", "Hands");
        aks.SetMeshMaterial("manniquen1_2", "Hands");
        aks.SetMeshMaterial("SK_FPSArms_Female.001", "FemaleArms");
        aks.SetMeshMaterial("SK_FPSArms_Female", "FemaleArms");
        aks.SetMeshMaterialByIndex(2, "AKS74U_3");
        aks.SetMeshMaterialByIndex(3, "AKS74U_3"); // possibly incorrect. this is the follower
        aks.SetMeshMaterialByIndex(4, "AKS74U_1");
        aks.SetMeshMaterialByIndex(5, "AKS74U_4");
        aks.SetMeshMaterialByIndex(6, "AKS74U_0");
        aks.SetMeshMaterialByIndex(7, "AKS74U_2");
        aks.SetMeshMaterialByIndex(8, "AKS74U_1");  // Bolt_low. Possibly wrong
        aks.SetMeshMaterialByIndex(9, "AKS74U_3"); // possibly incorrect.
        aks.SetScale(0.01f);
        aks.SetPosition(glm::vec3(2.5f, 1.5f, 3));
        aks.SetRotationY(HELL_PI * 0.5f);
    }


    AnimatedGameObject& nurse = _animatedGameObjects.emplace_back(AnimatedGameObject());
    nurse.SetName("NURSEGUY");
    nurse.SetSkinnedModel("NurseGuy");
    nurse.SetAnimatedTransformsToBindPose();
    //nurse.PlayAndLoopAnimation("NurseGuy_Glock_Idle", 1.0f);
    nurse.SetMaterial("Glock");
    //  glock.SetScale(0.01f);
    nurse.SetPosition(glm::vec3(1.5f, 0.1f, 3));
    // glock.SetRotationY(HELL_PI * 0.5f);


    AnimatedGameObject& glock = _animatedGameObjects.emplace_back(AnimatedGameObject());
    glock.SetName("UNISEXGUY");
    glock.SetSkinnedModel("UniSexGuyScaled");
    glock.SetAnimatedTransformsToBindPose();
    //glock.PlayAndLoopAnimation("UnisexGuy_Glock_Idle", 1.0f);
    glock.SetMaterial("Glock");
    //  glock.SetScale(0.01f);
    glock.SetPosition(glm::vec3(3.0f, 0.1f, 3));
    // glock.SetRotationY(HELL_PI * 0.5f);

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
    

    for (Light& light : Scene::_lights) {
        light.isDirty = true;
    }

}

void Scene::LoadMap(std::string mapPath) {
    Scene::CleanUp();   
	File::LoadMap(mapPath);
    Scene::LoadHardCodedObjects();
	Scene::RecreateDataStructures();
}

void Scene::SaveMap(std::string mapPath) {
	File::SaveMap(mapPath);
}

void Scene::CreatePlayers() {
    _players.clear();
    _players.push_back(Player(glm::vec3(4.0f, 0.1f, 3.6f), glm::vec3(-0.17, 1.54f, 0)));
	if (EngineState::GetPlayerCount() == 2) {
		//_players.push_back(Player(glm::vec3(9.39f, 0.1f, 1.6f), glm::vec3(-0.25, 1.53f, 0)));
		_players.push_back(Player(glm::vec3(2.1f, 0.1f, 9.5f), glm::vec3(-0.25, 0.0f, 0.0f)));
        //_players[1]._ignoreControl = true;
    }

    _players[0]._keyboardIndex = 0;
    _players[1]._keyboardIndex = 1;
    _players[0]._mouseIndex = 0;
    _players[1]._mouseIndex = 1;

    _players[0]._ragdoll.LoadFromJSON("UnisexGuy3.rag");
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
    for (Window& window : _windows) {
        window.CleanUp();
    }
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

    float pointSpacing = Renderer::GetPointCloudSpacing();;

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
        filterData.collidesWith = (CollisionGroup)(GENERIC_BOUNCEABLE | BULLET_CASING | PLAYER);
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


    // Editor objects
    for (GameObject& gameObject : _gameObjects) {
        gameObject.CreateEditorPhysicsObject();
    }


}

void Scene::RemoveAllDecalsFromWindow(Window* window) {    

    std::cout << "size was: " << _decals.size() << "\n";

    for (int i = 0; i < _decals.size(); i++) {
        PxRigidBody* decalParentRigid = _decals[i].parent;
        if (decalParentRigid == (void*)window->raycastBody ||
            decalParentRigid == (void*)window->raycastBodyTop) {
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

            if (actorA->userData == (void*)&CasingType::SHOTGUN_SHELL) {
                Audio::PlayAudio("ShellFloorBounce.wav", Util::RandomFloat(0.2f, 0.3f));
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
    Renderer::CreatePointCloudBuffer();
    Renderer::CreateTriangleWorldVertexBuffer();
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

    for (Wall& wall : _walls) {
        wall.CreateMesh();
    }
    for (Floor& floor : _floors) {
        floor.CreateMesh();
    }

	_rtVertices.clear();
	_rtMesh.clear();

    // RT vertices and mesh
    int baseVertex = 0;

    // House
    {
        for (Wall& wall : _walls) {
            wall.CreateMesh();
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


//////////////
//          // 
//   Wall   //

Wall::Wall(glm::vec3 begin, glm::vec3 end, float height, int materialIndex) {
    this->materialIndex = materialIndex;
    this->begin = begin;
    this->end = end;
    this->height = height;
    CreateMesh();
}



void Wall::CreateMesh() {

    vertices.clear();
    ceilingTrims.clear();
    floorTrims.clear();
    collisionLines.clear();

    // Init shit
    bool finishedBuildingWall = false;
    glm::vec3 wallStart = begin;
    glm::vec3 wallEnd = end;
    glm::vec3 cursor = wallStart;
    glm::vec3 wallDir = glm::normalize(wallEnd - cursor);
    float texScale = 2.0f;
    if (materialIndex == AssetManager::GetMaterialIndex("WallPaper")) {
        texScale = 1.0f;
    }

    float uvX1 = 0;
    float uvX2 = 0;

    bool hasTrims = (height == WALL_HEIGHT);

    int count = 0;
    while (!finishedBuildingWall || count > 1000) {
        count++;
		float shortestDistance = 9999;
		Door* closestDoor = nullptr;
		Window* closestWindow = nullptr;
        glm::vec3 intersectionPoint;

        for (Door& door : Scene::_doors) {

            // Left side
            glm::vec3 v1(door.GetFloorplanVertFrontLeft(0.05f));
            glm::vec3 v2(door.GetFloorplanVertBackRight(0.05f));
            // Right side
            glm::vec3 v3(door.GetFloorplanVertBackLeft(0.05f));
            glm::vec3 v4(door.GetFloorplanVertFrontRight(0.05f));
            // If an intersection is found closer than one u have already then store it
            glm::vec3 tempIntersectionPoint;
            if (Util::LineIntersects(v1, v2, cursor, wallEnd, tempIntersectionPoint)) {
                if (shortestDistance > glm::distance(cursor, tempIntersectionPoint)) {
                    shortestDistance = glm::distance(cursor, tempIntersectionPoint);
                    closestDoor = &door;
                    intersectionPoint = tempIntersectionPoint;
                }
            }
            // Check the other side now
            if (Util::LineIntersects(v3, v4, cursor, wallEnd, tempIntersectionPoint)) {
                if (shortestDistance > glm::distance(cursor, tempIntersectionPoint)) {
                    shortestDistance = glm::distance(cursor, tempIntersectionPoint);
                    closestDoor = &door;
                    intersectionPoint = tempIntersectionPoint;
                }
            }
        }

		for (Window& window : Scene::_windows) {

            
			// Left side
			glm::vec3 v3(window.GetFrontLeftCorner());
			glm::vec3 v4(window.GetBackRightCorner());
			// Right side
			glm::vec3 v1(window.GetFrontRightCorner());
			glm::vec3 v2(window.GetBackLeftCorner());
           

          /*
			// Left side
			//glm::vec3 v1(window.position + glm::vec3(-0.1, 0, WINDOW_WIDTH * -0.5));
            glm::vec3 v1 = window.GetFrontLeftCorner();
			glm::vec3 v2(window.position + glm::vec3(0.1f, 0, WINDOW_WIDTH * -0.5));
			// Right side
			glm::vec3 v3(window.position + glm::vec3(-0.1, 0, WINDOW_WIDTH * 0.5));
			glm::vec3 v4(window.position + glm::vec3(0.1f, 0, WINDOW_WIDTH * 0.5));
*/
			v1.y = 0.1f;
			v2.y = 0.1f;
			v3.y = 0.1f;
			v4.y = 0.1f;

            //std::cout << "\n" << Util::Vec3ToString(v1) << "\n\n";

			// If an intersection is found closer than one u have already then store it
			glm::vec3 tempIntersectionPoint;
			if (Util::LineIntersects(v1, v2, cursor, wallEnd, tempIntersectionPoint)) {
				if (shortestDistance > glm::distance(cursor, tempIntersectionPoint)) {
					shortestDistance = glm::distance(cursor, tempIntersectionPoint);
                    closestWindow = &window;
                    closestDoor = NULL;
					intersectionPoint = tempIntersectionPoint;
					//std::cout << "\n\n\nHELLO\n\n";
				}
			}
			// Check the other side now
			if (Util::LineIntersects(v3, v4, cursor, wallEnd, tempIntersectionPoint)) {
				if (shortestDistance > glm::distance(cursor, tempIntersectionPoint)) {
					shortestDistance = glm::distance(cursor, tempIntersectionPoint);
					closestWindow = &window;
					closestDoor = NULL;
					intersectionPoint = tempIntersectionPoint;
                    //std::cout << "\n\n\nHELLO\n\n";
				}
			}
		}

        // Did ya find a door?
        if (closestDoor != nullptr) {

            // The wall piece from cursor to door            
            Vertex v1, v2, v3, v4;
            v1.position = cursor;
            v2.position = cursor + glm::vec3(0, height, 0);
            v3.position = intersectionPoint + glm::vec3(0, height, 0);
            v4.position = intersectionPoint;
            float segmentWidth = abs(glm::length((v4.position - v1.position))) / WALL_HEIGHT;
            float segmentHeight = glm::length((v2.position - v1.position)) / WALL_HEIGHT;
            uvX2 = uvX1 + segmentWidth;
            v1.uv = glm::vec2(uvX1, segmentHeight) * texScale;
            v2.uv = glm::vec2(uvX1, 0) * texScale;
            v3.uv = glm::vec2(uvX2, 0) * texScale;
            v4.uv = glm::vec2(uvX2, segmentHeight) * texScale;
            SetNormalsAndTangentsFromVertices(&v3, &v2, &v1);
            SetNormalsAndTangentsFromVertices(&v3, &v1, &v4);
            vertices.push_back(v3);
            vertices.push_back(v2);
            vertices.push_back(v1);
            vertices.push_back(v3);
            vertices.push_back(v1);
            vertices.push_back(v4);

            if (hasTrims) {
                Transform trimTransform;
                trimTransform.position = cursor;
                trimTransform.rotation.y = Util::YRotationBetweenTwoPoints(v4.position, v1.position) + HELL_PI;
                trimTransform.scale.x = segmentWidth * WALL_HEIGHT;
                ceilingTrims.push_back(trimTransform);
                floorTrims.push_back(trimTransform);
            }

            // Bit above the door
            Vertex v5, v6, v7, v8;
            v5.position = intersectionPoint + glm::vec3(0, DOOR_HEIGHT, 0);
            v6.position = intersectionPoint + glm::vec3(0, height, 0);
            v7.position = intersectionPoint + (wallDir * (DOOR_WIDTH + 0.005f)) + glm::vec3(0, height, 0);
            v8.position = intersectionPoint + (wallDir * (DOOR_WIDTH + 0.005f)) + glm::vec3(0, DOOR_HEIGHT, 0);
            segmentWidth = abs(glm::length((v8.position - v5.position))) / WALL_HEIGHT;
            segmentHeight = glm::length((v6.position - v5.position)) / WALL_HEIGHT;
            uvX1 = uvX2;
            uvX2 = uvX1 + segmentWidth;
            v5.uv = glm::vec2(uvX1, segmentHeight) * texScale;
            v6.uv = glm::vec2(uvX1, 0) * texScale;
            v7.uv = glm::vec2(uvX2, 0) * texScale;
            v8.uv = glm::vec2(uvX2, segmentHeight) * texScale;
            SetNormalsAndTangentsFromVertices(&v7, &v6, &v5);
            SetNormalsAndTangentsFromVertices(&v7, &v5, &v8);
            vertices.push_back(v7);
            vertices.push_back(v6);
            vertices.push_back(v5);
            vertices.push_back(v7);
            vertices.push_back(v5);
            vertices.push_back(v8);

            if (hasTrims) {
                Transform trimTransform;
                trimTransform.position = intersectionPoint;
                trimTransform.rotation.y = Util::YRotationBetweenTwoPoints(v4.position, v1.position) + HELL_PI;
                trimTransform.scale.x = segmentWidth * WALL_HEIGHT;
                ceilingTrims.push_back(trimTransform);
            }

            cursor = intersectionPoint + (wallDir * (DOOR_WIDTH + 0.005f)); // This 0.05 is so you don't get an intersection with the door itself
            uvX1 = uvX2;

            Line collisionLine;
            collisionLine.p1.pos = v1.position;
            collisionLine.p2.pos = v4.position;
            collisionLine.p1.color = YELLOW;
            collisionLine.p2.color = RED;
            collisionLines.push_back(collisionLine);
        }
        else if (closestWindow != nullptr) {

			// The wall piece from cursor to window            
			Vertex v1, v2, v3, v4;
			v1.position = cursor;
			v2.position = cursor + glm::vec3(0, height, 0);
			v3.position = intersectionPoint + glm::vec3(0, height, 0);
			v4.position = intersectionPoint;
			float segmentWidth = abs(glm::length((v4.position - v1.position))) / WALL_HEIGHT;
			float segmentHeight = glm::length((v2.position - v1.position)) / WALL_HEIGHT;
			uvX2 = uvX1 + segmentWidth;
			v1.uv = glm::vec2(uvX1, segmentHeight) * texScale;
			v2.uv = glm::vec2(uvX1, 0) * texScale;
			v3.uv = glm::vec2(uvX2, 0) * texScale;
			v4.uv = glm::vec2(uvX2, segmentHeight) * texScale;
			SetNormalsAndTangentsFromVertices(&v3, &v2, &v1);
			SetNormalsAndTangentsFromVertices(&v3, &v1, &v4);
			vertices.push_back(v3);
			vertices.push_back(v2);
			vertices.push_back(v1);
			vertices.push_back(v3);
			vertices.push_back(v1);
			vertices.push_back(v4);

			if (hasTrims) {
				Transform trimTransform;
				trimTransform.position = cursor;
				trimTransform.rotation.y = Util::YRotationBetweenTwoPoints(v4.position, v1.position) + HELL_PI;
				trimTransform.scale.x = segmentWidth * WALL_HEIGHT;
				ceilingTrims.push_back(trimTransform);
				floorTrims.push_back(trimTransform);
			}

			// Bit above the window
			Vertex v5, v6, v7, v8;
			v5.position = intersectionPoint + glm::vec3(0, WINDOW_HEIGHT, 0);
			v6.position = intersectionPoint + glm::vec3(0, height, 0);
			v7.position = intersectionPoint + (wallDir * (WINDOW_WIDTH + 0.005f)) + glm::vec3(0, height, 0);
			v8.position = intersectionPoint + (wallDir * (WINDOW_WIDTH + 0.005f)) + glm::vec3(0, WINDOW_HEIGHT, 0);
			segmentWidth = abs(glm::length((v8.position - v5.position))) / WALL_HEIGHT;
			segmentHeight = glm::length((v6.position - v5.position)) / WALL_HEIGHT;
			uvX1 = uvX2;
			uvX2 = uvX1 + segmentWidth;
			v5.uv = glm::vec2(uvX1, segmentHeight) * texScale;
			v6.uv = glm::vec2(uvX1, 0) * texScale;
			v7.uv = glm::vec2(uvX2, 0) * texScale;
			v8.uv = glm::vec2(uvX2, segmentHeight) * texScale;
			SetNormalsAndTangentsFromVertices(&v7, &v6, &v5);
			SetNormalsAndTangentsFromVertices(&v7, &v5, &v8);
			vertices.push_back(v7);
			vertices.push_back(v6);
			vertices.push_back(v5);
			vertices.push_back(v7);
			vertices.push_back(v5);
			vertices.push_back(v8);	

			// Bit below the window
            {
                float windowYBegin = 0.8f;
                float height = windowYBegin + 0.1f;

                Vertex v5, v6, v7, v8;
				v5.position = intersectionPoint + glm::vec3(0, 0, 0);
				v6.position = intersectionPoint + glm::vec3(0, height, 0);
				v7.position = intersectionPoint + (wallDir * (WINDOW_WIDTH + 0.005f)) + glm::vec3(0, height, 0);
				v8.position = intersectionPoint + (wallDir * (WINDOW_WIDTH + 0.005f)) + glm::vec3(0, 0, 0);
				segmentWidth = abs(glm::length((v8.position - v5.position))) / WALL_HEIGHT;
				segmentHeight = glm::length((v6.position - v5.position)) / WALL_HEIGHT;
                uvX1 = uvX2;
                uvX2 = uvX1 + segmentWidth;
                v5.uv = glm::vec2(uvX1, segmentHeight) * texScale;
                v6.uv = glm::vec2(uvX1, 0) * texScale;
                v7.uv = glm::vec2(uvX2, 0) * texScale;
                v8.uv = glm::vec2(uvX2, segmentHeight) * texScale;
                SetNormalsAndTangentsFromVertices(&v7, &v6, &v5);
                SetNormalsAndTangentsFromVertices(&v7, &v5, &v8);
                vertices.push_back(v7);
                vertices.push_back(v6);
                vertices.push_back(v5);
                vertices.push_back(v7);
                vertices.push_back(v5);
                vertices.push_back(v8);
            }

			if (hasTrims) {
				Transform trimTransform;
				trimTransform.position = intersectionPoint;
				trimTransform.rotation.y = Util::YRotationBetweenTwoPoints(v4.position, v1.position) + HELL_PI;
				trimTransform.scale.x = segmentWidth * WALL_HEIGHT;
				ceilingTrims.push_back(trimTransform);
				floorTrims.push_back(trimTransform);
			}


			cursor = intersectionPoint + (wallDir * (WINDOW_WIDTH + 0.005f)); // This 0.05 is so you don't get an intersection with the door itself
			uvX1 = uvX2;
		}

        // You're on the final bit of wall then aren't ya
        else {

            // The wall piece from cursor to door            
            Vertex v1, v2, v3, v4;
            v1.position = cursor;
            v2.position = cursor + glm::vec3(0, height, 0);
            v3.position = wallEnd + glm::vec3(0, height, 0);
            v4.position = wallEnd;
            float segmentWidth = abs(glm::length((v4.position - v1.position))) / WALL_HEIGHT;
            float segmentHeight = glm::length((v2.position - v1.position)) / WALL_HEIGHT;
            uvX2 = uvX1 + segmentWidth;
            v1.uv = glm::vec2(uvX1, segmentHeight) * texScale;
            v2.uv = glm::vec2(uvX1, 0) * texScale;
            v3.uv = glm::vec2(uvX2, 0) * texScale;
            v4.uv = glm::vec2(uvX2, segmentHeight) * texScale;

            SetNormalsAndTangentsFromVertices(&v3, &v2, &v1);
            SetNormalsAndTangentsFromVertices(&v3, &v1, &v4);
            vertices.push_back(v3);
            vertices.push_back(v2);
            vertices.push_back(v1);
            vertices.push_back(v3);
            vertices.push_back(v1);
            vertices.push_back(v4);
            finishedBuildingWall = true;

            if (hasTrims) {
                Transform trimTransform;
                trimTransform.position = cursor;
                trimTransform.rotation.y = Util::YRotationBetweenTwoPoints(v4.position, v1.position) + HELL_PI;
                trimTransform.scale.x = segmentWidth * WALL_HEIGHT;
                ceilingTrims.push_back(trimTransform);
                floorTrims.push_back(trimTransform);
            }

            Line collisionLine;
            collisionLine.p1.pos = v1.position;
            collisionLine.p2.pos = v4.position;
            collisionLine.p1.color = YELLOW;
            collisionLine.p2.color = RED;
            collisionLines.push_back(collisionLine);
        }
    }

    if (VAO != 0) {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
    }
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, bitangent));

}

glm::vec3 Wall::GetNormal() {
    glm::vec3 vector = glm::normalize(begin - end);
    return glm::vec3(-vector.z, 0, vector.x) * glm::vec3(-1);
}

glm::vec3 Wall::GetMidPoint() {
    return (begin + end) * glm::vec3(0.5);
}

void Wall::Draw() {
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, vertices.size());
}

///////////////////
//               //
//    Ceiling    //

Ceiling::Ceiling(float x1, float z1, float x2, float z2, float height, int materialIndex) {
    this->materialIndex = materialIndex;
    this->x1 = x1;
    this->z1 = z1;
    this->x2 = x2;
    this->z2 = z2;
    this->height = height;
    Vertex v1, v2, v3, v4;
    v1.position = glm::vec3(x1, height, z1);
    v2.position = glm::vec3(x1, height, z2);
    v3.position = glm::vec3(x2, height, z2);
    v4.position = glm::vec3(x2, height, z1);
    glm::vec3 normal = NormalFromThreePoints(v3.position, v2.position, v1.position);
    v1.normal = normal;
    v2.normal = normal;
    v3.normal = normal;
    v4.normal = normal;
    float scale = 2.0f;
    float uv_x_low = z1 / scale;
    float uv_x_high = z2 / scale;
    float uv_y_low = x1 / scale;
    float uv_y_high = x2 / scale;
    uv_x_low = x1;
    uv_x_high = x2;
    uv_y_low = z1;
    uv_y_high = z2;
    v1.uv = glm::vec2(uv_x_low, uv_y_low);
    v2.uv = glm::vec2(uv_x_low, uv_y_high);
    v3.uv = glm::vec2(uv_x_high, uv_y_high);
    v4.uv = glm::vec2(uv_x_high, uv_y_low);
    SetNormalsAndTangentsFromVertices(&v3, &v2, &v1);
    SetNormalsAndTangentsFromVertices(&v1, &v4, &v3);
    vertices.push_back(v3);
    vertices.push_back(v2);
    vertices.push_back(v1);

    vertices.push_back(v1);
    vertices.push_back(v4);
    vertices.push_back(v3);

    if (VAO != 0) {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
    }
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, bitangent));
}

void Ceiling::Draw() {
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, vertices.size());
}