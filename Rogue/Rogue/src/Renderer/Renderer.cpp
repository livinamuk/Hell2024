#include "Renderer.h"
#include "GBuffer.h""
#include "Mesh.h"
#include "MeshUtil.hpp"
#include "Shader.h"
#include "ShadowMap.h"
#include "Texture.h"
#include "Texture3D.h"
#include "../common.h"
#include "../Util.hpp"
#include "../Core/Audio.hpp"
#include "../Core/GL.h"
#include "../Core/Player.h"
#include "../Core/Input.h"
#include "../Core/Scene.h"
#include "../Core/AssetManager.h"
#include "../Core/TextBlitter.h"
#include "../Core/Editor.h"
#include "Model.h"

#include <vector>
#include <cstdlib>
#include <format>

#include "../Core/AnimatedGameObject.h"

Shader _testShader;
Shader _solidColorShader;
Shader _shadowMapShader;
Shader _UIShader;
Shader _editorSolidColorShader;
Shader _compositeShader;
Shader _velocityMapShader;

Mesh _cubeMesh;

GBuffer _gBuffer;
GBuffer _gBufferFinalSize;

Texture _floorBoardsTexture;
Texture _wallpaperTexture;
Texture _plasterTexture;

unsigned int _pointLineVAO;
unsigned int _pointLineVBO;
int _renderWidth = 512  * 1.25f * 2;
int _renderHeight = 288 * 1.25f * 2;

std::vector<Point> _points;
std::vector<Point> _solidTrianglePoints;
std::vector<Line> _lines;

std::vector<UIRenderInfo> _UIRenderInfos;

Texture3D _indirectLightingTexture;

std::vector<ShadowMap> _shadowMaps;

//AnimatedGameObject _enemy;
//AnimatedGameObject _ak47;
//Enemy _glock2;

struct Toggles {
    bool drawLights = false;
    bool drawProbes = false;
    bool drawLines = false;
} _toggles;

enum RenderMode { COMPOSITE = 0, DIRECT_LIGHT, INDIRECT_LIGHT, POINT_CLOUD,  MODE_COUNT} _mode;

void QueueLineForDrawing(Line line);
void QueuePointForDrawing(Point point);
void QueueTriangleForLineRendering(Triangle& triangle);
void QueueTriangleForSolidRendering(Triangle& triangle);
void DrawScene(Shader& shader);
void DrawAnimatedScene(Shader& shader);
void DrawFullscreenQuad();

int _voxelTextureWidth = { 0 };
int _voxelTextureHeight = { 0 };
int _voxelTextureDepth = { 0 };
float _voxelSize2 = 0.2f;

GLuint _imageStoreTexture = { 0 };

void Renderer::Init() {

    Scene::Init();

    glGenVertexArrays(1, &_pointLineVAO);
    glGenBuffers(1, &_pointLineVBO);
    glPointSize(8);
        
    _testShader.Load("test.vert", "test.frag");
    _solidColorShader.Load("solid_color.vert", "solid_color.frag");
    _shadowMapShader.Load("shadowmap.vert", "shadowmap.frag", "shadowmap.geom");    
    _UIShader.Load("ui.vert", "ui.frag");
    _editorSolidColorShader.Load("editor_solid_color.vert", "editor_solid_color.frag");
    _compositeShader.Load("composite.vert", "composite.frag"); 
    _velocityMapShader.Load("velocity_map.vert", "velocity_map.frag");

    _floorBoardsTexture = Texture("res/textures/floorboards.png");
    _wallpaperTexture = Texture("res/textures/wallpaper.png");
    _plasterTexture = Texture("res/textures/plaster.png");
    //_floorBoardsTexture = Texture("res/textures/numgrid.png");
    
    _cubeMesh = MeshUtil::CreateCube(1.0f, 1.0f, true);

    RecreateFrameBuffers();

    _indirectLightingTexture = Texture3D(PROPOGATION_WIDTH, PROPOGATION_HEIGHT, PROPOGATION_DEPTH);

    _shadowMaps.push_back(ShadowMap());
    _shadowMaps.push_back(ShadowMap());
    _shadowMaps.push_back(ShadowMap());
    _shadowMaps.push_back(ShadowMap());

    for (ShadowMap& shadowMap : _shadowMaps) {
        shadowMap.Init();
    }
}

void Renderer::RecreateFrameBuffers() {
    _gBuffer.Configure(_renderWidth * 2, _renderHeight * 2);
    _gBufferFinalSize.Configure(_renderWidth / 2, _renderHeight / 2);
}

void DrawScene(Shader& shader) {

    for (Wall& wall : Scene::_walls) {
        AssetManager::BindMaterialByIndex(wall.materialIndex);
        wall.Draw();
    }
    for (Floor& floor : Scene::_floors) {
        AssetManager::BindMaterialByIndex(floor.materialIndex);
        floor.Draw();
    }
    for (Ceiling& ceiling : Scene::_ceilings) {
        AssetManager::BindMaterialByIndex(ceiling.materialIndex);
        ceiling.Draw();
    }

    for (Wall& wall : Scene::_walls) {

        if (wall.height != WALL_HEIGHT)
            continue;

        static int trimMaterialIndex = AssetManager::GetMaterialIndex("Trims");
        AssetManager::BindMaterialByIndex(trimMaterialIndex);
        Transform t;
        t.position = wall.begin;
        t.rotation.y = Util::YRotationBetweenTwoPoints(wall.begin, wall.end);
        t.scale.x = glm::distance(wall.end, wall.begin);
        shader.SetMat4("model", t.to_mat4());
        AssetManager::GetModel("TrimCeiling").Draw();
        
    }

    for (GameObject& gameObject : Scene::_gameObjects) {
        shader.SetMat4("model", gameObject.GetModelMatrix());
        for (int i = 0; i < gameObject._meshMaterialIndices.size(); i++) {
            AssetManager::BindMaterialByIndex(gameObject._meshMaterialIndices[i]);
            gameObject._model->_meshes[i].Draw();
        }
    }
}

