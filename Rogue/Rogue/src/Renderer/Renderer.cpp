#include "Renderer.h"
#include "GBuffer.h"
#include "PresentFrameBuffer.h"
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
#include "NumberBlitter.h"

#include <vector>
#include <cstdlib>
#include <format>

#include "../Core/AnimatedGameObject.h"
#include "../Effects/MuzzleFlash.h"

struct Shaders {
    Shader solidColor;
    Shader shadowMap;
    Shader UI;
    Shader editorSolidColor;
    Shader composite;
    Shader velocityMap;
    Shader fxaa;
    Shader animatedQuad;
    Shader depthOfField;
    Shader debugViewPointCloud;
    Shader debugViewPropgationGrid;
    Shader geometry;
    Shader lighting;
    ComputeShader compute;
    ComputeShader pointCloud;
    ComputeShader propogateLight;
} _shaders;

struct SSBOs {
    GLuint staticVertices = 0;;
    GLuint vertexCount = 0;;
} _ssbos;

struct PointCloud {
    GLuint VAO{ 0 };
    GLuint VBO{ 0 };
    int vertexCount{ 0 };
} _pointCloud;

struct Toggles {
    bool drawLights = true;
    bool drawProbes = false;
    bool drawLines = false;
} _toggles;

GLuint _progogationGridTexture = 0;
Mesh _cubeMesh;
GBuffer _gBuffer;
PresentFrameBuffer _presentFrameBuffer;
unsigned int _pointLineVAO;
unsigned int _pointLineVBO;
int _renderWidth = 768;// 512 * 1.5f;
int _renderHeight = 432;// 288 * 1.5f;

std::vector<Point> _points;
std::vector<Point> _solidTrianglePoints;
std::vector<Line> _lines;
std::vector<UIRenderInfo> _UIRenderInfos;
std::vector<ShadowMap> _shadowMaps;
GLuint _imageStoreTexture = { 0 };
bool _depthOfFieldScene = 0.9f;
bool _depthOfFieldWeapon = 1.0f;
float _propogationGridSpacing = 0.4;
float  _pointCloudSpacing = 0.4;
float _mapWidth = 24;
float _mapHeight = 8;
float _mapDepth = 24;

enum RenderMode { COMPOSITE, DIRECT_LIGHT, INDIRECT_LIGHT, POINT_CLOUD,  MODE_COUNT} _mode;

void QueueLineForDrawing(Line line);
void QueuePointForDrawing(Point point);
void QueueTriangleForLineRendering(Triangle& triangle);
void QueueTriangleForSolidRendering(Triangle& triangle);
void DrawScene(Shader& shader);
void DrawAnimatedScene(Shader& shader);
void DrawShadowMapScene(Shader& shader);
void DrawFullscreenQuad();
void DrawMuzzleFlashes();
void InitCompute();
void ComputePass();
void RenderShadowMaps();
void UpdatePointCloudLighting();
void UpdatePropogationgGrid();
void DrawPointCloud();
void GeometryPass();
void LightingPass();
void DebugPass();
void RenderImmediate();

void Renderer::Init() {

    Scene::Init();

    glGenVertexArrays(1, &_pointLineVAO);
    glGenBuffers(1, &_pointLineVBO);
    glPointSize(2);
        
    _shaders.solidColor.Load("solid_color.vert", "solid_color.frag");
    _shaders.shadowMap.Load("shadowmap.vert", "shadowmap.frag", "shadowmap.geom");
    _shaders.UI.Load("ui.vert", "ui.frag");
    _shaders.editorSolidColor.Load("editor_solid_color.vert", "editor_solid_color.frag");
    _shaders.composite.Load("composite.vert", "composite.frag");
    _shaders.velocityMap.Load("velocity_map.vert", "velocity_map.frag");
    _shaders.fxaa.Load("fxaa.vert", "fxaa.frag");
    _shaders.animatedQuad.Load("animated_quad.vert", "animated_quad.frag");
    _shaders.depthOfField.Load("depth_of_field.vert", "depth_of_field.frag");
    _shaders.debugViewPointCloud.Load("debug_view_point_cloud.vert", "debug_view_point_cloud.frag");
    _shaders.geometry.Load("geometry.vert", "geometry.frag");
    _shaders.lighting.Load("lighting.vert", "lighting.frag");
    _shaders.debugViewPropgationGrid.Load("debug_view_propogation_grid.vert", "debug_view_propogation_grid.frag");
    
    _cubeMesh = MeshUtil::CreateCube(1.0f, 1.0f, true);

    RecreateFrameBuffers();

    _shadowMaps.push_back(ShadowMap());
    _shadowMaps.push_back(ShadowMap());
    _shadowMaps.push_back(ShadowMap());
    _shadowMaps.push_back(ShadowMap());

    for (ShadowMap& shadowMap : _shadowMaps) {
        shadowMap.Init();
    }

    InitCompute();
}

