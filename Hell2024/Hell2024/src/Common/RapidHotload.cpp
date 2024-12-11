#include "RapidHotload.h"
#include "../Game/Game.h"
#include "../Game/Scene.h"

std::vector<GameObject> g_emergencyGameObjects;

std::vector<GameObject>& RapidHotload::GetEmergencyGameObjects() {
    return g_emergencyGameObjects;
}

void CreateGameObject(GameObjectCreateInfo createInfo) {
    GameObject& gameObject = g_emergencyGameObjects.emplace_back();
    gameObject.SetPosition(createInfo.position);
    gameObject.SetRotation(createInfo.rotation);
    gameObject.SetScale(createInfo.scale);
    gameObject.SetName("GameObject");
    gameObject.SetModel(createInfo.modelName);
    gameObject.SetMeshMaterial(createInfo.materialName.c_str());
}

void RapidHotload::Update() {


    //std::vector<GameObject> g_emergencyGameObjects;
    g_emergencyGameObjects.clear();

    GameObjectCreateInfo createInfo;
    createInfo.position = glm::vec3(-4.35f, 0.4, 0.4f);
    createInfo.rotation = glm::vec3(0, -1.57079632679f, 0);
    createInfo.scale = glm::vec3(1.8, 1, 1);
    createInfo.materialName = "Ceiling2";
    createInfo.modelName = "House_WeatherBoardsA";
    CreateGameObject(createInfo);

    createInfo.position = glm::vec3(-4.35f, 0.4, 2.2f);
    createInfo.rotation = glm::vec3(0, -1.57079632679f, 0);
    createInfo.scale = glm::vec3(1, 1, 1);
    createInfo.materialName = "Ceiling2";
    createInfo.modelName = "House_WeatherBoardsWindow";
    CreateGameObject(createInfo);

    createInfo.position = glm::vec3(-4.35f, 0.4, 3.0f);
    createInfo.rotation = glm::vec3(0, -1.57079632679f, 0);
    createInfo.scale = glm::vec3(1.4, 1, 1);
    createInfo.materialName = "Ceiling2";
    createInfo.modelName = "House_WeatherBoardsA";
    CreateGameObject(createInfo);

    createInfo.position = glm::vec3(-4.35f, 0.4, -0.75f);
    createInfo.rotation = glm::vec3(0, -1.57079632679f, 0);
    createInfo.scale = glm::vec3(1.2, 1, 1);
    createInfo.materialName = "Ceiling2";
    createInfo.modelName = "House_WeatherBoardsDoor";
    CreateGameObject(createInfo);

    createInfo.position = glm::vec3(-4.35f, 0.4, -4.5f);
    createInfo.rotation = glm::vec3(0, -1.57079632679f, 0);
    createInfo.scale = glm::vec3(2, 1, 1);
    createInfo.materialName = "Ceiling2";
    createInfo.modelName = "House_WeatherBoardsA";
    CreateGameObject(createInfo);

    createInfo.position = glm::vec3(-4.35f, 0.4, -2.5f);
    createInfo.rotation = glm::vec3(0, -1.57079632679f, 0);
    createInfo.scale = glm::vec3(1, 1, 1);
    createInfo.materialName = "Ceiling2";
    createInfo.modelName = "House_WeatherBoardsWindow";
    CreateGameObject(createInfo);


    createInfo.position = glm::vec3(-4.35f, 0.4, -1.8f);
    createInfo.rotation = glm::vec3(0, -1.57079632679f, 0);
    createInfo.scale = glm::vec3(1.3f, 1, 1);
    createInfo.materialName = "Ceiling2";
    createInfo.modelName = "House_WeatherBoardsA";
    CreateGameObject(createInfo);



    // side of deck no window, on corner of RED ROON
    createInfo.position = glm::vec3(-2.0f, 0.4, -4.65f);
    createInfo.rotation = glm::vec3(0, HELL_PI, 0);
    createInfo.scale = glm::vec3(2.3f, 1, 1);
    createInfo.materialName = "Ceiling2";
    createInfo.modelName = "House_WeatherBoardsA";
    CreateGameObject(createInfo);
    createInfo.position = glm::vec3(-2.0f, 0.4 - 2.6f, -4.65f);
    createInfo.rotation = glm::vec3(0, HELL_PI, 0);
    createInfo.scale = glm::vec3(2.3f, 1, 1);
    createInfo.materialName = "Ceiling2";
    createInfo.modelName = "House_WeatherBoardsA";
    CreateGameObject(createInfo);


    //  RED ROOM TALL NO WINDOW
    createInfo.position = glm::vec3(-2.15f, 0.4, -8.5f);
    createInfo.rotation = glm::vec3(0, -1.57079632679f, 0);
    createInfo.scale = glm::vec3(4.0f, 1, 1);
    createInfo.materialName = "Ceiling2";
    createInfo.modelName = "House_WeatherBoardsA";
    CreateGameObject(createInfo);
    createInfo.position = glm::vec3(-2.15f, 3.0, -8.5f);
    createInfo.rotation = glm::vec3(0, -1.57079632679f, 0);
    createInfo.scale = glm::vec3(4.6f, 1, 1);
    createInfo.materialName = "Ceiling2";
    createInfo.modelName = "House_WeatherBoardsA";
    CreateGameObject(createInfo);
    createInfo.position = glm::vec3(-2.15f, 0.4f - 2.6f, -8.5f);
    createInfo.rotation = glm::vec3(0, -1.57079632679f, 0);
    createInfo.scale = glm::vec3(4.0f, 1, 1);
    createInfo.materialName = "Ceiling2";
    createInfo.modelName = "House_WeatherBoardsA";
    CreateGameObject(createInfo);



    //  Z NEGATIVE SIDE
    createInfo.position = glm::vec3(6.5f, 0.4, -4.65f);
    createInfo.rotation = glm::vec3(0, HELL_PI, 0);
    createInfo.scale = glm::vec3(6.0f, 1, 1);
    createInfo.materialName = "Ceiling2";
    createInfo.modelName = "House_WeatherBoardsA";
    CreateGameObject(createInfo);
    createInfo.position = glm::vec3(6.5f, 0.4 + 2.6f, -4.65f);
    createInfo.rotation = glm::vec3(0, HELL_PI, 0);
    createInfo.scale = glm::vec3(5.0f, 1, 1);
    createInfo.materialName = "Ceiling2";
    createInfo.modelName = "House_WeatherBoardsA";
    CreateGameObject(createInfo);
    createInfo.position = glm::vec3(6.5f, 0.4 - 2.6f, -4.65f);
    createInfo.rotation = glm::vec3(0, HELL_PI, 0);
    createInfo.scale = glm::vec3(5.0f, 1, 1);
    createInfo.materialName = "Ceiling2";
    createInfo.modelName = "House_WeatherBoardsA";
    CreateGameObject(createInfo);

    // RED WALL BOTTOM with lil window
    createInfo.position = glm::vec3(1.45f, 0.4, -8.5f);
    createInfo.rotation = glm::vec3(0, HELL_PI, 0);
    createInfo.scale = glm::vec3(1.9f, 1, 1);
    createInfo.materialName = "Ceiling2";
    createInfo.modelName = "House_WeatherBoardsA";
    CreateGameObject(createInfo);
    createInfo.position = glm::vec3(-0.45f, 0.4, -8.5f);
    createInfo.rotation = glm::vec3(0, HELL_PI, 0);
    createInfo.scale = glm::vec3(1.0f, 1, 1);
    createInfo.materialName = "Ceiling2";
    createInfo.modelName = "House_WeatherBoardsWindow";
    CreateGameObject(createInfo);
    createInfo.position = glm::vec3(-1.35f, 0.4, -8.5f);
    createInfo.rotation = glm::vec3(0, HELL_PI, 0);
    createInfo.scale = glm::vec3(0.7f, 1, 1);
    createInfo.materialName = "Ceiling2";
    createInfo.modelName = "House_WeatherBoardsA";
    CreateGameObject(createInfo);
    // bottom
    createInfo.position = glm::vec3(1.45f, 0.4 - 2.6f, -8.5f);
    createInfo.rotation = glm::vec3(0, HELL_PI, 0);
    createInfo.scale = glm::vec3(3.5f, 1, 1);
    createInfo.materialName = "Ceiling2";
    createInfo.modelName = "House_WeatherBoardsA";
    CreateGameObject(createInfo);



    // RED WALL TOP with lil window
    createInfo.position = glm::vec3(1.45f, 3.0f, -8.5f);
    createInfo.rotation = glm::vec3(0, HELL_PI, 0);
    createInfo.scale = glm::vec3(1.4f, 1, 1);
    createInfo.materialName = "Ceiling2";
    createInfo.modelName = "House_WeatherBoardsA";
    CreateGameObject(createInfo);
    createInfo.position = glm::vec3(0.05f, 3.0f, -8.5f);
    createInfo.rotation = glm::vec3(0, HELL_PI, 0);
    createInfo.scale = glm::vec3(1.0f, 1, 1);
    createInfo.materialName = "Ceiling2";
    createInfo.modelName = "House_WeatherBoardsWindow";
    CreateGameObject(createInfo);
    createInfo.position = glm::vec3(-0.85f, 3.0f, -8.5f);
    createInfo.rotation = glm::vec3(0, HELL_PI, 0);
    createInfo.scale = glm::vec3(1.3f, 1, 1);
    createInfo.materialName = "Ceiling2";
    createInfo.modelName = "House_WeatherBoardsA";
    CreateGameObject(createInfo);


    // Red wall tall no window
    createInfo.position = glm::vec3(1.5f, 0.4, -4.5f);
    createInfo.rotation = glm::vec3(0, 1.57079632679f, 0);
    createInfo.scale = glm::vec3(4.0f, 1, 1);
    createInfo.materialName = "Ceiling2";
    createInfo.modelName = "House_WeatherBoardsA";
    CreateGameObject(createInfo);
    createInfo.position = glm::vec3(1.5f, 0.4f - 2.6f, -4.5f);
    createInfo.rotation = glm::vec3(0, 1.57079632679f, 0);
    createInfo.scale = glm::vec3(4.0f, 1, 1);
    createInfo.materialName = "Ceiling2";
    createInfo.modelName = "House_WeatherBoardsA";
    CreateGameObject(createInfo);
    createInfo.position = glm::vec3(1.5f, 0.4f + 2.6f, -4.5f);
    createInfo.rotation = glm::vec3(0, 1.57079632679f, 0);
    createInfo.scale = glm::vec3(4.0f, 1, 1);
    createInfo.materialName = "Ceiling2";
    createInfo.modelName = "House_WeatherBoardsA";
    CreateGameObject(createInfo);


   // createInfo.position = glm::vec3(-4.45f, 0.4, -1.8f);
   // createInfo.rotation = glm::vec3(0, -1.57079632679f, 0);
   // createInfo.scale = glm::vec3(1.3f, 1, 1);
   // createInfo.materialName = "Gold";
   // createInfo.modelName = "House_WeatherBoardsA";
   // CreateGameObject(createInfo);






    // back of house (double staiirs)
    createInfo.position = glm::vec3(6.45f, 0.4f, -2.85f);
    createInfo.rotation = glm::vec3(0, 1.57079632679f, 0);
    createInfo.scale = glm::vec3(1.8f, 1, 1);
    createInfo.materialName = "Ceiling2";
    createInfo.modelName = "House_WeatherBoardsA";
    CreateGameObject(createInfo);
    createInfo.position = glm::vec3(6.45f, 0.4f, -2.0f);
    createInfo.rotation = glm::vec3(0, 1.57079632679f, 0);
    createInfo.scale = glm::vec3(1.0f, 1, 1);
    createInfo.materialName = "Ceiling2";
    createInfo.modelName = "House_WeatherBoardsDoor";
    CreateGameObject(createInfo);
    createInfo.position = glm::vec3(6.45f, 0.4f, 4.4f);
    createInfo.rotation = glm::vec3(0, 1.57079632679f, 0);
    createInfo.scale = glm::vec3(6.4f, 1, 1);
    createInfo.materialName = "Ceiling2";
    createInfo.modelName = "House_WeatherBoardsA";
    CreateGameObject(createInfo);
    createInfo.position = glm::vec3(6.45f, 0.4f - 2.6f, 4.4f);
    createInfo.rotation = glm::vec3(0, 1.57079632679f, 0);
    createInfo.scale = glm::vec3(8.9f, 1, 1);
    createInfo.materialName = "Ceiling2";
    createInfo.modelName = "House_WeatherBoardsA";
    CreateGameObject(createInfo);
    // top of that above
    createInfo.position = glm::vec3(6.45f, 0.4f + 2.6f, 1.5f);
    createInfo.rotation = glm::vec3(0, 1.57079632679f, 0);
    createInfo.scale = glm::vec3(1.5f, 1, 1);
    createInfo.materialName = "Ceiling2";
    createInfo.modelName = "House_WeatherBoardsA";
    CreateGameObject(createInfo);
    createInfo.position = glm::vec3(6.45f, 0.4f + 2.6f, 0.0f);
    createInfo.rotation = glm::vec3(0, 1.57079632679f, 0);
    createInfo.scale = glm::vec3(1.0f, 1, 1);
    createInfo.materialName = "Ceiling2";
    createInfo.modelName = "House_WeatherBoardsWindow";
    CreateGameObject(createInfo);
    createInfo.position = glm::vec3(6.45f, 0.4f + 2.6f, -0.8f);
    createInfo.rotation = glm::vec3(0, 1.57079632679f, 0);
    createInfo.scale = glm::vec3(3.8f, 1, 1);
    createInfo.materialName = "Ceiling2";
    createInfo.modelName = "House_WeatherBoardsA";
    CreateGameObject(createInfo);
    

    // Christmas tree side of house
    createInfo.position = glm::vec3(-4.3f, 0.4f - 2.6f, 4.5f);
    createInfo.rotation = glm::vec3(0, 0.0f, 0);
    createInfo.scale = glm::vec3(10.6f, 1, 1);
    createInfo.materialName = "Ceiling2";
    createInfo.modelName = "House_WeatherBoardsA";
    CreateGameObject(createInfo);
    createInfo.position = glm::vec3(-4.3f, 0.4f, 4.5f);
    createInfo.rotation = glm::vec3(0, 0.0f, 0);
    createInfo.scale = glm::vec3(1.0f, 1, 1);
    createInfo.materialName = "Ceiling2";
    createInfo.modelName = "House_WeatherBoardsA";
    CreateGameObject(createInfo);
    createInfo.position = glm::vec3(-3.3f, 0.4f, 4.5f);
    createInfo.rotation = glm::vec3(0, 0.0f, 0);
    createInfo.scale = glm::vec3(1.0f, 1, 1);
    createInfo.materialName = "Ceiling2";
    createInfo.modelName = "House_WeatherBoardsWindow";
    CreateGameObject(createInfo);
    createInfo.position = glm::vec3(-2.5f, 0.4f, 4.5f);
    createInfo.rotation = glm::vec3(0, 0.0f, 0);
    createInfo.scale = glm::vec3(4.6f, 1, 1);
    createInfo.materialName = "Ceiling2";
    createInfo.modelName = "House_WeatherBoardsA";
    CreateGameObject(createInfo);
    createInfo.position = glm::vec3(2.1f, 0.4f, 4.5f);
    createInfo.rotation = glm::vec3(0, 0.0f, 0);
    createInfo.scale = glm::vec3(1.0f, 1, 1);
    createInfo.materialName = "Ceiling2";
    createInfo.modelName = "House_WeatherBoardsWindow";
    CreateGameObject(createInfo);
    createInfo.position = glm::vec3(3.0f, 0.4f, 4.5f);
    createInfo.rotation = glm::vec3(0, 0.0f, 0);
    createInfo.scale = glm::vec3(3.4f, 1, 1);
    createInfo.materialName = "Ceiling2";
    createInfo.modelName = "House_WeatherBoardsA";
    CreateGameObject(createInfo);
    // Top above that

    createInfo.position = glm::vec3(-1.2f, 3.0f, 1.55f);
    createInfo.rotation = glm::vec3(0, 0, 0);
    createInfo.scale = glm::vec3(1.1f, 1, 1);
    createInfo.materialName = "Ceiling2";
    createInfo.modelName = "House_WeatherBoardsA";
    CreateGameObject(createInfo);
    createInfo.position = glm::vec3(-0.1f, 3.0f, 1.55f);
    createInfo.rotation = glm::vec3(0, 0, 0);
    createInfo.scale = glm::vec3(1.0, 1, 1);
    createInfo.materialName = "Ceiling2";
    createInfo.modelName = "House_WeatherBoardsWindow";
    CreateGameObject(createInfo);
    createInfo.position = glm::vec3(0.8f, 3.0f, 1.55f);
    createInfo.rotation = glm::vec3(0, 0, 0);
    createInfo.scale = glm::vec3(5.6, 1, 1);
    createInfo.materialName = "Ceiling2";
    createInfo.modelName = "House_WeatherBoardsA";
    CreateGameObject(createInfo);



    // right angle from that (opposite stairs top window wall)
    createInfo.position = glm::vec3(-1.3f, 3.0f, -0.1f);
    createInfo.rotation = glm::vec3(0, -HELL_PI * 0.5f, 0);
    createInfo.scale = glm::vec3(1.7f, 1, 1);
    createInfo.materialName = "Ceiling2";
    createInfo.modelName = "House_WeatherBoardsA";
    CreateGameObject(createInfo);
    createInfo.position = glm::vec3(-1.3f, 3.0f, -1.0f);
    createInfo.rotation = glm::vec3(0, -HELL_PI * 0.5f, 0);
    createInfo.scale = glm::vec3(1.0f, 1, 1);
    createInfo.materialName = "Ceiling2";
    createInfo.modelName = "House_WeatherBoardsWindow";
    CreateGameObject(createInfo);
    createInfo.position = glm::vec3(-1.3f, 3.0f, -3.8f);
    createInfo.rotation = glm::vec3(0, -HELL_PI * 0.5f, 0);
    createInfo.scale = glm::vec3(2.8f, 1, 1);
    createInfo.materialName = "Ceiling2";
    createInfo.modelName = "House_WeatherBoardsA";
    CreateGameObject(createInfo);


    createInfo.position = glm::vec3(-2.1f, 3.0f, -3.8f);
    createInfo.rotation = glm::vec3(0, 0, 0);
    createInfo.scale = glm::vec3(0.85, 1, 1);
    createInfo.materialName = "Ceiling2";
    createInfo.modelName = "House_WeatherBoardsA";
    CreateGameObject(createInfo);


}