void DrawAnimatedObject(Shader& shader, AnimatedGameObject* animatedGameObject) {

    if (!animatedGameObject) {
        std::cout << "You tried to draw an nullptr AnimatedGameObject\n";
        return;
    }
    if (!animatedGameObject->_skinnedModel) {
        std::cout << "You tried to draw an AnimatedGameObject with a nullptr skinned model\n";
        return;
    }

    shader.SetMat4("model", animatedGameObject->GetModelMatrix());

    std::vector<glm::mat4> tempSkinningMats;
    for (unsigned int i = 0; i < animatedGameObject->_animatedTransforms.local.size(); i++) {

        glm::mat4 matrix = animatedGameObject->_animatedTransforms.local[i];
        shader.SetMat4("skinningMats[" + std::to_string(i) + "]", matrix);

        matrix = animatedGameObject->_animatedTransformsPrevious.local[i];
        shader.SetMat4("skinningMatsPrevious[" + std::to_string(i) + "]", matrix);

        tempSkinningMats.push_back(matrix);
    }
    shader.SetMat4("model", animatedGameObject->GetModelMatrix());

    SkinnedModel& skinnedModel = *animatedGameObject->_skinnedModel;

    glBindVertexArray(skinnedModel.m_VAO);

    for (auto bone : animatedGameObject->_animatedTransforms.worldspace) {
        glm::mat4 m = animatedGameObject->GetModelMatrix() * bone;
        float x = m[3][0];
        float y = m[3][1];
        float z = m[3][2];
        //Point p(glm::vec3(x, y, z), YELLOW);
        Point p2(glm::vec3(x, y, z), RED);
   //     QueuePointForDrawing(p2);
    }

    std::vector<Triangle> enemyTris;

    for (int i = 0; i < skinnedModel.m_meshEntries.size(); i++) {


        if (skinnedModel.m_meshEntries[i].Name == "manniquen1_2.001" || 
            skinnedModel.m_meshEntries[i].Name == "manniquen1_2") {
            AssetManager::BindMaterialByIndex(AssetManager::GetMaterialIndex("Hands"));
            //continue;
        }
        else if (skinnedModel.m_meshEntries[i].Name == "Shotgun Mesh") {
            AssetManager::BindMaterialByIndex(AssetManager::GetMaterialIndex("Shotgun"));
          
        }
        else if (skinnedModel.m_meshEntries[i].Name == "SK_FPSArms_Female.001" || 
            skinnedModel.m_meshEntries[i].Name == "SK_FPSArms_Female") {
            continue;
            AssetManager::BindMaterialByIndex(AssetManager::GetMaterialIndex("NumGrid"));
            AssetManager::BindMaterialByIndex(AssetManager::GetMaterialIndex("FemaleArms"));
        }
        else {
            AssetManager::BindMaterialByIndex(AssetManager::GetMaterialIndex("Glock"));
            //AssetManager::BindMaterialByIndex(AssetManager::GetMaterialIndex("NumGrid"));
        }
        
        
        if (i == 0) {
            //_skinBodyTexture.Bind(5);
        }
        else if (i == 1) {
            //_skinHeadTexture.Bind(5);
        }
        else if (i == 2) {
            //_skinBodyTexture.Bind(5);
        }
        else if (i == 3) {
            //_skinBodyTexture.Bind(5);
        }
        else if (i == 4) {
            //_skinBodyTexture.Bind(5);
        }
        else if (i == 5) {
            //_eyesTexture.Bind(5);
        }
        else if (i == 6) {
            // _eyesTexture.Bind(5);
        }
        else if (i == 7) {
            // _jeansTexture.Bind(5);
        }

        glDrawElementsBaseVertex(GL_TRIANGLES, skinnedModel.m_meshEntries[i].NumIndices, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * skinnedModel.m_meshEntries[i].BaseIndex), skinnedModel.m_meshEntries[i].BaseVertex);

        /*int eyeMeshIndex = 5;
        int eyeMeshIndex2 = 6;

        if (i != eyeMeshIndex && i != eyeMeshIndex2)
        {
            int baseIndex = skinnedModel.m_meshEntries[i].BaseIndex;
            int BaseVertex = skinnedModel.m_meshEntries[i].BaseVertex;

            for (int j = 0; j < skinnedModel.m_meshEntries[i].NumIndices; j += 3) {

                int index0 = skinnedModel.Indices[j + 0 + baseIndex];
                int index1 = skinnedModel.Indices[j + 1 + baseIndex];
                int index2 = skinnedModel.Indices[j + 2 + baseIndex];
                glm::vec3 p1 = skinnedModel.Positions[index0 + BaseVertex];
                glm::vec3 p2 = skinnedModel.Positions[index1 + BaseVertex];
                glm::vec3 p3 = skinnedModel.Positions[index2 + BaseVertex];
                VertexBoneData vert1BoneData = skinnedModel.Bones[index0 + BaseVertex];
                VertexBoneData vert2BoneData = skinnedModel.Bones[index1 + BaseVertex];
                VertexBoneData vert3BoneData = skinnedModel.Bones[index2 + BaseVertex];

                {
                    glm::vec4 totalLocalPos = glm::vec4(0);
                    glm::vec4 vertexPosition = glm::vec4(p1, 1.0);


                    for (int k = 0; k < 4; k++) {
                        glm::mat4 jointTransform = tempSkinningMats[int(vert1BoneData.IDs[k])];
                        glm::vec4 posePosition = jointTransform * vertexPosition * vert1BoneData.Weights[k];
                        totalLocalPos += posePosition;
                    }
                    p1 = totalLocalPos;
                }
                {
                    glm::vec4 totalLocalPos = glm::vec4(0);
                    glm::vec4 vertexPosition = glm::vec4(p2, 1.0);

                    for (int k = 0; k < 4; k++) {
                        glm::mat4 jointTransform = tempSkinningMats[int(vert2BoneData.IDs[k])];
                        glm::vec4 posePosition = jointTransform * vertexPosition * vert2BoneData.Weights[k];
                        totalLocalPos += posePosition;
                    }
                    p2 = totalLocalPos;
                }
                {
                    glm::vec4 totalLocalPos = glm::vec4(0);
                    glm::vec4 vertexPosition = glm::vec4(p3, 1.0);

                    for (int k = 0; k < 4; k++) {
                        glm::mat4 jointTransform = tempSkinningMats[int(vert3BoneData.IDs[k])];
                        glm::vec4 posePosition = jointTransform * vertexPosition * vert3BoneData.Weights[k];
                        totalLocalPos += posePosition;
                    }
                    p3 = totalLocalPos;
                }

                Line lineA, lineB, lineC;
                lineA.p1 = Point(p1, YELLOW);
                lineA.p2 = Point(p2, YELLOW);
                lineB.p1 = Point(p2, YELLOW);
                lineB.p2 = Point(p3, YELLOW);
                lineC.p1 = Point(p3, YELLOW);
                lineC.p2 = Point(p1, YELLOW);

                // QueueLineForDrawing(lineA);
              //   QueueLineForDrawing(lineB);
               //  QueueLineForDrawing(lineC);

                Triangle tri;
                tri.p1 = p1;
                tri.p2 = p2;
                tri.p3 = p3;
                enemyTris.push_back(tri);
            }
        }*/
    }
}