void Renderer::RenderFrame() {

    ComputePass(); // Fills the indirect lighting data structures
    RenderShadowMaps();
    GeometryPass();
    LightingPass();
    DrawMuzzleFlashes();
    
    // Propgation Grid
    if (_toggles.drawProbes) {
        Transform cubeTransform;
        cubeTransform.scale = glm::vec3(0.025);
        _shaders.debugViewPropgationGrid.Use();
        _shaders.debugViewPropgationGrid.SetMat4("projection", Player::GetProjectionMatrix(_depthOfFieldScene));
        _shaders.debugViewPropgationGrid.SetMat4("view", Player::GetViewMatrix());
        _shaders.debugViewPropgationGrid.SetMat4("model", cubeTransform.to_mat4());
        _shaders.debugViewPropgationGrid.SetFloat("propogationGridSpacing", _propogationGridSpacing);
        _shaders.debugViewPropgationGrid.SetInt("propogationTextureWidth", _mapWidth / _propogationGridSpacing);
        _shaders.debugViewPropgationGrid.SetInt("propogationTextureHeight", _mapHeight / _propogationGridSpacing);
        _shaders.debugViewPropgationGrid.SetInt("propogationTextureDepth", _mapDepth / _propogationGridSpacing);
        int count = _mapWidth * _mapHeight * _mapDepth / _propogationGridSpacing / _propogationGridSpacing / _propogationGridSpacing;
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_3D, _progogationGridTexture);
        glBindVertexArray(_cubeMesh.GetVAO());
        glDrawElementsInstanced(GL_TRIANGLES, _cubeMesh.GetIndexCount(), GL_UNSIGNED_INT, 0, count);
    }

    // Blit final image from large FBO down into a smaller FBO
    glBindFramebuffer(GL_READ_FRAMEBUFFER, _gBuffer.GetID());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _presentFrameBuffer.GetID());
    glReadBuffer(GL_COLOR_ATTACHMENT3);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glBlitFramebuffer(0, 0, _gBuffer.GetWidth(), _gBuffer.GetHeight(), 0, 0, _presentFrameBuffer.GetWidth(), _presentFrameBuffer.GetHeight(), GL_COLOR_BUFFER_BIT, GL_LINEAR);   

    // FXAA on that smaller FBO
    _shaders.fxaa.Use();
    _shaders.fxaa.SetFloat("viewportWidth", _presentFrameBuffer.GetWidth());
    _shaders.fxaa.SetFloat("viewportHeight", _presentFrameBuffer.GetHeight());
    _presentFrameBuffer.Bind();
    glViewport(0, 0, _presentFrameBuffer.GetWidth(), _presentFrameBuffer.GetHeight());
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _presentFrameBuffer._inputTexture);
    glDrawBuffer(GL_COLOR_ATTACHMENT1);
    glDisable(GL_DEPTH_TEST);
    DrawFullscreenQuad();

    // Render any debug shit, like the point cloud, progation grid, misc points, lines, etc
    DebugPass();

    // Render UI
    _presentFrameBuffer.Bind();
    glDrawBuffer(GL_COLOR_ATTACHMENT1);
    QueueUIForRendering("CrosshairDot", _presentFrameBuffer.GetWidth() / 2, _presentFrameBuffer.GetHeight() / 2, true);
    TextBlitter::_debugTextToBilt += "View pos: " + Util::Vec3ToString(Player::GetViewPos()) + "\n";
    TextBlitter::_debugTextToBilt += "View rot: " + Util::Vec3ToString(Player::GetViewRotation()) + "\n";
    glBindFramebuffer(GL_FRAMEBUFFER, _presentFrameBuffer.GetID());
    glViewport(0, 0, _presentFrameBuffer.GetWidth(), _presentFrameBuffer.GetHeight());
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    Renderer::RenderUI();
 
    // Blit that smaller FBO into the main frame buffer
    glBindFramebuffer(GL_READ_FRAMEBUFFER, _presentFrameBuffer.GetID());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glReadBuffer(GL_COLOR_ATTACHMENT1);
    glBlitFramebuffer(0, 0, _presentFrameBuffer.GetWidth(), _presentFrameBuffer.GetHeight(), 0, 0, GL::GetWindowWidth(), GL::GetWindowHeight(), GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

void GeometryPass() {
    glm::mat4 projection = Player::GetProjectionMatrix(_depthOfFieldScene); // 1.0 for weapon, 0.9 for scene.
    glm::mat4 view = Player::GetViewMatrix();
    _gBuffer.Bind();
    unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments);
    glViewport(0, 0, _gBuffer.GetWidth(), _gBuffer.GetHeight());
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    _shaders.geometry.Use();
    _shaders.geometry.SetMat4("projection", projection);
    _shaders.geometry.SetMat4("view", view);
    _shaders.geometry.SetVec3("viewPos", Player::GetViewPos());
    _shaders.geometry.SetVec3("camForward", Player::GetCameraFront());
    DrawScene(_shaders.geometry);
    DrawAnimatedScene(_shaders.geometry);
}

