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
#include "../Timer.hpp"

#include <vector>
#include <cstdlib>
#include <format>
#include <future>
#include <algorithm>

#include "../Core/AnimatedGameObject.h"
#include "../Effects/MuzzleFlash.h"

struct Shaders {
    Shader solidColor;
    Shader shadowMap;
    Shader UI;
    Shader editorSolidColor;
    Shader editorTextured;
    Shader composite;
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
    //ComputeShader propogationList;
    //ComputeShader calculateIndirectDispatchSize;
} _shaders;

struct SSBOs {
    GLuint rtVertices = 0;
    GLuint rtMesh = 0;
    GLuint rtInstances = 0;
    GLuint dirtyPointCloudIndices = 0;
    //GLuint dirtyGridChunks = 0; // 4x4x4
   // GLuint atomicCounter = 0;
    //GLuint propogationList = 0; // contains coords of all dirty grid points
   // GLuint indirectDispatchSize = 0;
   // GLuint floorVertices = 0;
    GLuint dirtyGridCoordinates = 0;
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
float _propogationGridSpacing = 0.4f;
float _pointCloudSpacing = 0.4f;
float _maxPropogationDistance = 2.6f;
float _mapWidth = 16;
float _mapHeight = 8;
float _mapDepth = 16; 
std::vector<int> _dirtyPointCloudIndices;
std::vector<int> _newDirtyPointCloudIndices;
int _floorVertexCount;

const int _gridTextureWidth = _mapWidth / _propogationGridSpacing;
const int _gridTextureHeight = _mapHeight / _propogationGridSpacing;
const int _gridTextureDepth = _mapDepth / _propogationGridSpacing;
const int _gridTextureSize = _gridTextureWidth * _gridTextureHeight * _gridTextureDepth;
const int xSize = std::ceil(_gridTextureWidth * 0.25f);
const int ySize = std::ceil(_gridTextureHeight * 0.25f);
const int zSize = std::ceil(_gridTextureDepth * 0.25f);

ThreadPool dirtyUpdatesPool(4);
ThreadPool gridIndicesPool(1);

std::vector<glm::uvec4> _gridIndices;
std::vector<glm::uvec4> _newGridIndices;
static std::vector<glm::uvec4> _probeCoordsWithinMapBounds;

enum RenderMode { COMPOSITE, DIRECT_LIGHT, INDIRECT_LIGHT, POINT_CLOUD,  MODE_COUNT} _mode;

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
void FindProbeCoordsWithinMapBounds();
std::vector<glm::uvec4> GridIndicesUpdate(std::vector<glm::uvec4> allGridIndicesWithinRooms);

void Renderer::Init() {

    glGenVertexArrays(1, &_pointLineVAO);
    glGenBuffers(1, &_pointLineVBO);
    glPointSize(2);
        
    _shaders.solidColor.Load("solid_color.vert", "solid_color.frag");
    _shaders.shadowMap.Load("shadowmap.vert", "shadowmap.frag", "shadowmap.geom");
    _shaders.UI.Load("ui.vert", "ui.frag");
    _shaders.editorSolidColor.Load("editor_solid_color.vert", "editor_solid_color.frag");
    _shaders.composite.Load("composite.vert", "composite.frag");
    _shaders.fxaa.Load("fxaa.vert", "fxaa.frag");
    _shaders.animatedQuad.Load("animated_quad.vert", "animated_quad.frag");
    _shaders.depthOfField.Load("depth_of_field.vert", "depth_of_field.frag");
    _shaders.debugViewPointCloud.Load("debug_view_point_cloud.vert", "debug_view_point_cloud.frag");
    _shaders.geometry.Load("geometry.vert", "geometry.frag");
    _shaders.lighting.Load("lighting.vert", "lighting.frag");
    _shaders.debugViewPropgationGrid.Load("debug_view_propogation_grid.vert", "debug_view_propogation_grid.frag");
    _shaders.editorTextured.Load("editor_textured.vert", "editor_textured.frag");
    

    _cubeMesh = MeshUtil::CreateCube(1.0f, 1.0f, true);

    RecreateFrameBuffers();

    /*
    _shadowMaps.push_back(ShadowMap());
    _shadowMaps.push_back(ShadowMap());
    _shadowMaps.push_back(ShadowMap());
    _shadowMaps.push_back(ShadowMap());
    */

    for(int i = 0; i < 16; i++) {
        _shadowMaps.emplace_back();
    }

    for (ShadowMap& shadowMap : _shadowMaps) {
        shadowMap.Init();
    }

    FindProbeCoordsWithinMapBounds();

    InitCompute();
}

std::vector<int> Renderer::UpdateDirtyPointCloudIndices() {

    std::vector<int> result;
    result.reserve(Scene::_cloudPoints.size());

    // If the area within the lights radius has been modified, queue all the relevant cloud points
    for (int j = 0; j < Scene::_cloudPoints.size(); j++) {
        CloudPoint& cloudPoint = Scene::_cloudPoints[j];
        for (auto& light : Scene::_lights) {
            if (light.isDirty) {
                if (Util::DistanceSquared(cloudPoint.position, light.position) < light.radius * light.radius) {
                    result.push_back(j);
                    break;
                }
            }
        }
    }
    return result;
}


void Renderer::RenderFrame() {


    //Timer t{ "RenderFrame" };

    if (Input::KeyPressed(HELL_KEY_T)) {
        for (Light& light : Scene::_lights) {
            light.isDirty = true;
        }
    }   
    if (Input::KeyPressed(HELL_KEY_C)) {
        Scene::_lights[1].isDirty = true;
    }





    if(_dirtyPointCloudIndices.capacity() != Scene::_cloudPoints.size())
        _dirtyPointCloudIndices.reserve(Scene::_cloudPoints.size());


    _dirtyPointCloudIndices = _newDirtyPointCloudIndices;
    
	dirtyUpdatesPool.addTask([]() {
		_newDirtyPointCloudIndices = Renderer::UpdateDirtyPointCloudIndices();
	});


    if (_newGridIndices.capacity() != _gridTextureSize)
        _newGridIndices.reserve(_gridTextureSize);
    if (_gridIndices.capacity() != _gridTextureSize)
        _gridIndices.reserve(_gridTextureSize);

    gridIndicesPool.pause();
    _gridIndices = _newGridIndices;

    gridIndicesPool.addTask([]() {
        auto a = GridIndicesUpdate(_probeCoordsWithinMapBounds);
        if (a.size() > 0)
            _newGridIndices = a;
    });

    gridIndicesPool.unpause();





    ComputePass(); // Fills the indirect lighting data structures
    RenderShadowMaps();
    GeometryPass();
    LightingPass();
    DrawMuzzleFlashes();
    
    // Propgation Grid
    if (_toggles.drawProbes) {
        Transform cubeTransform;
        cubeTransform.scale = glm::vec3(0.025f);
        _shaders.debugViewPropgationGrid.Use();
        _shaders.debugViewPropgationGrid.SetMat4("projection", Player::GetProjectionMatrix(_depthOfFieldScene));
        _shaders.debugViewPropgationGrid.SetMat4("view", Player::GetViewMatrix());
        _shaders.debugViewPropgationGrid.SetMat4("model", cubeTransform.to_mat4());
        _shaders.debugViewPropgationGrid.SetFloat("propogationGridSpacing", _propogationGridSpacing);
        _shaders.debugViewPropgationGrid.SetInt("propogationTextureWidth", _gridTextureWidth);
        _shaders.debugViewPropgationGrid.SetInt("propogationTextureHeight", _gridTextureHeight);
        _shaders.debugViewPropgationGrid.SetInt("propogationTextureDepth", _gridTextureDepth);
        int count = _gridTextureWidth * _gridTextureHeight * _gridTextureDepth;
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
    std::string texture = "CrosshairDot";
    if (Scene::CursorShouldBeInterect()) {
        texture = "CrosshairSquare";
    }
    QueueUIForRendering(texture, _presentFrameBuffer.GetWidth() / 2, _presentFrameBuffer.GetHeight() / 2, true);

    if (Scene::_cameraRayData.found) {
        if (Scene::_cameraRayData.raycastObjectType == RaycastObjectType::WALLS) {
            TextBlitter::_debugTextToBilt += "Ray hit: WALL\n";
        }
        if (Scene::_cameraRayData.raycastObjectType == RaycastObjectType::DOOR) {
            TextBlitter::_debugTextToBilt += "Ray hit: DOOR\n";
        }
    }
    else {
        TextBlitter::_debugTextToBilt += "Ray hit: NONE\n";
    }

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


    // Wipe all light dirty flags back to false
    for (Light& light : Scene::_lights) {
        light.isDirty = false;
    }
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

    // Update lights
    auto& lights = Scene::_lights;
    for (int i = 0; i < lights.size(); i++) {
        if (i >= 16) break;
        glActiveTexture(GL_TEXTURE5 + i);
        glBindTexture(GL_TEXTURE_CUBE_MAP, _shadowMaps[i]._depthTexture);
        _shaders.lighting.SetVec3("lights[" + std::to_string(i) + "].position", lights[i].position);
        _shaders.lighting.SetVec3("lights[" + std::to_string(i) + "].color", lights[i].color);
        _shaders.lighting.SetFloat("lights[" + std::to_string(i) + "].radius", lights[i].radius);
        _shaders.lighting.SetFloat("lights[" + std::to_string(i) + "].strength", lights[i].strength);
    }
    _shaders.lighting.SetInt("lightsCount", std::min((int)lights.size(), 16));

    static float time = 0;
    time += 0.01f;
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
        //std::cout << Scene::_lights.size() << "\n";
        for (Light& light : Scene::_lights) {
            glm::vec3 lightCenter = light.position;
            Transform lightTransform;
            lightTransform.scale = glm::vec3(0.2f);
            lightTransform.position = lightCenter;
            _shaders.solidColor.SetMat4("model", lightTransform.to_mat4());

            if (light.isDirty) {
                _shaders.solidColor.SetVec3("color", YELLOW);
            }
            else {
                _shaders.solidColor.SetVec3("color", WHITE);
            }
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

        for (RTInstance& instance : Scene::_rtInstances) {
            RTMesh& mesh = Scene::_rtMesh[instance.meshIndex];

            for (int i = mesh.baseVertex; i < mesh.baseVertex + mesh.vertexCount; i+=3) {
                Triangle t;
                t.p1 = instance.modelMatrix * glm::vec4(Scene::_rtVertices[i+0], 1.0);
                t.p2 = instance.modelMatrix * glm::vec4(Scene::_rtVertices[i+1], 1.0);
                t.p3 = instance.modelMatrix * glm::vec4(Scene::_rtVertices[i+2], 1.0);
                t.color = YELLOW;
                Renderer::QueueTriangleForLineRendering(t);
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

    AssetManager::BindMaterialByIndex(AssetManager::GetMaterialIndex("Ceiling"));
    //AssetManager::BindMaterialByIndex(AssetManager::GetMaterialIndex("NumGrid"));
    for (Wall& wall : Scene::_walls) {
        //AssetManager::BindMaterialByIndex(wall.materialIndex);
        wall.Draw();
    }

    for (Floor& floor : Scene::_floors) {
        AssetManager::BindMaterialByIndex(floor.materialIndex);
        //  AssetManager::BindMaterialByIndex(AssetManager::GetMaterialIndex("NumGrid"));
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
   AssetManager::BindMaterialByIndex(AssetManager::GetMaterialIndex("Ceiling"));
    for (Wall& wall : Scene::_walls) {
        for (auto& transform : wall.ceilingTrims) {
            shader.SetMat4("model", transform.to_mat4());
            AssetManager::GetModel("TrimCeiling").Draw();
        }
    } 
    // Floor trims
    AssetManager::BindMaterialByIndex(trimFloorMaterialIndex);
    for (Wall& wall : Scene::_walls) {
        for (auto& transform : wall.floorTrims) {
            shader.SetMat4("model", transform.to_mat4());
            AssetManager::GetModel("TrimFloor").Draw();
        }
    }

    // Render game objects
    for (GameObject& gameObject : Scene::_gameObjects) {
        shader.SetMat4("model", gameObject.GetModelMatrix());
        for (int i = 0; i < gameObject._meshMaterialIndices.size(); i++) {
            AssetManager::BindMaterialByIndex(gameObject._meshMaterialIndices[i]);
            gameObject._model->_meshes[i].Draw();
        }
    }

    for (Door& door : Scene::_doors) {

        AssetManager::BindMaterialByIndex(AssetManager::GetMaterialIndex("Door"));
        shader.SetMat4("model", door.GetFrameModelMatrix());
        auto& doorFrameModel = AssetManager::GetModel("DoorFrame");
        doorFrameModel.Draw();

        shader.SetMat4("model", door.GetDoorModelMatrix());
        auto& doorModel = AssetManager::GetModel("Door");
        doorModel.Draw();
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

    for (Door& door : Scene::_doors) {
        shader.SetMat4("model", door.GetFrameModelMatrix());
        auto& doorFrameModel = AssetManager::GetModel("DoorFrame");
        doorFrameModel.Draw();
        shader.SetMat4("model", door.GetDoorModelMatrix());
        auto& doorModel = AssetManager::GetModel("Door");
        doorModel.Draw();
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
    _shaders.fxaa.Load("fxaa.vert", "fxaa.frag");
    _shaders.animatedQuad.Load("animated_quad.vert", "animated_quad.frag");
    _shaders.depthOfField.Load("depth_of_field.vert", "depth_of_field.frag");
    _shaders.debugViewPointCloud.Load("debug_view_point_cloud.vert", "debug_view_point_cloud.frag");
    _shaders.geometry.Load("geometry.vert", "geometry.frag");
    _shaders.lighting.Load("lighting.vert", "lighting.frag");
    _shaders.debugViewPropgationGrid.Load("debug_view_propogation_grid.vert", "debug_view_propogation_grid.frag");
    _shaders.editorTextured.Load("editor_textured.vert", "editor_textured.frag");

    _shaders.compute.Load("res/shaders/compute.comp");
    _shaders.pointCloud.Load("res/shaders/point_cloud.comp");
    _shaders.propogateLight.Load("res/shaders/propogate_light.comp");
    //_shaders.propogationList.Load("res/shaders/propogation_list.comp");
    //_shaders.calculateIndirectDispatchSize.Load("res/shaders/calculate_inidrect_dispatch_size.comp");
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

    _presentFrameBuffer.Bind();

    float renderWidth = _presentFrameBuffer.GetWidth();
    float renderHeight = _presentFrameBuffer.GetHeight();
    float screenWidth = GL::GetWindowWidth();
    float screenHeight = GL::GetWindowHeight();

    glViewport(0, 0, renderWidth, renderHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_DEPTH_TEST);

    _shaders.editorTextured.Use();
    _shaders.editorTextured.SetMat4("projection", Editor::GetProjectionMatrix());
    _shaders.editorTextured.SetMat4("view", Editor::GetViewMatrix());
    Transform t;
    t.position.y = -3.5f;
    _shaders.editorTextured.SetMat4("model", t.to_mat4());

    for (Floor& floor : Scene::_floors) {
        AssetManager::BindMaterialByIndex(floor.materialIndex);
        floor.Draw();
    }

    _shaders.editorSolidColor.Use();
    _shaders.editorSolidColor.SetMat4("projection", Editor::GetProjectionMatrix());
    _shaders.editorSolidColor.SetMat4("view", Editor::GetViewMatrix());
    _shaders.editorSolidColor.SetBool("uniformColor", false);

    RenderImmediate();

   
    // Render UI
    glBindFramebuffer(GL_FRAMEBUFFER, _presentFrameBuffer.GetID());
    glViewport(0, 0, _presentFrameBuffer.GetWidth(), _presentFrameBuffer.GetHeight());
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    Renderer::RenderUI();

    // Blit image back to frame buffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, _presentFrameBuffer.GetID());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, _presentFrameBuffer.GetWidth(), _presentFrameBuffer.GetHeight(), 0, 0, GL::GetWindowWidth(), GL::GetWindowHeight(), GL_COLOR_BUFFER_BIT, GL_NEAREST);
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

void Renderer::QueueLineForDrawing(Line line) {
    _lines.push_back(line);
}

void Renderer::QueuePointForDrawing(Point point) {
    _points.push_back(point);
}

void Renderer::QueueTriangleForLineRendering(Triangle& triangle) {
    _lines.push_back(Line(triangle.p1, triangle.p2, triangle.color));
    _lines.push_back(Line(triangle.p2, triangle.p3, triangle.color));
    _lines.push_back(Line(triangle.p3, triangle.p1, triangle.color));
}

void Renderer::QueueTriangleForSolidRendering(Triangle& triangle) {
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
    AssetManager::GetTexture("MuzzleFlash_ALB").Bind(0);

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

void Renderer::CreatePointCloudBuffer() {
    _pointCloud.vertexCount = Scene::_cloudPoints.size();
    if (_pointCloud.VAO != 0) {
        glDeleteBuffers(1, &_pointCloud.VAO);
        glDeleteVertexArrays(1, &_pointCloud.VAO);
    }
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

    // Floor vertices
   /* std::vector<glm::vec4> vertices;
    for (Floor& floor : Scene::_floors) {
        for (Vertex& vertex : floor.vertices) {
            vertices.push_back(glm::vec4(vertex.position.x, vertex.position.y, vertex.position.z, 0));
        }
    }
    if (_ssbos.floorVertices != 0) {
        glDeleteBuffers(1, &_ssbos.floorVertices);
    }
    glGenBuffers(1, &_ssbos.floorVertices);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _ssbos.floorVertices);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, vertices.size() * sizeof(glm::vec4), &vertices[0], GL_DYNAMIC_STORAGE_BIT);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);  
    _floorVertexCount = vertices.size();
    std::cout << "You sent " << _floorVertexCount << " to the GPU\n";*/
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
    glGenTextures(1, &_progogationGridTexture);
    glBindTexture(GL_TEXTURE_3D, _progogationGridTexture);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA16F, _gridTextureWidth, _gridTextureHeight, _gridTextureDepth, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindImageTexture(1, _progogationGridTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, _progogationGridTexture);

    // Create ssbos
    //glGenBuffers(1, &_ssbos.indirectDispatchSize);
    //glGenBuffers(1, &_ssbos.atomicCounter);
    //glDeleteBuffers(1, &_ssbos.propogationList);
    glDeleteBuffers(1, &_ssbos.rtVertices);
    glDeleteBuffers(1, &_ssbos.rtMesh);
    glGenBuffers(1, &_ssbos.rtInstances);
    glGenBuffers(1, &_ssbos.dirtyPointCloudIndices);

    _shaders.compute.Load("res/shaders/compute.comp");
    _shaders.pointCloud.Load("res/shaders/point_cloud.comp");
    _shaders.propogateLight.Load("res/shaders/propogate_light.comp");
    //_shaders.propogationList.Load("res/shaders/propogation_list.comp");
   // _shaders.calculateIndirectDispatchSize.Load("res/shaders/calculate_inidrect_dispatch_size.comp");

    
    Scene::CreatePointCloud();
    Renderer::CreatePointCloudBuffer();
    Renderer::CreateTriangleWorldVertexBuffer();
    
    std::cout << "Point cloud has " << Scene::_cloudPoints.size() << " points\n";
    std::cout << "Propogation grid has " << (_mapWidth * _mapHeight * _mapDepth / _propogationGridSpacing) << " cells\n";

    // Propogation List
   /*glGenBuffers(1, &_ssbos.propogationList);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _ssbos.propogationList);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, _gridTextureSize * sizeof(glm::uvec4), nullptr, GL_DYNAMIC_STORAGE_BIT);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
    std::cout << "The propogation list has room for " << _gridTextureSize << " uvec4 elements\n";*/
}

void Renderer::CreateTriangleWorldVertexBuffer() {

    // Vertices
    std::vector<glm::vec4> vertices;
    for (glm::vec3 vertex : Scene::_rtVertices) {
        vertices.push_back(glm::vec4(vertex, 0.0f));
    }
    glGenBuffers(1, &_ssbos.rtVertices);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _ssbos.rtVertices);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, vertices.size() * sizeof(glm::vec4), &vertices[0], GL_DYNAMIC_STORAGE_BIT);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);

    // Mesh
    glGenBuffers(1, &_ssbos.rtMesh);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _ssbos.rtMesh);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, Scene::_rtMesh.size() * sizeof(RTMesh), &Scene::_rtMesh[0], GL_DYNAMIC_STORAGE_BIT);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);

    std::cout << "You are raytracing against " << (vertices.size() / 3) << " tris\n";
    std::cout << "You are raytracing " << Scene::_rtMesh.size() << " mesh\n";
}

void ComputePass() {

    // Update RT Instances
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _ssbos.rtInstances);
    glBufferData(GL_SHADER_STORAGE_BUFFER, Scene::_rtInstances.size() * sizeof(RTInstance), &Scene::_rtInstances[0], GL_DYNAMIC_COPY);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);

    UpdatePointCloudLighting();
    UpdatePropogationgGrid();
}



void UpdatePointCloudLighting() {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, _ssbos.rtVertices);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, _pointCloud.VBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, _ssbos.rtMesh);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, _ssbos.rtInstances);
    _shaders.pointCloud.Use();
    _shaders.pointCloud.SetInt("meshCount", Scene::_rtMesh.size());
    _shaders.pointCloud.SetInt("instanceCount", Scene::_rtInstances.size());
    _shaders.pointCloud.SetInt("lightCount", std::min((int)Scene::_lights.size(), 32));

    auto& lights = Scene::_lights;
    for (int i = 0; i < lights.size(); i++) {
        _shaders.pointCloud.SetVec3("lights[" + std::to_string(i) + "].position", lights[i].position);
        _shaders.pointCloud.SetVec3("lights[" + std::to_string(i) + "].color", lights[i].color);
        _shaders.pointCloud.SetFloat("lights[" + std::to_string(i) + "].radius", lights[i].radius);
        _shaders.pointCloud.SetFloat("lights[" + std::to_string(i) + "].strength", lights[i].strength);
    }
 

    // Cloud point indices buffer
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _ssbos.dirtyPointCloudIndices);
    glBufferData(GL_SHADER_STORAGE_BUFFER, _dirtyPointCloudIndices.size() * sizeof(int), &_dirtyPointCloudIndices[0], GL_STATIC_COPY);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, _ssbos.dirtyPointCloudIndices);

    if (!_dirtyPointCloudIndices.empty()) {
        //std::cout << "_cloudPointIndices.size(): " << _cloudPointIndices.size() << "\n";
        glDispatchCompute(std::ceil(_dirtyPointCloudIndices.size() / 64.0f), 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }
}