void DrawAnimatedScene(Shader& shader) {

    shader.Use();
    shader.SetBool("isAnimated", true);

    DrawAnimatedObject(shader, Scene::GetAnimatedGameObjectByName("Enemy"));

    glm::mat4 projection = glm::perspective(1.0f, (float)GL::GetWindowWidth() / (float)GL::GetWindowHeight(), 0.01f, 100.0f);
    shader.SetMat4("projection", projection);

    DrawAnimatedObject(shader, Scene::GetAnimatedGameObjectByName("AKS74U"));

    shader.SetBool("isAnimated", false);
}


void RenderImmediate() {

    glBindVertexArray(_pointLineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, _pointLineVBO);

    // Draw triangles
    glBufferData(GL_ARRAY_BUFFER, _solidTrianglePoints.size() * sizeof(Point), _solidTrianglePoints.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)offsetof(Point, color));
    glBindVertexArray(_pointLineVAO);
    glBindVertexArray(_pointLineVAO);
    glDrawArrays(GL_TRIANGLES, 0, _solidTrianglePoints.size());

    // Draw lines
    glBufferData(GL_ARRAY_BUFFER, _lines.size() * sizeof(Line), _lines.data(), GL_STATIC_DRAW);
    glDrawArrays(GL_LINES, 0, 2 * _lines.size());

    // Draw points
    glBufferData(GL_ARRAY_BUFFER, _points.size() * sizeof(Point), _points.data(), GL_STATIC_DRAW);
    glBindVertexArray(_pointLineVAO);
    glDrawArrays(GL_POINTS, 0, _points.size());

    // Cleanup
    _lines.clear();
    _points.clear();
    _solidTrianglePoints.clear();
}

