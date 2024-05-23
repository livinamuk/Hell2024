#pragma once
#include "../../Common.h"
#include "../../API/OpenGL/Types/GL_shader.h"
#include "../../Core/AssetManager.h"
#include "../../Core/GameObject.h"

struct Toilet {

    Transform transform;
    GameObject mainBody;
    GameObject lid;
    GameObject seat;

    Toilet() {}

    Toilet(glm::vec3 position, float rotation) {

        transform.position = position;
        transform.rotation.y = rotation;

        PhysicsFilterData filterData;
        filterData.raycastGroup = RAYCAST_DISABLED;
        filterData.collisionGroup = CollisionGroup::ENVIROMENT_OBSTACLE;
        filterData.collidesWith = (CollisionGroup)(GENERIC_BOUNCEABLE | BULLET_CASING | PLAYER | RAGDOLL);
        /*
        mainBody.SetPosition(position);
        mainBody.SetModel("Toilet");
        mainBody.SetName("Toilet");
        mainBody.SetMeshMaterial("Toilet");
        mainBody.SetRaycastShapeFromModel(OpenGLAssetManager::GetModel("Toilet"));
        mainBody.SetRotationY(rotation);
        mainBody.SetKinematic(true);
      
        OpenGLModel* convexMesh0 = OpenGLAssetManager::GetModel("Toilet_ConvexMesh");
        if (convexMesh0) {
            mainBody.AddCollisionShapeFromConvexMesh(&convexMesh0->_meshes[0], filterData);
        }
        OpenGLModel* convexMesh1 = OpenGLAssetManager::GetModel("Toilet_ConvexMesh1");
        if (convexMesh1) {
            mainBody.AddCollisionShapeFromConvexMesh(&convexMesh1->_meshes[0], filterData);
        }*/
        mainBody.SetModelMatrixMode(ModelMatrixMode::GAME_TRANSFORM);

        glm::vec4 seatPosition = mainBody._transform.to_mat4() * glm::vec4(0.0f, 0.40727f, -0.2014f, 1.0f);
        glm::vec4 lidPosition = mainBody._transform.to_mat4() * glm::vec4(0.0f, 0.40727f, -0.2014f, 1.0f);

        seat.SetModel("ToiletSeat");
        seat.SetPosition(seatPosition);
        seat.SetRotationY(rotation);
        seat.SetName("ToiletSeat");
        seat.SetMeshMaterial("Toilet");
        seat.SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f);
        seat.SetOpenAxis(OpenAxis::ROTATION_NEG_X);
        //seat.SetRaycastShapeFromModel(OpenGLAssetManager::GetModel("ToiletSeat"));
        seat.SetModelMatrixMode(ModelMatrixMode::GAME_TRANSFORM);

        lid.SetPosition(lidPosition);
        lid.SetRotationY(rotation);
        lid.SetModel("ToiletLid");
        lid.SetName("ToiletLid");
        lid.SetMeshMaterial("Toilet");
        lid.SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f);
        lid.SetOpenAxis(OpenAxis::ROTATION_POS_X);
      //  lid.SetRaycastShapeFromModel(OpenGLAssetManager::GetModel("ToiletLid"));
        lid.SetModelMatrixMode(ModelMatrixMode::GAME_TRANSFORM);
    }

    void Draw(Shader& shader, bool bindMaterial = true) {

        // Main body
    /*    shader.SetMat4("model", mainBody.GetModelMatrix());
        for (int i = 0; i < mainBody._meshMaterialIndices.size(); i++) {
            if (bindMaterial) {
                AssetManager::BindMaterialByIndex(mainBody._meshMaterialIndices[i]);
            }
            mainBody._model_OLD->_meshes[i].Draw();
        }    
        // Lid
        shader.SetMat4("model", lid.GetModelMatrix());
        for (int i = 0; i < lid._meshMaterialIndices.size(); i++) {
            if (bindMaterial) {
                AssetManager::BindMaterialByIndex(lid._meshMaterialIndices[i]);
            }
            lid._model_OLD->_meshes[i].Draw();
        }    
        // Seat
        shader.SetMat4("model", seat.GetModelMatrix());
        for (int i = 0; i < seat._meshMaterialIndices.size(); i++) {
            if (bindMaterial) {
                AssetManager::BindMaterialByIndex(seat._meshMaterialIndices[i]);
            }
            seat._model_OLD->_meshes[i].Draw();
        }*/
    }

    void Update(float deltaTime) {
        mainBody.Update(deltaTime);
        lid.Update(deltaTime);
        seat.Update(deltaTime);
    }

    glm::mat4 GetModelMatrix() {
        return transform.to_mat4();
    }

    void CleanUp() {
        mainBody.CleanUp();
        seat.CleanUp();
        lid.CleanUp();
    }
};