void LightingPass() {

    _shaders.lighting.Use();

    // Debug Text
    TextBlitter::_debugTextToBilt = "";
    if (_mode == RenderMode::COMPOSITE) {
        TextBlitter::_debugTextToBilt = "Mode: COMPOSITE\n";
        _shaders.lighting.SetInt("mode", 0);
    }
    if (_mode == RenderMode::POINT_CLOUD) {
        TextBlitter::_debugTextToBilt = "Mode: POINT CLOUD\n";
        _shaders.lighting.SetInt("mode", 1);
    }
    if (_mode == RenderMode::DIRECT_LIGHT) {
        TextBlitter::_debugTextToBilt = "Mode: DIRECT LIGHT\n";
        _shaders.lighting.SetInt("mode", 2);
    }
    if (_mode == RenderMode::INDIRECT_LIGHT) {
        TextBlitter::_debugTextToBilt = "Mode: INDIRECT LIGHT\n";
        _shaders.lighting.SetInt("mode", 3);
    }
  
    _gBuffer.Bind();
    glDrawBuffer(GL_COLOR_ATTACHMENT3);
    glViewport(0, 0, _gBuffer.GetWidth(), _gBuffer.GetHeight());
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _gBuffer._gBaseColorTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, _gBuffer._gNormalTexture);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, _gBuffer._gRMATexture);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, _gBuffer._gDepthTexture);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_3D, _progogationGridTexture);

    // Set potential unused lights to black
    for (int i = 0; i < 4; i++) {
        _shaders.lighting.SetVec3("lightColor[" + std::to_string(i) + "]", BLACK);
    }
    // Update any actual lights
    auto& lights = Scene::_lights;
    for (int i = 0; i < lights.size(); i++) {
        glActiveTexture(GL_TEXTURE5 + i);
        glBindTexture(GL_TEXTURE_CUBE_MAP, _shadowMaps[i]._depthTexture);
        _shaders.lighting.SetVec3("lightPosition[" + std::to_string(i) + "]", lights[i].position);
        _shaders.lighting.SetVec3("lightColor[" + std::to_string(i) + "]", lights[i].color);
        _shaders.lighting.SetFloat("lightRadius[" + std::to_string(i) + "]", lights[i].radius);
        _shaders.lighting.SetFloat("lightStrength[" + std::to_string(i) + "]", lights[i].strength);
    }

    static float time = 0;
    time += 0.01;
    _shaders.lighting.SetMat4("model", glm::mat4(1));
    _shaders.lighting.SetFloat("time", time);
    _shaders.lighting.SetFloat("screenWidth", _gBuffer.GetWidth());
    _shaders.lighting.SetFloat("screenHeight", _gBuffer.GetHeight());
    _shaders.lighting.SetMat4("projectionScene", Player::GetProjectionMatrix(_depthOfFieldScene));
    _shaders.lighting.SetMat4("projectionWeapon", Player::GetProjectionMatrix(_depthOfFieldWeapon));
    _shaders.lighting.SetMat4("inverseProjectionScene", glm::inverse(Player::GetProjectionMatrix(_depthOfFieldScene)));
    _shaders.lighting.SetMat4("inverseProjectionWeapon", glm::inverse(Player::GetProjectionMatrix(_depthOfFieldWeapon)));
    _shaders.lighting.SetMat4("view", Player::GetViewMatrix());
    _shaders.lighting.SetMat4("inverseView", glm::inverse(Player::GetViewMatrix()));
    _shaders.lighting.SetVec3("viewPos", Player::GetViewPos());
    _shaders.lighting.SetFloat("propogationGridSpacing", _propogationGridSpacing);

    DrawFullscreenQuad();
}