void Renderer::RenderFrame() {

    // Viewing mode
    _testShader.Use();
    if (_mode == RenderMode::COMPOSITE) {
        TextBlitter::_debugTextToBilt = "Mode: COMPOSITE\n";
        _testShader.SetInt("mode", 0);
    }
    if (_mode == RenderMode::POINT_CLOUD) {
        TextBlitter::_debugTextToBilt = "Mode: POINT CLOUD\n";
        _testShader.SetInt("mode", 1);
    }
    if (_mode == RenderMode::DIRECT_LIGHT) {
        TextBlitter::_debugTextToBilt = "Mode: DIRECT LIGHT\n";
        _testShader.SetInt("mode", 2);
    }
    if (_mode == RenderMode::INDIRECT_LIGHT) {
        TextBlitter::_debugTextToBilt = "Mode: INDIRECT LIGHT\n";
        _testShader.SetInt("mode", 3);
    }

    glm::mat4 projection = glm::perspective(0.9f, (float)GL::GetWindowWidth() / (float)GL::GetWindowHeight(), 0.01f, 100.0f);
   
  //  Transform cameraRotationCorrection;
  //  cameraRotationCorrection.rotation.x = HELL_PI / 2;
  //  cameraRotationCorrection.rotation.y = HELL_PI;

    AnimatedGameObject* aks74u = Scene::GetAnimatedGameObjectByName("AKS74U");
    glm::mat4 view = glm::mat4(glm::mat3(aks74u->_cameraMatrix)) * Player::GetViewMatrix();
   // glm::mat4 view =  Player::GetViewMatrix();
   // std::cout << Util::Mat4ToString(_ak47._cameraMatrix) << "\n";

    // Previous Frame
    _gBuffer.Bind();
    glViewport(0, 0, _gBuffer.GetWidth(), _gBuffer.GetHeight());
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDrawBuffer(GL_COLOR_ATTACHMENT3);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    _velocityMapShader.Use();
    _velocityMapShader.SetMat4("projection", projection);
    _velocityMapShader.SetMat4("view", view);
    _velocityMapShader.SetMat4("model", glm::mat4(1));
    DrawScene(_velocityMapShader);
    DrawAnimatedScene(_velocityMapShader);



    // Current frame


    _gBuffer.Bind();
    unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);
    glViewport(0, 0, _gBuffer.GetWidth(), _gBuffer.GetHeight());
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);


    glActiveTexture(GL_TEXTURE8);
    glBindTexture(GL_TEXTURE_2D, _gBuffer._gVelocityMapTexture);

    //projection = glm::perspective(0.9f, (float)GL::GetWindowWidth() / (float)GL::GetWindowHeight(), 0.1f, 100.0f);

    //Transform cameraRotationCorrection;
    //cameraRotationCorrection.rotation.x = HELL_PI / 2;
    //cameraRotationCorrection.rotation.z = HELL_PI;

    //glm::mat4 view = glm::mat4(glm::mat3(_glock2._cameraMatrix)) * cameraRotationCorrection.to_mat4() * Player::GetViewMatrix();

    //glm::mat4 view = Player::GetViewMatrix();// *_glock2._cameraMatrix;

    _testShader.Use();
    _testShader.SetMat4("projection", projection);
    _testShader.SetMat4("view", view);
    _testShader.SetVec3("viewPos", Player::GetViewPos());
    _testShader.SetVec3("camForward", Player::GetCameraFront());

    // Bind shadow maps

    for (int i = 0; i < 4; i++) {
        _testShader.SetVec3("lightColor[" + std::to_string(i) + "]", BLACK);
    }

    auto& lights = Scene::_lights;
    for (int i = 0; i < lights.size(); i++) {
        glActiveTexture(GL_TEXTURE1 + i);
        glBindTexture(GL_TEXTURE_CUBE_MAP, _shadowMaps[i]._depthTexture);
        glm::vec3 position = glm::vec3(lights[i].x, lights[i].y, lights[i].z) * _voxelSize2;
        _testShader.SetVec3("lightPosition[" + std::to_string(i) + "]", position);
        _testShader.SetVec3("lightColor[" + std::to_string(i) + "]", lights[i].color);
        _testShader.SetFloat("lightRadius[" + std::to_string(i) + "]", lights[i].radius);
        _testShader.SetFloat("lightStrength[" + std::to_string(i) + "]", lights[i].strength);
    }

    // Fill Indirect Lighting Texture
    _indirectLightingTexture.Bind(0); // ? is this neccessary
    int width = _indirectLightingTexture.GetWidth();
    int height = _indirectLightingTexture.GetHeight();
    int depth = _indirectLightingTexture.GetDepth();

    std::vector<glm::vec4> data;
    for (int z = 0; z < depth; z++) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {

                if (Scene::_propogrationGrid[x][y][z].ignore) {
                    Scene::_propogrationGrid[x][y][z].color = glm::vec3(-1);
                }
                glm::vec4 color = glm::vec4(Scene::_propogrationGrid[x][y][z].color, 1);
                data.push_back(color);
            }
        }
    }
    glActiveTexture(GL_TEXTURE0);
    glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, width, height, depth, GL_RGBA, GL_FLOAT, data.data());

    static float time = 0;
    time += 0.01;
    _testShader.SetMat4("model", glm::mat4(1));
    _testShader.SetFloat("time", time);
    _testShader.SetFloat("screenWidth", GetRenderWidth());
    _testShader.SetFloat("screenHeight", GetRenderHeight());

    DrawScene(_testShader);
    DrawAnimatedScene(_testShader);





    //glViewport(0, 0, _gBuffer.GetWidth(), _gBuffer.GetHeight());
   // glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
   // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDisable(GL_DEPTH_TEST);

    _solidColorShader.Use();
    _solidColorShader.SetMat4("projection", projection);
    _solidColorShader.SetMat4("view", Player::GetViewMatrix());
    _solidColorShader.SetVec3("viewPos", Player::GetViewPos());
    _solidColorShader.SetVec3("color", glm::vec3(1, 1, 0));
    _solidColorShader.SetMat4("model", glm::mat4(1));

     // Draw lights
    if (_toggles.drawLights) {
        for (Light& light : Scene::_lights) {
            glm::vec3 lightCenter = glm::vec3(light.x, light.y, light.z) * _voxelSize2;
            Transform lightTransform;
            lightTransform.scale = glm::vec3(_voxelSize2);
            lightTransform.position = lightCenter;
            _solidColorShader.SetMat4("model", lightTransform.to_mat4());
            _solidColorShader.SetVec3("color", WHITE);
            _solidColorShader.SetBool("uniformColor", true);
            _cubeMesh.Draw();
        }
    }

    // Draw probes
    glEnable(GL_DEPTH_TEST);
    if (_toggles.drawProbes) {
        for (int x = 0; x < PROPOGATION_WIDTH; x++) {
            for (int y = 0; y < PROPOGATION_HEIGHT; y++) {
                for (int z = 0; z < PROPOGATION_DEPTH; z++) {

                    GridProbe& probe = Scene::_propogrationGrid[x][y][z];

                    float worldX = x * VOXEL_SIZE;
                    float worldY = y * VOXEL_SIZE;
                    float worldZ = z * VOXEL_SIZE;

                    if (worldX < 0.1f ||
                        worldX > 6.1f ||
                        worldY < 0.1f ||
                        worldY > 3.1 ||
                        worldZ < 0.1f ||
                        worldZ > 6.8f) {
                        //probe.ignore = true;
                        //continue;
                    }

                    // draw
                    Transform t;
                    t.scale = glm::vec3(0.05f);
                    t.position = glm::vec3(worldX, worldY, worldZ);
                    _solidColorShader.SetMat4("model", t.to_mat4());

                    if (probe.ignore) {
                        _solidColorShader.SetVec3("color", glm::vec3(0, 1, 0));
                        //_cubeMesh.Draw();
                    }
                    else {
                        _solidColorShader.SetBool("uniformColor", true);
                        _solidColorShader.SetVec3("color", probe.color); 
                        _cubeMesh.Draw();
                    }
                }
            }
        }
    }


    static int i = 0;
    if (Input::KeyPressed(HELL_KEY_N)) {
        Audio::PlayAudio("RE_Beep.wav", 0.25f);
        i--;
    }
    if (Input::KeyPressed(HELL_KEY_M)) {
        Audio::PlayAudio("RE_Beep.wav", 0.25f);
        i++;
    }
            
  //  glEnable(GL_DEPTH_TEST);
    glDisable(GL_DEPTH_TEST);
    _solidColorShader.Use();
    _solidColorShader.SetMat4("model", glm::mat4(1));
    _solidColorShader.SetBool("uniformColor", false);
    RenderImmediate();

    glDisable(GL_DEPTH_TEST);
 
    if (_toggles.drawLines) {
        for (Line& line : Scene::_worldLines) {
            //QueueLineForDrawing(line);
        }
        for (Triangle& tri : Scene::_triangleWorld) {
            tri.color = YELLOW;
            //QueueTriangleForLineRendering(tri);
        }
    }

    if (_mode == RenderMode::POINT_CLOUD) {
        // Draw all relevant cloud points
        for (auto& cloudPoint : Scene::_cloudPoints) {
            if (glm::length(cloudPoint.directLighting) > 0.01) {
                Point p;
                p.pos = cloudPoint.position;
                p.color = cloudPoint.directLighting;
                QueuePointForDrawing(p);
            }
        }
    }

    if (_toggles.drawLines) {
        RayCastResult cameraRayData;
        glm::vec3 rayOrigin = Player::GetViewPos();
        glm::vec3 rayDirection = Player::GetCameraFront() * glm::vec3(-1);
        Util::EvaluateRaycasts(rayOrigin, rayDirection, 9999, Scene::_triangleWorld, RaycastObjectType::FLOOR, glm::mat4(1), cameraRayData);
        if (cameraRayData.found) {
            cameraRayData.triangle.color = YELLOW;
            glDisable(GL_DEPTH_TEST);
            QueueTriangleForLineRendering(cameraRayData.triangle);
        }

       //  std::cout << cameraRayData.rayCount << "\n";
    }
        RenderImmediate();
   


    if (_toggles.drawLines) {
        // Prep
        _solidColorShader.Use();
        _solidColorShader.SetMat4("projection", projection);
        _solidColorShader.SetMat4("view", view);
        _solidColorShader.SetMat4("model", glm::mat4(1));
        _solidColorShader.SetBool("uniformColor", false);
        _solidColorShader.SetBool("uniformColor", false);
        _solidColorShader.SetVec3("color", glm::vec3(1, 0, 1));
        glDisable(GL_DEPTH_TEST);
        RenderImmediate();
    } 
    


    //_toggles.drawLines = true;
 //   TextBlitter::_debugTextToBilt += "View Matrix\n";
 //   TextBlitter::_debugTextToBilt += Util::Mat4ToString(Player::GetViewMatrix()) + "\n" + "\n";
    //TextBlitter::_debugTextToBilt += "Inverse View Matrix\n";
    //TextBlitter::_debugTextToBilt += Util::Mat4ToString(Player::GetInverseViewMatrix()) + "\n" + "\n";
    TextBlitter::_debugTextToBilt += "View pos: " + Util::Vec3ToString(Player::GetViewPos()) + "\n";
    TextBlitter::_debugTextToBilt += "View rot: " + Util::Vec3ToString(Player::GetViewRotation()) + "\n";
 //   TextBlitter::_debugTextToBilt += "Forward: " + Util::Vec3ToString(Player::GetCameraFront()) + "\n";
  //  TextBlitter::_debugTextToBilt += "Up: " + Util::Vec3ToString(Player::GetCameraUp()) + "\n";
 //   TextBlitter::_debugTextToBilt += "Right: " + Util::Vec3ToString(Player::GetCameraRight()) + "\n" + "\n";
  //  TextBlitter::_debugTextToBilt += "Total faces: " + std::to_string(::VoxelWorld::GetTotalVoxelFaceCount()) + "\n" + "\n";

  


    // Render shadowmaps
    _shadowMapShader.Use();
    _shadowMapShader.SetFloat("far_plane", SHADOW_FAR_PLANE);
    _shadowMapShader.SetMat4("model", glm::mat4(1));
    glDepthMask(true);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);

    for (int i = 0; i < lights.size(); i++) {            
        glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
        glBindFramebuffer(GL_FRAMEBUFFER, _shadowMaps[i]._ID);
        glEnable(GL_DEPTH_TEST);
        glClear(GL_DEPTH_BUFFER_BIT);
        std::vector<glm::mat4> projectionTransforms; 
        glm::vec3 position = glm::vec3(lights[i].x, lights[i].y, lights[i].z) * _voxelSize2;
        glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), (float)SHADOW_MAP_SIZE / (float)SHADOW_MAP_SIZE, SHADOW_NEAR_PLANE, SHADOW_FAR_PLANE);
        projectionTransforms.clear();
        projectionTransforms.push_back(shadowProj * glm::lookAt(position, position + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
        projectionTransforms.push_back(shadowProj * glm::lookAt(position, position + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
        projectionTransforms.push_back(shadowProj * glm::lookAt(position, position + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
        projectionTransforms.push_back(shadowProj * glm::lookAt(position, position + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)));
        projectionTransforms.push_back(shadowProj * glm::lookAt(position, position + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
        projectionTransforms.push_back(shadowProj * glm::lookAt(position, position + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
        _shadowMapShader.SetMat4("shadowMatrices[0]", projectionTransforms[0]);
        _shadowMapShader.SetMat4("shadowMatrices[1]", projectionTransforms[1]);
        _shadowMapShader.SetMat4("shadowMatrices[2]", projectionTransforms[2]);
        _shadowMapShader.SetMat4("shadowMatrices[3]", projectionTransforms[3]);
        _shadowMapShader.SetMat4("shadowMatrices[4]", projectionTransforms[4]);
        _shadowMapShader.SetMat4("shadowMatrices[5]", projectionTransforms[5]);
        _shadowMapShader.SetVec3("lightPosition", position);
        _shadowMapShader.SetMat4("model", glm::mat4(1));
        DrawScene(_shadowMapShader);
        //DrawAnimatedScene(_shadowMapShader);
    }

    // Composite pass
    _gBuffer.Bind();
    glDrawBuffer(GL_COLOR_ATTACHMENT2);
    glDisable(GL_DEPTH_TEST);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _gBuffer._gAlbedoTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, _gBuffer._gVelocityMapTexture);
    glActiveTexture(GL_TEXTURE2);
    glViewport(0, 0, _gBuffer.GetWidth(), _gBuffer.GetHeight());
    _compositeShader.Use();
    DrawFullscreenQuad();

    // Blit image back to a smaller FBO
    glBindFramebuffer(GL_READ_FRAMEBUFFER, _gBuffer.GetID());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _gBufferFinalSize.GetID());
    glReadBuffer(GL_COLOR_ATTACHMENT2);
    glBlitFramebuffer(0, 0, _gBuffer.GetWidth(), _gBuffer.GetHeight(), 0, 0, _gBufferFinalSize.GetWidth(), _gBufferFinalSize.GetHeight(), GL_COLOR_BUFFER_BIT, GL_LINEAR);

    // Render UI
    QueueUIForRendering("CrosshairDot", _gBufferFinalSize.GetWidth() / 2, _gBufferFinalSize.GetHeight() / 2, true);
    glBindFramebuffer(GL_FRAMEBUFFER, _gBufferFinalSize.GetID());
    glViewport(0, 0, _gBufferFinalSize.GetWidth(), _gBufferFinalSize.GetHeight());
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    Renderer::RenderUI();

    // Blit that smaller FBO into the main frame buffer
    glBindFramebuffer(GL_READ_FRAMEBUFFER, _gBufferFinalSize.GetID());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, _gBufferFinalSize.GetWidth(), _gBufferFinalSize.GetHeight(), 0, 0, GL::GetWindowWidth(), GL::GetWindowHeight(), GL_COLOR_BUFFER_BIT, GL_NEAREST);



    const int bufferSize = MAP_WIDTH * MAP_HEIGHT * MAP_DEPTH * sizeof(float) * 4;
    static float buffer[bufferSize];

    static GLuint pboID = 0;
    if (pboID == 0) {
        glGenBuffers(1, &pboID);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, pboID);
        glBufferData(GL_PIXEL_PACK_BUFFER, bufferSize, NULL, GL_STREAM_READ);

    };

    if (Input::KeyPressed(HELL_KEY_Z) || true) {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);

        glBindBuffer(GL_PIXEL_PACK_BUFFER, pboID);
        glBindTexture(GL_TEXTURE_3D, _imageStoreTexture);
        glGetTexImage(GL_TEXTURE_3D, 0, GL_RGBA, GL_FLOAT, 0);

        GLsync sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        glClientWaitSync(sync, 0, GL_TIMEOUT_IGNORED);
        glDeleteSync(sync);

        GLfloat* ptr = (GLfloat*)glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, bufferSize, GL_MAP_READ_BIT);


        //float* buffer = new float[bufferSize];
        memcpy(&buffer, ptr, bufferSize);

        glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);       
    }
}