std::vector<glm::uvec4> GridIndicesUpdate(std::vector<glm::uvec4> allGridIndicesWithinRooms)
{
    std::vector<glm::uvec4> gridIndices;
    gridIndices.reserve(_gridTextureSize);

    const float maxDistanceSquared = _maxPropogationDistance * _maxPropogationDistance;

    for (int i = 0; i < allGridIndicesWithinRooms.size(); i++) {

        float x = (float)allGridIndicesWithinRooms[i].x;
        float y = (float)allGridIndicesWithinRooms[i].y;
        float z = (float)allGridIndicesWithinRooms[i].z;
        //gridIndices.emplace_back(x, y, z, 0);
        glm::vec3 probePosition = glm::vec3(x, y, z) * _propogationGridSpacing;

        for (int& index : _dirtyPointCloudIndices) {

            glm::vec3 cloudPointPosition = Scene::_cloudPoints[index].position;
            glm::vec3 cloudPointNormal = Scene::_cloudPoints[index].normal;

            // skip probe if cloud point faces away from probe                
            float r = dot(normalize(cloudPointPosition - probePosition), cloudPointNormal);
            if (r > 0.0) {
                continue;
            }

            if (Util::DistanceSquared(cloudPointPosition, probePosition) < maxDistanceSquared) {
                gridIndices.emplace_back(x, y, z, 0);
                break;
            }
        }
    }
    return gridIndices;
}