void DebugPass() {

    _presentFrameBuffer.Bind();
    glDrawBuffer(GL_COLOR_ATTACHMENT1);
    glViewport(0, 0, _presentFrameBuffer.GetWidth(), _presentFrameBuffer.GetHeight());
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    // Point cloud
    if (_mode == RenderMode::POINT_CLOUD) {
        DrawPointCloud();
    }

    _shaders.solidColor.Use();
    _shaders.solidColor.SetMat4("projection", Player::GetProjectionMatrix(_depthOfFieldScene));
    _shaders.solidColor.SetMat4("view", Player::GetViewMatrix());
    _shaders.solidColor.SetVec3("viewPos", Player::GetViewPos());
    _shaders.solidColor.SetVec3("color", glm::vec3(1, 1, 0));

    // Lights
    if (_toggles.drawLights) {
        for (Light& light : Scene::_lights) {
            glm::vec3 lightCenter = light.position;
            Transform lightTransform;
            lightTransform.scale = glm::vec3(0.2f);
            lightTransform.position = lightCenter;
            _shaders.solidColor.SetMat4("model", lightTransform.to_mat4());
            _shaders.solidColor.SetVec3("color", WHITE);
            _shaders.solidColor.SetBool("uniformColor", true);
            _cubeMesh.Draw();
        }
    }

    // Triangle world as lines
    if (_toggles.drawLines) {
        _shaders.solidColor.Use();
        _shaders.solidColor.SetBool("uniformColor", true);
        _shaders.solidColor.SetVec3("color", YELLOW);
        _shaders.solidColor.SetMat4("model", glm::mat4(1));
        // The static triangle world
        for (int i = 0; i < Scene::_triangleWorld.size(); i++) {
            QueueTriangleForLineRendering(Scene::_triangleWorld[i]);
        }
        // The door that opens
        GameObject* gameObject = Scene::GetGameObjectByName("Door2");
        if (gameObject) {
            for (Triangle& triangle : gameObject->GetTris()) {
                Triangle t;
                t.p1 = gameObject->GetModelMatrix() * glm::vec4(triangle.p1, 1.0);
                t.p2 = gameObject->GetModelMatrix() * glm::vec4(triangle.p2, 1.0);
                t.p3 = gameObject->GetModelMatrix() * glm::vec4(triangle.p3, 1.0);
                t.color = YELLOW;
                QueueTriangleForLineRendering(t);
            }
        }
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        RenderImmediate();
    }    
}

void Renderer::RecreateFrameBuffers() {
    _gBuffer.Configure(_renderWidth * 2, _renderHeight * 2);
    _presentFrameBuffer.Configure(_renderWidth, _renderHeight);
    std::cout << "Render Size: " << _gBuffer.GetWidth() << " x " << _gBuffer.GetHeight() << "\n";
    std::cout << "Present Size: " << _presentFrameBuffer.GetWidth() << " x " << _presentFrameBuffer.GetHeight() << "\n";
}