void Renderer::HotloadShaders() {
    std::cout << "Hotloaded shaders\n";
    _testShader.Load("test.vert", "test.frag");
    _solidColorShader.Load("solid_color.vert", "solid_color.frag");
    _UIShader.Load("ui.vert", "ui.frag");
    _editorSolidColorShader.Load("editor_solid_color.vert", "editor_solid_color.frag");
    _compositeShader.Load("composite.vert", "composite.frag");
    _velocityMapShader.Load("velocity_map.vert", "velocity_map.frag");
}

Texture3D& Renderer::GetIndirectLightingTexture() {
    return _indirectLightingTexture;
}

void QueueEditorGridSquareForDrawing(int x, int z, glm::vec3 color) {
    Triangle triA;
    Triangle triB;
    triA.p3 = Editor::GetEditorWorldPosFromCoord(x, z);;
    triA.p2 = Editor::GetEditorWorldPosFromCoord(x + 1, z);
    triA.p1 = Editor::GetEditorWorldPosFromCoord(x + 1, z + 1);
    triB.p1 = Editor::GetEditorWorldPosFromCoord(x, z + 1);;
    triB.p2 = Editor::GetEditorWorldPosFromCoord(x + 1, z + 1);
    triB.p3 = Editor::GetEditorWorldPosFromCoord(x, z);
    triA.color = color;
    triB.color = color;
    QueueTriangleForSolidRendering(triA);
    QueueTriangleForSolidRendering(triB);
}