void FindProbeCoordsWithinMapBounds() {

    _probeCoordsWithinMapBounds.clear();

    Timer t("FindProbeCoordsWithinMapBounds()");

    // Build a list of floor/ceiling vertices
    std::vector<glm::vec4> floorVertices;
    std::vector<glm::vec4> ceilingVertices;
    for (Floor& floor : Scene::_floors) {
        for (Vertex& vertex : floor.vertices) {
            floorVertices.emplace_back(vertex.position.x, vertex.position.y, vertex.position.z, floor.height);
        }
    }
    for (Ceiling& ceiling : Scene::_ceilings) {
        for (Vertex& vertex : ceiling.vertices) {
            ceilingVertices.emplace_back(vertex.position.x, vertex.position.y, vertex.position.z, ceiling.height);
        }
    }

    // Check which probes are within those bounds created above
    for (int x = 0; x < _gridTextureWidth; x++) {
        for (int y = 0; y < _gridTextureHeight; y++) {
            for (int z = 0; z < _gridTextureDepth; z++) {

             
                bool foundOne = false;

                glm::vec3 probePosition = glm::vec3(x, y, z) * _propogationGridSpacing;

                // Check floors
                for (int j = 0; j < floorVertices.size(); j += 3) {
                    if (probePosition.y < floorVertices[j].w) {
                        continue;
                    }
                    glm::vec2 probePos = glm::vec2(probePosition.x, probePosition.z);
                    glm::vec2 v1 = glm::vec2(floorVertices[j + 0].x, floorVertices[j + 0].z); // when you remove this gen code then it doesnt generate any gridIndices meaning no indirectLight
                    glm::vec2 v2 = glm::vec2(floorVertices[j + 1].x, floorVertices[j + 1].z);
                    glm::vec2 v3 = glm::vec2(floorVertices[j + 2].x, floorVertices[j + 2].z);

                    // If you are above one, check if you are also below a ceiling
                    if (Util::PointIn2DTriangle(probePos, v1, v2, v3)) {

                        if (probePosition.y < 2.6f) {
                            _probeCoordsWithinMapBounds.emplace_back(x, y, z, 0);
                        }

                        /*for (int j = 0; j < ceilingVertices.size(); j += 3) {
                            if (probePosition.y > ceilingVertices[j].w) {
                                continue;
                            }
                            glm::vec2 v1c = glm::vec2(ceilingVertices[j + 0].x, ceilingVertices[j + 0].z);
                            glm::vec2 v2c = glm::vec2(ceilingVertices[j + 1].x, ceilingVertices[j + 1].z);
                            glm::vec2 v3c = glm::vec2(ceilingVertices[j + 2].x, ceilingVertices[j + 2].z);
                            if (Util::PointIn2DTriangle(probePos, v1c, v2c, v3c)) {
                                
                                if (!foundOne) {
                                    _probeCoordsWithinMapBounds.emplace_back(x, y, z, 0);
                                    foundOne = true;
                                }
                                //goto hell;
                            }
                        }*/
                    }
                }
            //hell: {}
            }
        }
    }

   // std::sort(_probeCoordsWithinMapBounds.begin(), _probeCoordsWithinMapBounds.end());
    //_probeCoordsWithinMapBounds.erase(std::unique(_probeCoordsWithinMapBounds.begin(), _probeCoordsWithinMapBounds.end()), _probeCoordsWithinMapBounds.end());

    std::cout << "There are " << _probeCoordsWithinMapBounds.size() << " probes within rooms\n";
    
}