void DrawScene(Shader& shader) {

    shader.SetMat4("model", glm::mat4(1));

    AssetManager::BindMaterialByIndex(AssetManager::GetMaterialIndex("Ceiling2"));
    for (Wall& wall : Scene::_walls) {
        //AssetManager::BindMaterialByIndex(wall.materialIndex);
        wall.Draw();
    }

    for (Floor& floor : Scene::_floors) {
        AssetManager::BindMaterialByIndex(floor.materialIndex);
        floor.Draw();
    }

    AssetManager::BindMaterialByIndex(AssetManager::GetMaterialIndex("Ceiling"));
    for (Ceiling& ceiling : Scene::_ceilings) {
        //AssetManager::BindMaterialByIndex(ceiling.materialIndex);
        ceiling.Draw();
    }

    static int trimCeilingMaterialIndex = AssetManager::GetMaterialIndex("Trims");
    static int trimFloorMaterialIndex = AssetManager::GetMaterialIndex("Door");

    // Ceiling trims
    AssetManager::BindMaterialByIndex(AssetManager::GetMaterialIndex("Ceiling2"));
    for (Wall& wall : Scene::_walls) {
        if (!wall.hasTopTrim)
            continue;
        Transform t;
        t.position = wall.begin;
        t.position.y = wall.topTrimBottom;
        t.rotation.y = Util::YRotationBetweenTwoPoints(wall.begin, wall.end);
        t.scale.x = glm::distance(wall.end, wall.begin);
        shader.SetMat4("model", t.to_mat4());
        AssetManager::GetModel("TrimCeiling").Draw();
    }
    // Floor trims
    AssetManager::BindMaterialByIndex(trimFloorMaterialIndex);
    for (Wall& wall : Scene::_walls) {
        if (!wall.hasBottomTrim)
            continue;
        Transform t;
        t.position = wall.begin;
        t.rotation.y = Util::YRotationBetweenTwoPoints(wall.begin, wall.end);
        t.scale.x = glm::distance(wall.end, wall.begin) * 0.5f;
        shader.SetMat4("model", t.to_mat4());
        AssetManager::GetModel("TrimFloor").Draw();
    }

    // Render game objects
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

    std::vector<glm::mat4> tempSkinningMats;
    for (unsigned int i = 0; i < animatedGameObject->_animatedTransforms.local.size(); i++) {
        glm::mat4 matrix = animatedGameObject->_animatedTransforms.local[i];
        shader.SetMat4("skinningMats[" + std::to_string(i) + "]", matrix);
        matrix = animatedGameObject->_animatedTransformsPrevious.local[i];
        shader.SetMat4("skinningMatsPrevious[" + std::to_string(i) + "]", matrix);
        tempSkinningMats.push_back(matrix);
    }
    shader.SetMat4("model", animatedGameObject->GetModelMatrix());

    /*
    for (int i = 0; i < animatedGameObject->_animatedTransforms.worldspace.size(); i++) {
        auto& bone = animatedGameObject->_animatedTransforms.worldspace[i];
        glm::mat4 m = animatedGameObject->GetModelMatrix() * bone;
        float x = m[3][0];
        float y = m[3][1];
        float z = m[3][2];
        Point p2(glm::vec3(x, y, z), RED);
        // QueuePointForDrawing(p2);
        if (animatedGameObject->GetName() == "Shotgun") {
            glm::vec3 barrelPosition = animatedGameObject->GetShotgunBarrelPostion();
            Point point = Point(barrelPosition, BLUE);
            //   QueuePointForDrawing(point);
        }
    }*/

    static bool maleHands = true;
    if (Input::KeyPressed(HELL_KEY_U)) {
        maleHands = !maleHands;
    }
    SkinnedModel& skinnedModel = *animatedGameObject->_skinnedModel;
    glBindVertexArray(skinnedModel.m_VAO);
    for (int i = 0; i < skinnedModel.m_meshEntries.size(); i++) {
        AssetManager::BindMaterialByIndex(animatedGameObject->_materialIndices[i]);
        if (maleHands) {
            if (skinnedModel.m_meshEntries[i].Name == "SK_FPSArms_Female.001" ||
                skinnedModel.m_meshEntries[i].Name == "SK_FPSArms_Female") {
                continue;
            }
        }
        else {
            if (skinnedModel.m_meshEntries[i].Name == "manniquen1_2.001" ||
                skinnedModel.m_meshEntries[i].Name == "manniquen1_2") {
                continue;
            }
        }
        glDrawElementsBaseVertex(GL_TRIANGLES, skinnedModel.m_meshEntries[i].NumIndices, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * skinnedModel.m_meshEntries[i].BaseIndex), skinnedModel.m_meshEntries[i].BaseVertex);
    }
}

void DrawAnimatedScene(Shader& shader) {

    shader.Use();
    shader.SetBool("isAnimated", true);

    for (auto& animatedObject : Scene::GetAnimatedGameObjects()) {
        //DrawAnimatedObject(shader, &animatedObject);
    }

    glDisable(GL_CULL_FACE);
    shader.SetFloat("projectionMatrixIndex", 1.0f);
    shader.SetMat4("projection", Player::GetProjectionMatrix(_depthOfFieldWeapon)); // 1.0 for weapon, 0.9 for scene.
    DrawAnimatedObject(shader, &Player::GetFirstPersonWeapon());
    shader.SetFloat("projectionMatrixIndex", 0.0f);
    glEnable(GL_CULL_FACE);

    shader.SetBool("isAnimated", false);
}