/*void Renderer::RenderEditorFrame() {

    _gBuffer.Bind();
    _gBuffer.EnableAllDrawBuffers();
    
    glViewport(0, 0, _renderWidth, _renderHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    float _zoom = 100.0f;
    float width = (float) _renderWidth / _zoom;
    float height = (float) _renderHeight / _zoom;
    glm::mat4 projection = glm::ortho(-width/2, width/2, -height/2, height/2, 0.1f, 100.0f);
      
    // Draw grid
    for (int x = 0; x <= WORLD_WIDTH; x++) {
        QueueLineForDrawing(Line(glm::vec3(x * WORLD_GRID_SPACING, 0, 0), glm::vec3(x * WORLD_GRID_SPACING, 0, WORLD_DEPTH * WORLD_GRID_SPACING), GRID_COLOR));
    }
    for (int z = 0; z <= WORLD_DEPTH; z++) {
        QueueLineForDrawing(Line(glm::vec3(0, 0, z * WORLD_GRID_SPACING), glm::vec3(WORLD_WIDTH * WORLD_GRID_SPACING, 0, z * WORLD_GRID_SPACING), GRID_COLOR));
    }

    for (int x = 0; x < WORLD_WIDTH; x++) {
        for (int z = 0; z < WORLD_DEPTH; z++) {
            if (Editor::CooridnateIsWall(x, z))
                QueueEditorGridSquareForDrawing(x, z, WHITE);
        }
    }

 //   mouseGridX += _cameraX ;


    //_mouseX -= (_cameraX / _zoom);
   QueueEditorGridSquareForDrawing(Editor::GetMouseGridX(), Editor::GetMouseGridZ(), YELLOW);


    _editorSolidColorShader.Use();
    _editorSolidColorShader.SetMat4("projection", projection);
    _editorSolidColorShader.SetBool("uniformColor", false);
    _editorSolidColorShader.SetFloat("viewportWidth", _renderWidth);
    _editorSolidColorShader.SetFloat("viewportHeight", _renderHeight);
    _editorSolidColorShader.SetFloat("cameraX", Editor::GetCameraGridX());
    _editorSolidColorShader.SetFloat("cameraZ", Editor::GetCameraGridZ());
    glDisable(GL_DEPTH_TEST);
    RenderImmediate();


    //QueueUIForRendering("CrosshairDot", Editor::GetMouseScreenX(), Editor::GetMouseScreenZ(), true);
    TextBlitter::_debugTextToBilt = "Mouse: " + std::to_string(Editor::GetMouseScreenX()) + ", " + std::to_string(Editor::GetMouseScreenZ()) + "\n";
    TextBlitter::_debugTextToBilt += "World: " + std::format("{:.2f}", (Editor::GetMouseWorldX()))+ ", " + std::format("{:.2f}", Editor::GetMouseWorldZ()) + "\n";
    TextBlitter::_debugTextToBilt += "Grid: " + std::to_string(Editor::GetMouseGridX()) + ", " + std::to_string(Editor::GetMouseGridZ()) + "\n";
    TextBlitter::_debugTextToBilt += "Cam: " + std::to_string(Editor::GetCameraGridX()) + ", " + std::to_string(Editor::GetCameraGridZ()) + "\n";
    Renderer::RenderUI();

    // Blit image back to frame buffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, _gBuffer.GetID());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, _gBuffer.GetWidth(), _gBuffer.GetHeight(), 0, 0, GL::GetWindowWidth(), GL::GetWindowHeight(), GL_COLOR_BUFFER_BIT, GL_NEAREST);
}*/