glm::mat4 RapidHotload::computeTileProjectionMatrix(float fovY, float aspectRatio, float nearPlane, float farPlane, int screenWidth, int screenHeight, int tileX, int tileY, int tileWidth, int tileHeight) {
    // Compute the tangents of half the field of view angles
    float tanHalfFovY = tanf(fovY * 0.5f);
    float tanHalfFovX = tanHalfFovY * aspectRatio;

    // Full frustum boundaries at the near plane
    float topFull = nearPlane * tanHalfFovY;
    float bottomFull = -topFull;
    float rightFull = nearPlane * tanHalfFovX;
    float leftFull = -rightFull;

    // Normalize tile coordinates to [0, 1]
    float u0 = tileX / screenWidth;
    float u1 = (tileX + tileWidth) / screenWidth;
    float v0 = tileY / screenHeight;
    float v1 = (tileY + tileHeight) / screenHeight;

    // Flip Y-axis: screen origin is top-left, NDC origin is bottom-left
    float v0_flipped = 1.0f - v1;
    float v1_flipped = 1.0f - v0;

    // Interpolate frustum boundaries for the tile
    float left = leftFull + (rightFull - leftFull) * u0;
    float right = leftFull + (rightFull - leftFull) * u1;
    float bottom = bottomFull + (topFull - bottomFull) * v0_flipped;
    float top = bottomFull + (topFull - bottomFull) * v1_flipped;

    // Create the projection matrix using glm::frustum
    glm::mat4 projectionMatrix = glm::frustum(left, right, bottom, top, nearPlane, farPlane);

    return projectionMatrix;
}