void DrawShadowMapScene(Shader& shader) {

    shader.SetMat4("model", glm::mat4(1));
    for (Wall& wall : Scene::_walls) {
        wall.Draw();
    }
    for (Floor& floor : Scene::_floors) {
        floor.Draw();
    }
    for (Ceiling& ceiling : Scene::_ceilings) {
        ceiling.Draw();
    }
    for (GameObject& gameObject : Scene::_gameObjects) {
        shader.SetMat4("model", gameObject.GetModelMatrix());
        for (int i = 0; i < gameObject._meshMaterialIndices.size(); i++) {
            gameObject._model->_meshes[i].Draw();
        }
    }
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

void Renderer::HotloadShaders() {
    std::cout << "Hotloaded shaders\n";
    _shaders.solidColor.Load("solid_color.vert", "solid_color.frag");
    _shaders.UI.Load("ui.vert", "ui.frag");
    _shaders.editorSolidColor.Load("editor_solid_color.vert", "editor_solid_color.frag");
    _shaders.composite.Load("composite.vert", "composite.frag");
    _shaders.velocityMap.Load("velocity_map.vert", "velocity_map.frag");
    _shaders.fxaa.Load("fxaa.vert", "fxaa.frag");
    _shaders.animatedQuad.Load("animated_quad.vert", "animated_quad.frag");
    _shaders.depthOfField.Load("depth_of_field.vert", "depth_of_field.frag");
    _shaders.debugViewPointCloud.Load("debug_view_point_cloud.vert", "debug_view_point_cloud.frag");
    _shaders.geometry.Load("geometry.vert", "geometry.frag");
    _shaders.lighting.Load("lighting.vert", "lighting.frag");
    _shaders.debugViewPropgationGrid.Load("debug_view_propogation_grid.vert", "debug_view_propogation_grid.frag");
    _shaders.compute.Load("res/shaders/compute.comp");
    _shaders.pointCloud.Load("res/shaders/point_cloud.comp");
    _shaders.propogateLight.Load("res/shaders/propogate_light.comp");    
}

void QueueEditorGridSquareForDrawing(int x, int z, glm::vec3 color) {
   /* Triangle triA;
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
    QueueTriangleForSolidRendering(triB);*/
}

void Renderer::RenderEditorFrame() {

    /*_gBuffer.Bind();
    
    
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


    _shaders.editorSolidColor.Use();
    _shaders.editorSolidColor.SetMat4("projection", projection);
    _shaders.editorSolidColor.SetBool("uniformColor", false);
    _shaders.editorSolidColor.SetFloat("viewportWidth", _renderWidth);
    _shaders.editorSolidColor.SetFloat("viewportHeight", _renderHeight);
    _shaders.editorSolidColor.SetFloat("cameraX", Editor::GetCameraGridX());
    _shaders.editorSolidColor.SetFloat("cameraZ", Editor::GetCameraGridZ());
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
    */

 //   _mapWidth* _mapHeight* _mapDepth / _propogationGridSpacing
}

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
    float renderTargetWidth = _renderWidth;// * 0.5f;
    float renderTargetHeight = _renderHeight;// * 0.5f;

    float width = (1.0f / renderTargetWidth) * quadWidth * scale;
    float height = (1.0f / renderTargetHeight) * quadHeight * scale;
    float ndcX = ((xPosition + (quadWidth / 2.0f)) / renderTargetWidth) * 2 - 1;
    float ndcY = ((yPosition + (quadHeight / 2.0f)) / renderTargetHeight) * 2 - 1;
    Transform transform;
    transform.position.x = ndcX;
    transform.position.y = ndcY * -1;
    transform.scale = glm::vec3(width, height * -1, 1);
    //transform.scale.x *= 0.5;
    //transform.scale.y *= 0.5;

    _shaders.UI.SetMat4("model", transform.to_mat4());
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
    _shaders.UI.Use();

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

float Renderer::GetPointCloudSpacing() {
    return _pointCloudSpacing;
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

void DrawMuzzleFlashes() {

    static MuzzleFlash muzzleFlash; // has init on the first use

    // Bail if no flash    
    if (Player::GetMuzzleFlashTime() < 0)
        return;
    if (Player::GetMuzzleFlashTime() > 1)
        return;

    _gBuffer.Bind();
    glDrawBuffer(GL_COLOR_ATTACHMENT3);
    glViewport(0, 0, _gBuffer.GetWidth(), _gBuffer.GetHeight());

    muzzleFlash.m_CurrentTime = Player::GetMuzzleFlashTime();
    glm::vec3 worldPosition = glm::vec3(0);
    if (Player::GetCurrentWeaponIndex() == GLOCK) {
        worldPosition = Player::GetFirstPersonWeapon().GetGlockBarrelPostion();
    }
    else if (Player::GetCurrentWeaponIndex() == AKS74U) {
        worldPosition = Player::GetFirstPersonWeapon().GetAKS74UBarrelPostion();
    }
    else if (Player::GetCurrentWeaponIndex() == SHOTGUN) {
        worldPosition = Player::GetFirstPersonWeapon().GetShotgunBarrelPostion();
    }
    else {
        return;
    }

    Transform t;
    t.position = worldPosition;
    t.rotation = Player::GetViewRotation();

    // draw to lighting shader
    glDrawBuffer( GL_COLOR_ATTACHMENT3);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    /// this is sketchy. add this to player class.
    glm::mat4 projection = Player::GetProjectionMatrix(_depthOfFieldWeapon); // 1.0 for weapon, 0.9 for scene.
    glm::mat4 view = Player::GetViewMatrix();
    glm::vec3 viewPos = Player::GetViewPos();

    _shaders.animatedQuad.Use();
    _shaders.animatedQuad.SetMat4("u_MatrixProjection", projection);
    _shaders.animatedQuad.SetMat4("u_MatrixView", view);
    _shaders.animatedQuad.SetVec3("u_ViewPos", viewPos);

    glActiveTexture(GL_TEXTURE0);
    AssetManager::GetTexture("MuzzleFlash").Bind(0);

    muzzleFlash.Draw(&_shaders.animatedQuad, t, Player::GetMuzzleFlashRotation());
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
}

void RenderShadowMaps() {

    if (!Renderer::_shadowMapsAreDirty)
        return;

    _shaders.shadowMap.Use();
    _shaders.shadowMap.SetFloat("far_plane", SHADOW_FAR_PLANE);
    _shaders.shadowMap.SetMat4("model", glm::mat4(1));
    glDepthMask(true);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);

    // clear all, incase there are less lights now
    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
    for (int i = 0; i < _shadowMaps.size(); i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, _shadowMaps[i]._ID);
        glClear(GL_DEPTH_BUFFER_BIT);
    }

    for (int i = 0; i < Scene::_lights.size(); i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, _shadowMaps[i]._ID);
        std::vector<glm::mat4> projectionTransforms;
        glm::vec3 position = Scene::_lights[i].position;
        glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), (float)SHADOW_MAP_SIZE / (float)SHADOW_MAP_SIZE, SHADOW_NEAR_PLANE, SHADOW_FAR_PLANE);
        projectionTransforms.clear();
        projectionTransforms.push_back(shadowProj * glm::lookAt(position, position + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
        projectionTransforms.push_back(shadowProj * glm::lookAt(position, position + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
        projectionTransforms.push_back(shadowProj * glm::lookAt(position, position + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
        projectionTransforms.push_back(shadowProj * glm::lookAt(position, position + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)));
        projectionTransforms.push_back(shadowProj * glm::lookAt(position, position + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
        projectionTransforms.push_back(shadowProj * glm::lookAt(position, position + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
        _shaders.shadowMap.SetMat4("shadowMatrices[0]", projectionTransforms[0]);
        _shaders.shadowMap.SetMat4("shadowMatrices[1]", projectionTransforms[1]);
        _shaders.shadowMap.SetMat4("shadowMatrices[2]", projectionTransforms[2]);
        _shaders.shadowMap.SetMat4("shadowMatrices[3]", projectionTransforms[3]);
        _shaders.shadowMap.SetMat4("shadowMatrices[4]", projectionTransforms[4]);
        _shaders.shadowMap.SetMat4("shadowMatrices[5]", projectionTransforms[5]);
        _shaders.shadowMap.SetVec3("lightPosition", position);
        _shaders.shadowMap.SetMat4("model", glm::mat4(1));
        DrawShadowMapScene(_shaders.shadowMap);
    }
    //Renderer::_shadowMapsAreDirty = false;
}

void CreatePointCloudBuffer() {
    _pointCloud.vertexCount = Scene::_cloudPoints.size();
    glGenVertexArrays(1, &_pointCloud.VAO);
    glGenBuffers(1, &_pointCloud.VBO);
    glBindVertexArray(_pointCloud.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, _pointCloud.VBO);
    glBufferData(GL_ARRAY_BUFFER, _pointCloud.vertexCount * sizeof(CloudPoint), &Scene::_cloudPoints[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(CloudPoint), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(CloudPoint), (void*)offsetof(CloudPoint, normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(CloudPoint), (void*)offsetof(CloudPoint, directLighting));
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
}

void DrawPointCloud() {
    _shaders.debugViewPointCloud.Use();
    _shaders.debugViewPointCloud.SetMat4("projection", Player::GetProjectionMatrix(_depthOfFieldScene));
    _shaders.debugViewPointCloud.SetMat4("view", Player::GetViewMatrix());
    //glBindBuffer(GL_SHADER_STORAGE_BUFFER, _ssbos.pointCloud);
    //glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, _ssbos.pointCloud);
    glDisable(GL_DEPTH_TEST);
    glBindVertexArray(_pointCloud.VAO);
    glDrawArrays(GL_POINTS, 0, _pointCloud.vertexCount);
    glBindVertexArray(0);
}

void InitCompute() {
    // query limitations
    /*int max_compute_work_group_count[3];
    int max_compute_work_group_size[3];
    int max_compute_work_group_invocations;
    for (int idx = 0; idx < 3; idx++) {
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, idx, &max_compute_work_group_count[idx]);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, idx, &max_compute_work_group_size[idx]);
    }
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &max_compute_work_group_invocations);
    std::cout << "OpenGL Limitations: " << std::endl;
    std::cout << "maximum number of work groups in X dimension " << max_compute_work_group_count[0] << std::endl;
    std::cout << "maximum number of work groups in Y dimension " << max_compute_work_group_count[1] << std::endl;
    std::cout << "maximum number of work groups in Z dimension " << max_compute_work_group_count[2] << std::endl;
    std::cout << "maximum size of a work group in X dimension " << max_compute_work_group_size[0] << std::endl;
    std::cout << "maximum size of a work group in Y dimension " << max_compute_work_group_size[1] << std::endl;
    std::cout << "maximum size of a work group in Z dimension " << max_compute_work_group_size[2] << std::endl;
    std::cout << "Number of invocations in a single local work group that may be dispatched to a compute shader " << max_compute_work_group_invocations << std::endl;
    */

    int width = _mapWidth / _propogationGridSpacing;
    int height = _mapHeight / _propogationGridSpacing;
    int depth = _mapDepth / _propogationGridSpacing;
    glGenTextures(1, &_progogationGridTexture);
    glBindTexture(GL_TEXTURE_3D, _progogationGridTexture);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA16F, width, height, depth, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindImageTexture(1, _progogationGridTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, _progogationGridTexture);

    _shaders.compute.Load("res/shaders/compute.comp");
    _shaders.pointCloud.Load("res/shaders/point_cloud.comp");
    _shaders.propogateLight.Load("res/shaders/propogate_light.comp");

    Scene::CreatePointCloud();
    CreatePointCloudBuffer();

    // Create vertices SSBO
    std::vector<glm::vec4> vertices;
    for (int i = 0; i < Scene::_triangleWorld.size(); i++) {
        vertices.push_back(glm::vec4(Scene::_triangleWorld[i].p1, 0));
        vertices.push_back(glm::vec4(Scene::_triangleWorld[i].p2, 0));
        vertices.push_back(glm::vec4(Scene::_triangleWorld[i].p3, 0));
    }
    GameObject* gameObject = Scene::GetGameObjectByName("Door2");
    if (gameObject) {
        for (Triangle& tri : gameObject->GetTris()) {
            vertices.push_back(glm::vec4(tri.p1, 1));
            vertices.push_back(glm::vec4(tri.p2, 1));
            vertices.push_back(glm::vec4(tri.p3, 1));
        }
    }
    _ssbos.vertexCount = vertices.size();
    glGenBuffers(1, &_ssbos.staticVertices);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _ssbos.staticVertices);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, vertices.size() * sizeof(glm::vec4), &vertices[0], GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
    std::cout << "You are raytracing against " << (vertices.size() / 3) << " tris\n";
    std::cout << "Point cloud has: " << Scene::_cloudPoints.size() << " points\n";
    std::cout << "Propogation grid has : " << (_mapWidth * _mapHeight * _mapDepth / _propogationGridSpacing) << " cells\n";
}

void ComputePass() {
    UpdatePointCloudLighting();
    UpdatePropogationgGrid();
}

void UpdatePointCloudLighting() {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, _ssbos.staticVertices);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, _pointCloud.VBO);
    _shaders.pointCloud.Use();
    _shaders.pointCloud.SetInt("vertexCount", _ssbos.vertexCount);
    _shaders.pointCloud.SetVec3("lightPosition", Scene::_lights[0].position);
    _shaders.pointCloud.SetMat4("doorMatrix", Scene::GetGameObjectByName("Door2")->GetModelMatrix());
    int pointCloudSize = Scene::_cloudPoints.size();
    glDispatchCompute(std::ceil(pointCloudSize / 64.0f), 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void UpdatePropogationgGrid() {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, _ssbos.staticVertices);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, _pointCloud.VBO);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, _progogationGridTexture);
    _shaders.propogateLight.Use();
    _shaders.propogateLight.SetInt("pointCloudSize", Scene::_cloudPoints.size());
    _shaders.propogateLight.SetInt("vertexCount", _ssbos.vertexCount);
    _shaders.propogateLight.SetMat4("doorMatrix", Scene::GetGameObjectByName("Door2")->GetModelMatrix());
    _shaders.propogateLight.SetFloat("propogationGridSpacing", _propogationGridSpacing);

    int textureWidth = _mapWidth / _propogationGridSpacing;
    int textureHeight = _mapHeight / _propogationGridSpacing;
    int textureDepth = _mapDepth / _propogationGridSpacing;

    int xSize = std::ceil(textureWidth / 4.0f);
    int ySize = std::ceil(textureHeight / 4.0f);
    int zSize = std::ceil(textureDepth / 4.0f);
    glDispatchCompute(xSize, ySize, zSize);
    //glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
}