void Renderer::WipeShadowMaps() {

    for (ShadowMap& shadowMap : _shadowMaps) {
        shadowMap.Clear();
    }
}

void Renderer::ToggleDrawingLights() {
    _toggles.drawLights = !_toggles.drawLights;
}
void Renderer::ToggleDrawingProbes() {
    _toggles.drawProbes = !_toggles.drawProbes;
}
void Renderer::ToggleDrawingLines() {
    _toggles.drawLines = !_toggles.drawLines;
}

static Mesh _quadMesh;

inline void DrawQuad(int textureWidth, int textureHeight, int xPosition, int yPosition, bool centered = false, float scale = 1.0f, int xSize = -1, int ySize = -1, int xClipMin = -1, int xClipMax = -1, int yClipMin = -1, int yClipMax = -1) {

    float quadWidth = xSize;
    float quadHeight = ySize;
    if (xSize == -1) {
        quadWidth = textureWidth;
    }
    if (ySize == -1) {
        quadHeight = textureHeight;
    }
    if (centered) {
        xPosition -= quadWidth / 2;
        yPosition -= quadHeight / 2;
    }
    float renderTargetWidth = _renderWidth * 0.5f;
    float renderTargetHeight = _renderHeight * 0.5f;

    float width = (1.0f / renderTargetWidth) * quadWidth * scale;
    float height = (1.0f / renderTargetHeight) * quadHeight * scale;
    float ndcX = ((xPosition + (quadWidth / 2.0f)) / renderTargetWidth) * 2 - 1;
    float ndcY = ((yPosition + (quadHeight / 2.0f)) / renderTargetHeight) * 2 - 1;
    Transform transform;
    transform.position.x = ndcX;
    transform.position.y = ndcY * -1;
    transform.scale = glm::vec3(width, height * -1, 1);
    
    _UIShader.SetMat4("model", transform.to_mat4());
    _quadMesh.Draw();
}