void UpdatePropogationgGrid() {
    
    
    // _gridTextureSize is the max size possible

    //dirtyUpdatesPool.wait(); // seems like thats shit
    

    // check ThreadPool.h and .cpp if you want

    // how many frames behind do you think this is?
    // 1-2 max
    // wait i have an idea uhhh
    // whats the biggest capacity can _gridIndices have.

    // im asking AI how to fix lol
  /*  glBindBuffer(GL_SHADER_STORAGE_BUFFER, _ssbos.indirectDispatchSize);
    glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 3 * sizeof(glm::uint), nullptr, GL_DYNAMIC_COPY);
    */
    // Reset list counter to 0

   /* GLuint counter = 0;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _ssbos.atomicCounter);
    glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(glm::uint), &counter, GL_DYNAMIC_COPY);
     */
    // Build list of probes that need updating
  /*  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, _ssbos.atomicCounter);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, _ssbos.propogationList);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, _ssbos.dirtyPointCloudIndices);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, _pointCloud.VBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, _ssbos.floorVertices);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, _progogationGridTexture);

    _shaders.propogationList.Use();
    _shaders.propogationList.SetInt("dirtyPointCloudIndexCount", _dirtyPointCloudIndices.size());
    _shaders.propogationList.SetFloat("propogationGridSpacing", _propogationGridSpacing);
    _shaders.propogationList.SetFloat("maxDistanceSquared", _maxPropogationDistance * _maxPropogationDistance);
    _shaders.propogationList.SetInt("floorVertexCount", _floorVertexCount);
    
    glDispatchCompute(xSize, ySize, zSize);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Calculate indirect dispatch size
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, _ssbos.atomicCounter);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, _ssbos.indirectDispatchSize);
    _shaders.calculateIndirectDispatchSize.Use();
    glDispatchCompute(1, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);*/



    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, _ssbos.dirtyPointCloudIndices);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, _ssbos.rtVertices);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, _pointCloud.VBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, _ssbos.rtMesh);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, _ssbos.rtInstances);
  //  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, _ssbos.atomicCounter);
  //  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, _ssbos.indirectDispatchSize);
    //glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, _ssbos.propogationList);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, _progogationGridTexture);
    _shaders.propogateLight.Use();
    _shaders.propogateLight.SetInt("pointCloudSize", Scene::_cloudPoints.size());
    _shaders.propogateLight.SetInt("meshCount", Scene::_rtMesh.size());
    _shaders.propogateLight.SetInt("instanceCount", Scene::_rtInstances.size());
    _shaders.propogateLight.SetFloat("propogationGridSpacing", _propogationGridSpacing);
    _shaders.propogateLight.SetFloat("maxDistance", _maxPropogationDistance);
    _shaders.propogateLight.SetInt("dirtyPointCloudIndexCount", _dirtyPointCloudIndices.size());

    if (_ssbos.dirtyGridCoordinates == 0) {
        glGenBuffers(1, &_ssbos.dirtyGridCoordinates);
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _ssbos.dirtyGridCoordinates);
    glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
    glBufferData(GL_SHADER_STORAGE_BUFFER, _gridIndices.size() * sizeof(glm::uvec4), &_gridIndices[0], GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, _ssbos.dirtyGridCoordinates);

    if (_gridIndices.size() && _dirtyPointCloudIndices.size()) {
        int invocationCount = std::ceil(_gridIndices.size() / 64.0f);
        glDispatchCompute(invocationCount, 1, 1);
    }
    // i mean sure let me try removing it from threading rq.
    /*int textureWidth = _mapWidth / _propogationGridSpacing;
    int textureHeight = _mapHeight / _propogationGridSpacing;
    int textureDepth = _mapDepth / _propogationGridSpacing;

    if (_dirtyPointCloudIndices.size()) {
        int xSize = std::ceil(textureWidth / 4.0f);
        int ySize = std::ceil(textureHeight / 4.0f);
        int zSize = std::ceil(textureDepth / 4.0f);
        glDispatchCompute(xSize, ySize, zSize);
    }*/
    /*
    glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, _ssbos.indirectDispatchSize);
    glDispatchComputeIndirect(0);*/
} // this is skill issue idk why it does that lol