void Renderer::QueueUIForRendering(std::string textureName, int screenX, int screenY, bool centered) {
    UIRenderInfo info;
    info.textureName = textureName;
    info.screenX = screenX;
    info.screenY = screenY;
    info.centered = centered;
    _UIRenderInfos.push_back(info);
}
void Renderer::QueueUIForRendering(UIRenderInfo renderInfo) {
    _UIRenderInfos.push_back(renderInfo);
}

void Renderer::RenderUI() {

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);
    _UIShader.Use();

    if (_quadMesh.GetIndexCount() == 0) {
        Vertex vertA, vertB, vertC, vertD;
        vertA.position = { -1.0f, -1.0f, 0.0f };
        vertB.position = { -1.0f, 1.0f, 0.0f };
        vertC.position = { 1.0f,  1.0f, 0.0f };
        vertD.position = { 1.0f,  -1.0f, 0.0f };
        vertA.uv = { 0.0f, 0.0f };
        vertB.uv = { 0.0f, 1.0f };
        vertC.uv = { 1.0f, 1.0f };
        vertD.uv = { 1.0f, 0.0f };
        std::vector<Vertex> vertices;
        vertices.push_back(vertA);
        vertices.push_back(vertB);
        vertices.push_back(vertC);
        vertices.push_back(vertD);
        std::vector<uint32_t> indices = { 0, 1, 2, 0, 2, 3 };
        _quadMesh = Mesh(vertices, indices, "QuadMesh");
    }

    for (UIRenderInfo& uiRenderInfo : _UIRenderInfos) {
        AssetManager::GetTexture(uiRenderInfo.textureName).Bind(0);
        Texture& texture = AssetManager::GetTexture(uiRenderInfo.textureName);
        DrawQuad(texture.GetWidth(), texture.GetHeight(), uiRenderInfo.screenX, uiRenderInfo.screenY, uiRenderInfo.centered);
    }

    _UIRenderInfos.clear();
    glDisable(GL_BLEND);
}

int Renderer::GetRenderWidth() {
    return _renderWidth;
}

int Renderer::GetRenderHeight() {
    return _renderHeight;
}

void Renderer::NextMode() {
    _mode = (RenderMode)(int(_mode) + 1);
    if (_mode == MODE_COUNT)
        _mode = (RenderMode)0;
}

void Renderer::PreviousMode() {
    if (int(_mode) == 0)
        _mode = RenderMode(int(MODE_COUNT) - 1);
    else
        _mode = (RenderMode)(int(_mode) - 1);
}

// Local functions

void QueueLineForDrawing(Line line) {
    _lines.push_back(line);
}

void QueuePointForDrawing(Point point) {
    _points.push_back(point);
}

void QueueTriangleForLineRendering(Triangle& triangle) {
    _lines.push_back(Line(triangle.p1, triangle.p2, triangle.color));
    _lines.push_back(Line(triangle.p2, triangle.p3, triangle.color));
    _lines.push_back(Line(triangle.p3, triangle.p1, triangle.color));
}

void QueueTriangleForSolidRendering(Triangle& triangle) {
    _solidTrianglePoints.push_back(Point(triangle.p1, triangle.color));
    _solidTrianglePoints.push_back(Point(triangle.p2, triangle.color));
    _solidTrianglePoints.push_back(Point(triangle.p3, triangle.color));
}

void DrawFullscreenQuad() {
    static GLuint vao = 0;
    if (vao == 0) {
        float vertices[] = {
            // positions         texcoords
            -1.0f,  1.0f, 0.0f,  0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f,  0.0f, 0.0f,
             1.0f,  1.0f, 0.0f,  1.0f, 1.0f,
             1.0f, -1.0f, 0.0f,  1.0f, 0.0f,
        };
        unsigned int vbo;
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}
