#include "BS_thread_pool.hpp"
#include "Renderer_OLD.h"
#include "NumberBlitter.h"
#include "RendererCommon.h"
#include "TextBlitter.h"
#include "../common.h"

#include "../API/OpenGL/GL_assetManager.h"
#include "../API/OpenGL/GL_renderer.h"
#include "../API/OpenGL/GL_backEnd.h"
#include "../API/OpenGL/Types/GL_gBuffer.h"
#include "../API/OpenGL/Types/GL_presentBuffer.h"
#include "../API/OpenGL/Types/GL_shader.h"
#include "../API/OpenGL/Types/GL_shadowMap.h"
#include "../API/OpenGL/Types/GL_texture3D.h"
#include "../API/Vulkan/VK_renderer.h"
#include "../BackEnd/BackEnd.h"
#include "../Core/AnimatedGameObject.h"
#include "../Core/Audio.hpp"
#include "../Core/AssetManager.h"
#include "../Core/DebugMenu.h"
#include "../Core/Floorplan.h"
#include "../Core/Game.h"
#include "../Core/Gizmo.hpp"
#include "../Core/Input.h"
#include "../Core/Player.h"
#include "../Core/Scene.h"
#include "../Effects/MuzzleFlash.h"
#include "../Types/Texture.h"
#include "../Physics/Physics.h"
#include "../Util.hpp"

#include "../EngineState.hpp"
#include "../Timer.hpp"

#include <vector>
#include <cstdlib>
#include <format>
#include <future>
#include <algorithm>
#include <stb_image.h>

struct RenderTarget {
    GLuint fbo = { 0 };
    GLuint texture = { 0 };
    GLuint width = { 0 };
    GLuint height = { 0 };

    void Create(int textureWidth, int textureHeight) {
        if (fbo != 0) {
            glDeleteFramebuffers(1, &fbo);
        }
        glGenFramebuffers(1, &fbo);
        this->width = textureWidth;
        this->height = textureHeight;
    }
};

std::vector<glm::vec3> debugPoints;

struct EditorObject {
    void* ptr;
    void* rigidBody;
    PhysicsObjectType type;
};


EditorObject _selectedEditorObject;
EditorObject _hoveredEditorObject;
RenderTarget _menuRenderTarget;

struct BlurBuffer {
	GLuint width = 0;
	GLuint height = 0;
	GLuint FBO = 0;
	GLuint textureA = 0;
	GLuint textureB = 0;
};
std::vector<BlurBuffer> _blurBuffers;

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
	Shader geometry_instanced;
	Shader skybox;
	Shader lighting;
	Shader bulletDecals;
	Shader glass;
	Shader glassComposite;
	Shader blurVertical;
	Shader blurHorizontal;
	Shader outline;
    Shader envMapShader;
    Shader brdf;
    Shader test;
    Shader bloodDecals;
    Shader bloodVolumetric;
    Shader toiletWater;
    Shader tri;
    ComputeShader compute;
	ComputeShader pointCloud;
	ComputeShader propogateLight;
	ComputeShader computeTest;
    ComputeShader skin;

} _shaders;

struct SSBOs {
    GLuint rtVertices = 0;
    GLuint rtMesh = 0;
    GLuint rtInstances = 0;
    GLuint dirtyPointCloudIndices = 0;
    GLuint dirtyGridCoordinates = 0;
    GLuint instanceMatrices = 0;
} _ssbos;

struct PointCloud {
    GLuint VAO{ 0 };
    GLuint VBO{ 0 };
    int vertexCount{ 0 };
} _pointCloud;

struct Toggles {
    bool drawLights = false;
    bool drawProbes = false;
    bool drawLines = false;
    bool drawRagdolls = false;
    bool drawDebugText = false;
} _toggles;

struct PlayerRenderTarget {
    GBuffer gBuffer;
    PresentFrameBuffer presentFrameBuffer;
};

GLuint _progogationGridTexture = 0;
//OpenGLMesh _cubeMesh;
unsigned int _pointLineVAO;
unsigned int _pointLineVBO;

constexpr int _horizontalTileCount = PRESENT_WIDTH / 24 * 2;
constexpr int _verticalTileCount = PRESENT_HEIGHT / 24 * 2;

// Light volumes
// *************
// bounding volumes must not exceed the lights radius
// but can be smaller, of course

std::vector<PlayerRenderTarget> _playerRenderTargets;
std::vector<Point> _points;
std::vector<Point> _solidTrianglePoints;
std::vector<Line> _lines;
std::vector<UIRenderInfo> _UIRenderInfos;
std::vector<ShadowMap> _shadowMaps;
GLuint _imageStoreTexture = { 0 };
bool _depthOfFieldScene = 0.9f;
bool _depthOfFieldWeapon = 1.0f;
const float _propogationGridSpacing = 0.375f;
const float _pointCloudSpacing = 0.4f;
const float _maxPropogationDistance = 2.6f; 
const float _maxDistanceSquared = _maxPropogationDistance * _maxPropogationDistance;
float _mapWidth = 16;
float _mapHeight = 8;
float _mapDepth = 16; 
//std::vector<int> _newDirtyPointCloudIndices;
int _floorVertexCount;

bool _lastBlendState = false;

const int _gridTextureWidth = (int)(_mapWidth / _propogationGridSpacing);
const int _gridTextureHeight = (int)(_mapHeight / _propogationGridSpacing);
const int _gridTextureDepth = (int)(_mapDepth / _propogationGridSpacing);
const int _gridTextureSize = (int)(_gridTextureWidth * _gridTextureHeight * _gridTextureDepth);

static std::vector<glm::uvec4> _probeCoordsWithinMapBounds;
static std::vector<glm::vec3> _probeWorldPositionsWithinMapBounds;
std::vector<glm::uvec4> _dirtyProbeCoords;
std::vector<int> _dirtyPointCloudIndices;
int _dirtyProbeCount;

std::vector<glm::mat4> _glockMatrices;
std::vector<glm::mat4> _aks74uMatrices;

enum RenderMode { COMPOSITE, DIRECT_LIGHT, INDIRECT_LIGHT, POINT_CLOUD, MODE_COUNT } _mode;
DebugLineRenderMode _debugLineRenderMode_OLD;


void DrawHudAmmo(Player* player);
void DrawHudLowRes(Player* player);
void DrawScene(Shader& shader);
void DrawAnimatedScene(Shader& shader, Player* player);
void DrawShadowMapScene(Shader& shader);
void DrawBulletDecals(Player* player);
void DrawCasingProjectiles(Player* player);
void DrawFullscreenQuad(); 
void DrawFullscreenQuadWithNormals();
void DrawMuzzleFlashes(Player* player);
void BlurEmissiveBulbs(Player* player);
void InitCompute();
void ComputePass();
void RenderShadowMaps();
void UpdatePointCloudLighting();
void UpdatePropogationgGrid();
void DrawPointCloud(Player* player);
void GeometryPass(Player* player);
void LightingPass(Player* player);
void DebugPass(Player* player);
void RenderImmediate();
void FindProbeCoordsWithinMapBounds();
void CalculateDirtyCloudPoints();
void CalculateDirtyProbeCoords();
void GlassPass(Player* player);
void ToietWaterPass(Player* player);
void BlitDebugTexture(GLint fbo, GLenum colorAttachment, GLint srcWidth, GLint srcHeight);
void DrawQuad(int viewportWidth, int viewPortHeight, int textureWidth, int textureHeight, int xPos, int yPos, bool centered = false, float scale = 1.0f, int xSize = -1, int ySize = -1);
void SkyBoxPass(Player* player);
void QueueAABBForRenering(PxRigidStatic* body);
void QueueAABBForRenering(PxRigidBody* body);
void QueueAABBForRenering(AABB& aabb, glm::vec3 color);
void DrawInstancedVAO(GLuint vao, GLsizei indexCount, std::vector<glm::mat4>& matrices);
void RenderVATBlood(Player* player);

void DrawMeshh(int meshIndex) {
    Mesh* mesh = AssetManager::GetMeshByIndex(meshIndex);
    glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
    glDrawElementsBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), mesh->baseVertex);
}
void DrawSkinnedMeshh(int meshIndex) {
    SkinnedMesh* mesh = AssetManager::GetSkinnedMeshByIndex(meshIndex);
    glBindVertexArray(OpenGLBackEnd::GetWeightedVertexDataVAO());
    glDrawElementsBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), mesh->baseVertex);
}

void DrawModel(Model* model) {
    for (auto meshIndex : model->GetMeshIndices()) {
        DrawMeshh(meshIndex);
    }
}

void DrawRenderItemm(RenderItem3D& renderItem) {
    Mesh* mesh = AssetManager::GetMeshByIndex(renderItem.meshIndex);
    glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
    glDrawElementsBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), mesh->baseVertex);
}

void SetBlendState(bool state) {
    if (state != _lastBlendState) {
        if (state) {
            glEnable(GL_BLEND);
        }
        else {
            glDisable(GL_BLEND);
        }
        _lastBlendState = state;
    }
}

int GetPlayerIndexFromPlayerPointer(Player* player) {
	for (int i = 0; i < Scene::_players.size(); i++) {
		if (&Scene::_players[i] == player) {
			return i;
		}
	}
	return -1;
}

PlayerRenderTarget& GetPlayerRenderTarget(int playerIndex) {
	return _playerRenderTargets[playerIndex];
}

void Renderer_OLD::InitMinimumGL() {

    _shaders.UI.LoadOLD("ui.vert", "ui.frag");
    
    _menuRenderTarget.Create(PRESENT_WIDTH, PRESENT_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, _menuRenderTarget.fbo);
    glGenTextures(1, &_menuRenderTarget.texture);
    glBindTexture(GL_TEXTURE_2D, _menuRenderTarget.texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _menuRenderTarget.width, _menuRenderTarget.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _menuRenderTarget.texture, 0);
}

void Renderer_OLD::Init() {

    Gizmo::Init();
    SetBlendState(false);

    glGenVertexArrays(1, &_pointLineVAO);
    glGenBuffers(1, &_pointLineVBO);
    glPointSize(2);

    _shaders.solidColor.LoadOLD("solid_color.vert", "solid_color.frag");
    _shaders.shadowMap.LoadOLD("shadowmap.vert", "shadowmap.frag", "shadowmap.geom");
    _shaders.editorSolidColor.LoadOLD("editor_solid_color.vert", "editor_solid_color.frag");
    _shaders.composite.LoadOLD("composite.vert", "composite.frag");
    _shaders.fxaa.LoadOLD("fxaa.vert", "fxaa.frag");
    _shaders.animatedQuad.LoadOLD("animated_quad.vert", "animated_quad.frag");
    _shaders.depthOfField.LoadOLD("depth_of_field.vert", "depth_of_field.frag");
    _shaders.debugViewPointCloud.LoadOLD("debug_view_point_cloud.vert", "debug_view_point_cloud.frag");
    _shaders.geometry.LoadOLD("geometry.vert", "geometry.frag");
    _shaders.geometry_instanced.LoadOLD("geometry_instanced.vert", "geometry_instanced.frag");
    _shaders.lighting.LoadOLD("lighting.vert", "lighting.frag");
    _shaders.debugViewPropgationGrid.LoadOLD("debug_view_propogation_grid.vert", "debug_view_propogation_grid.frag");
	_shaders.editorTextured.LoadOLD("editor_textured.vert", "editor_textured.frag");
	_shaders.bulletDecals.LoadOLD("bullet_decals.vert", "bullet_decals.frag");
	_shaders.glass.LoadOLD("glass.vert", "glass.frag");
	_shaders.glassComposite.LoadOLD("glass_composite.vert", "glass_composite.frag");
	_shaders.outline.LoadOLD("outline.vert", "outline.frag");
	_shaders.skybox.LoadOLD("skybox.vert", "skybox.frag");
	_shaders.envMapShader.LoadOLD("envMap.vert", "envMap.frag", "envMap.geom");
	_shaders.brdf.LoadOLD("brdf.vert", "brdf.frag");
    _shaders.test.LoadOLD("test.vert", "test.frag");
    _shaders.bloodVolumetric.LoadOLD("blood_volumetric.vert", "blood_volumetric.frag");
    _shaders.bloodDecals.LoadOLD("blood_decals.vert", "blood_decals.frag");
    _shaders.computeTest.LoadOLD("res/shaders/OpenGL_OLD/test.comp");
    _shaders.skin.LoadOLD("res/shaders/OpenGL_OLD/skin.comp");
    _shaders.blurVertical.LoadOLD("blurVertical.vert", "blur.frag");
    _shaders.blurHorizontal.LoadOLD("blurHorizontal.vert", "blur.frag");
    _shaders.toiletWater.LoadOLD("toilet_water.vert", "toilet_water.frag");
    _shaders.tri.LoadOLD("tri.vert", "tri.frag");
    
  //  _cubeMesh = MeshUtil::CreateCube(1.0f, 1.0f, true);

    RecreateFrameBuffers(0);

    for(int i = 0; i < 16; i++) {
        _shadowMaps.emplace_back();
    }

    for (ShadowMap& shadowMap : _shadowMaps) {
        shadowMap.Init();
    }

    FindProbeCoordsWithinMapBounds();
    InitCompute();


    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices = { 0,1,2 };

    Vertex v1, v2, v3;
    v1.position = glm::vec3(-1, -1, 0);
    v2.position = glm::vec3(1, -1, 0);
    v3.position = glm::vec3(0, 1, 0);
    v1.normal = glm::vec3(RED);
    v2.normal = glm::vec3(BLACK);
    v3.normal = glm::vec3(GREEN);


    vertices.push_back(v1);
    vertices.push_back(v2);
    vertices.push_back(v3);

    AssetManager::CreateMesh("Triangle", vertices, indices);
    AssetManager::UploadVertexData();
    //OpenGLBackEnd::UploadVertexData(vertices, indices);
}

void Renderer_OLD::RenderLoadingScreen() {

    int desiredTotalLines = 40;
    float linesPerPresentHeight = (float)PRESENT_HEIGHT / (float)TextBlitter::GetLineHeight();
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
    TextBlitter::AddDebugText(text);
    TextBlitter::CreateRenderItems(loadingScreenWidth, loadingScreenHeight);
    OpenGLRenderer::RenderLoadingScreen(TextBlitter::GetRenderItems());
}

void Renderer_OLD::RenderFrame(Player* player) {

    if (BackEnd::WindowIsMinimized()) { 
        return;
    }

    _shaders.UI.Use();
    _shaders.UI.SetVec3("overrideColor", WHITE);

    int playerIndex = GetPlayerIndexFromPlayerPointer(player);
    if (!player || playerIndex == -1) {
        return;
    }
    PlayerRenderTarget& playerRenderTarget = GetPlayerRenderTarget(playerIndex);
    GBuffer& gBuffer = playerRenderTarget.gBuffer;
    PresentFrameBuffer& presentFrameBuffer = playerRenderTarget.presentFrameBuffer;
    gBuffer.Bind();
    unsigned int attachments[6] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4, GL_COLOR_ATTACHMENT6 };
    glDrawBuffers(6, attachments);
    glViewport(0, 0, gBuffer.GetWidth(), gBuffer.GetHeight());
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (playerIndex == 0) {
        RenderShadowMaps();
        CalculateDirtyCloudPoints();
        CalculateDirtyProbeCoords();
        ComputePass(); // Fills the indirect lighting data structures
    }
    GeometryPass(player);  
    RenderVATBlood(player);   
    DrawInstancedBloodDecals(&_shaders.bloodDecals, player);
    DrawBulletDecals(player);
    DrawCasingProjectiles(player);
    LightingPass(player);
    SkyBoxPass(player);
    BlurEmissiveBulbs(player);
    ToietWaterPass(player);
    GlassPass(player);
    DrawMuzzleFlashes(player);
    // debug
    //_toggles.drawProbes = true;
    // Propagation Grid
    if (_toggles.drawProbes) {
        Transform cubeTransform;
        cubeTransform.scale = glm::vec3(0.025f);
        _shaders.debugViewPropgationGrid.Use();
        _shaders.debugViewPropgationGrid.SetMat4("projection", player->GetProjectionMatrix());
        _shaders.debugViewPropgationGrid.SetMat4("view", player->GetViewMatrix());
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
        
        static Model* cubeModel = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Cube"));
        Mesh* mesh = AssetManager::GetMeshByIndex(cubeModel->GetMeshIndices()[0]);
        glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
        glDrawElementsInstancedBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), count, mesh->baseVertex);
    }

    // Ammo counter
    if (EngineState::GetEngineMode() == EngineMode::GAME) {
        glDisable(GL_DEPTH_TEST);
        glDrawBuffer(GL_COLOR_ATTACHMENT3);
        DrawHudAmmo(player);
    }

    // Blit final image from large FBO down into a smaller FBO
    glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer.GetID());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, presentFrameBuffer.GetID());
    glReadBuffer(GL_COLOR_ATTACHMENT3);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glBlitFramebuffer(0, 0, gBuffer.GetWidth(), gBuffer.GetHeight(), 0, 0, presentFrameBuffer.GetWidth(), presentFrameBuffer.GetHeight(), GL_COLOR_BUFFER_BIT, GL_LINEAR);

    // FXAA on that smaller FBO
    _shaders.fxaa.Use();
    _shaders.fxaa.SetFloat("viewportWidth", presentFrameBuffer.GetWidth());
    _shaders.fxaa.SetFloat("viewportHeight", presentFrameBuffer.GetHeight());
    presentFrameBuffer.Bind();
    glViewport(0, 0, presentFrameBuffer.GetWidth(), presentFrameBuffer.GetHeight());
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, presentFrameBuffer._inputTexture);
    glDrawBuffer(GL_COLOR_ATTACHMENT1);
    glDisable(GL_DEPTH_TEST);
    DrawFullscreenQuad();

    // Render any debug shit, like the point cloud, prorogation grid, misc points, lines, etc
    DebugPass(player);

    // HUD (killcount, debug text and crosshair)
    if (EngineState::GetEngineMode() == EngineMode::GAME) {
        glDisable(GL_DEPTH_TEST);
        glDrawBuffer(GL_COLOR_ATTACHMENT3);
        DrawHudLowRes(player);
    }

    /*
    _shaders.tri.Use();
    _shaders.tri.SetMat4("projection", player->GetProjectionMatrix());
    int index = AssetManager::GetMeshIndexByName("Triangle");
    Mesh* mesh = AssetManager::GetMeshByIndex(index);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
    glDrawElementsBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), mesh->baseVertex);
    */

    // Editor (GIZMO render and Editor logic in here)
    if (EngineState::GetEngineMode() == EngineMode::EDITOR) {
        RenderEditorMode();
    }
    else {
        _hoveredEditorObject.ptr = nullptr;
        _hoveredEditorObject.rigidBody = nullptr;
        _hoveredEditorObject.type = PhysicsObjectType::UNDEFINED;
        _selectedEditorObject.ptr = nullptr;
        _selectedEditorObject.rigidBody = nullptr;
        _selectedEditorObject.type = PhysicsObjectType::UNDEFINED;
    }

    // Blit that smaller FBO into the main frame buffer 
    if (Game::GetSplitscreenMode() == Game::SplitscreenMode::NONE) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, playerRenderTarget.presentFrameBuffer.GetID());
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glReadBuffer(GL_COLOR_ATTACHMENT1);
        glBlitFramebuffer(0, 0, playerRenderTarget.presentFrameBuffer.GetWidth(), playerRenderTarget.presentFrameBuffer.GetHeight(), 0, 0, BackEnd::GetCurrentWindowWidth(), BackEnd::GetCurrentWindowHeight(), GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }
    else if (Game::GetSplitscreenMode() == Game::SplitscreenMode::TWO_PLAYER) {

        if (GetPlayerIndexFromPlayerPointer(player) == 0) {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, playerRenderTarget.presentFrameBuffer.GetID());
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
            glReadBuffer(GL_COLOR_ATTACHMENT1);
            glBlitFramebuffer(0, 0, playerRenderTarget.presentFrameBuffer.GetWidth(), playerRenderTarget.presentFrameBuffer.GetHeight(), 0, BackEnd::GetCurrentWindowHeight() / 2, BackEnd::GetCurrentWindowWidth(), BackEnd::GetCurrentWindowHeight(), GL_COLOR_BUFFER_BIT, GL_NEAREST);
        }
        if (GetPlayerIndexFromPlayerPointer(player) == 1) {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, playerRenderTarget.presentFrameBuffer.GetID());
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
            glReadBuffer(GL_COLOR_ATTACHMENT1);
            glBlitFramebuffer(0, 0, playerRenderTarget.presentFrameBuffer.GetWidth(), playerRenderTarget.presentFrameBuffer.GetHeight(), 0, 0, BackEnd::GetCurrentWindowWidth(), BackEnd::GetCurrentWindowHeight() / 2, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        }
    }

    // Wipe all light dirty flags back to false
    for (Light& light : Scene::_lights) {
        light.isDirty = false;
    }

    TextBlitter::ClearAllText();
}



void RenderVATBlood(Player* player) {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glCullFace(GL_BACK);
    _shaders.bloodVolumetric.Use();
    _shaders.bloodVolumetric.SetMat4("u_MatrixProjection", player->GetProjectionMatrix());
    _shaders.bloodVolumetric.SetMat4("u_MatrixView", player->GetViewMatrix());
    _shaders.bloodVolumetric.SetVec3("u_viewPos", player->GetViewPos());
    for (int i = 0; i < Scene::_volumetricBloodSplatters.size(); i++) {
        Scene::_volumetricBloodSplatters[i].Draw(&_shaders.bloodVolumetric);
    }
}

void ToietWaterPass(Player* player) {

    int playerIndex = GetPlayerIndexFromPlayerPointer(player);
    PlayerRenderTarget& playerRenderTarget = GetPlayerRenderTarget(playerIndex);
    GBuffer& gBuffer = playerRenderTarget.gBuffer;

    gBuffer.Bind();
    unsigned int attachments[1] = { GL_COLOR_ATTACHMENT3 };
    glDrawBuffers(1, attachments);
    glViewport(0, 0, gBuffer.GetWidth(), gBuffer.GetHeight());

    _shaders.toiletWater.Use();
    _shaders.toiletWater.SetMat4("projection", player->GetProjectionMatrix());
    _shaders.toiletWater.SetMat4("view", player->GetViewMatrix());
    _shaders.toiletWater.SetInt("lightCount", std::min((int)Scene::_lights.size(), 32));

    auto& lights = Scene::_lights;
    for (int i = 0; i < lights.size(); i++) {
        _shaders.toiletWater.SetVec3("lights[" + std::to_string(i) + "].position", lights[i].position);
        _shaders.toiletWater.SetVec3("lights[" + std::to_string(i) + "].color", lights[i].color);
        _shaders.toiletWater.SetFloat("lights[" + std::to_string(i) + "].radius", lights[i].radius);
        _shaders.toiletWater.SetFloat("lights[" + std::to_string(i) + "].strength", lights[i].strength);
    }

    glEnable(GL_BLEND);
    glActiveTexture(GL_TEXTURE0);
    AssetManager::GetTextureByName("ToiletWater2_NRM")->GetGLTexture().Bind(0);

    for (Toilet& toilet : Scene::_toilets) {
        _shaders.toiletWater.SetMat4("model", toilet.GetModelMatrix());
        DrawModel(AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("ToiletWater")));
    }

    glDisable(GL_BLEND);        
}

void GlassPass(Player* player) {

    glm::mat4 projection = player->GetProjectionMatrix();// Renderer::GetProjectionMatrix(_depthOfFieldScene);
    glm::mat4 view = player->GetViewMatrix();

    int playerIndex = GetPlayerIndexFromPlayerPointer(player);
    PlayerRenderTarget& playerRenderTarget = GetPlayerRenderTarget(playerIndex);
    GBuffer& gBuffer = playerRenderTarget.gBuffer;

    gBuffer.Bind();
    unsigned int attachments[1] = { GL_COLOR_ATTACHMENT4 };
    glDrawBuffers(1, attachments);
    glViewport(0, 0, gBuffer.GetWidth(), gBuffer.GetHeight());
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    _shaders.glass.Use();
    _shaders.glass.SetMat4("projection", projection);
    _shaders.glass.SetMat4("view", view);
    _shaders.glass.SetVec3("viewPos", player->GetViewPos());
    _shaders.glass.SetFloat("screenWidth", gBuffer.GetWidth());
    _shaders.glass.SetFloat("screenHeight", gBuffer.GetHeight());

    _shaders.glass.SetInt("lightCount", std::min((int)Scene::_lights.size(), 32));
    auto& lights = Scene::_lights;
    for (int i = 0; i < lights.size(); i++) {
        _shaders.glass.SetVec3("lights[" + std::to_string(i) + "].position", lights[i].position);
        _shaders.glass.SetVec3("lights[" + std::to_string(i) + "].color", lights[i].color);
        _shaders.glass.SetFloat("lights[" + std::to_string(i) + "].radius", lights[i].radius);
        _shaders.glass.SetFloat("lights[" + std::to_string(i) + "].strength", lights[i].strength);
    }

    AssetManager::BindMaterialByIndex(AssetManager::GetMaterialIndex("WindowExterior"));
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, gBuffer.lightingTexture);

    //glDisable(GL_CULL_FACE);
    _shaders.glass.SetBool("isWindow", true);
    for (Window& window : Scene::_windows) {
        _shaders.glass.SetMat4("model", window.GetModelMatrix());
        static Model* GlassModel = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Glass"));      
        DrawModel(GlassModel);
    }
    _shaders.glass.SetBool("isWindow", false);




    // draw the scope
    if (player->GetCurrentWeaponIndex() == AKS74U && !player->InADS() && player->_hasAKS74UScope) {
        AnimatedGameObject* ak = &player->GetFirstPersonWeapon();
        SkinnedModel* skinnedModel = ak->_skinnedModel;
        int boneIndex = skinnedModel->m_BoneMapping["Weapon"];
        if (ak->_animatedTransforms.worldspace.size()) {
            glm::mat4 boneMatrix = ak->_animatedTransforms.worldspace[boneIndex];
            glm::mat4 m = ak->GetModelMatrix() * boneMatrix * player->GetWeaponSwayMatrix();
            _shaders.glass.SetMat4("model", m);
           // AssetManager::BindMaterialByIndex(AssetManager::GetMaterialIndex("Scope"));
           // OpenGLAssetManager::GetModel("ScopeACOG")->_meshes[2].Draw();
           // AssetManager::BindMaterialByIndex(AssetManager::GetMaterialIndex("Scope"));
           // OpenGLAssetManager::GetModel("ScopeACOG")->_meshes[3].Draw();
        }
    }


    _shaders.glass.SetBool("isAnimated", true);

    if (player->GetCurrentWeaponIndex() == GLOCK) {
    
        AnimatedGameObject* animatedGameObject = &player->GetFirstPersonWeapon();
        Shader& shader = _shaders.glass;

        if (!animatedGameObject) {
            std::cout << "You tried to draw an nullptr AnimatedGameObject\n";
            return;
        }
        if (!animatedGameObject->_skinnedModel) {
            std::cout << "You tried to draw an AnimatedGameObject with a nullptr skinned model\n";
            return;
        }

        // Animated transforms
        for (unsigned int i = 0; i < animatedGameObject->_animatedTransforms.local.size(); i++) {
            glm::mat4 matrix = animatedGameObject->_animatedTransforms.local[i];
            shader.SetMat4("skinningMats[" + std::to_string(i) + "]", matrix);
        }


        shader.SetMat4("model", animatedGameObject->GetModelMatrix());


        // Draw
        for (MeshRenderingEntry& meshRenderingEntry : animatedGameObject->_meshRenderingEntries) {

            const int materialIndex = meshRenderingEntry.materialIndex;
            const bool blendingEnabled = meshRenderingEntry.blendingEnabled;
            const bool drawingEnabled = meshRenderingEntry.drawingEnabled;
            const bool renderAsGlass = meshRenderingEntry.renderAsGlass;
            const std::string& meshName = meshRenderingEntry.meshName;

            SetBlendState(blendingEnabled);

            if (drawingEnabled && renderAsGlass) {
                AssetManager::BindMaterialByIndex(materialIndex);
                DrawSkinnedMeshh(meshRenderingEntry.meshIndex);
            }
        }
        SetBlendState(false);
    }
    _shaders.glass.SetBool("isAnimated", false);







    _shaders.glassComposite.Use();

    glm::vec3 colorTint = glm::vec3(1, 1, 1);
    float contrastAmount = 1;

    if (!player->_isDead && player->_isOutside) {
        colorTint = RED;
        colorTint.g = player->_outsideDamageAudioTimer;
        colorTint.b = player->_outsideDamageAudioTimer;
    }

    if (!player->_isDead && player->_damageColorTimer < 1.0f) {
        colorTint.g = player->_damageColorTimer + 0.75;
        colorTint.b = player->_damageColorTimer + 0.75;
        colorTint.g = std::min(colorTint.g, 1.0f);
        colorTint.b = std::min(colorTint.b, 1.0f);
    }

    if (player->_isDead) {

        // Make it red
        if (player->_timeSinceDeath > 0) {
            colorTint.g *= 0.25f;
            colorTint.b *= 0.25f;
            contrastAmount = 1.2f;
        }
        // Darken it after 3 seconds
        float waitTime = 3;
        if (player->_timeSinceDeath > waitTime) {
            float val = (player->_timeSinceDeath - waitTime) * 10;
            colorTint.r -= val;
        }
    }
    _shaders.glassComposite.SetVec3("screenTint", colorTint);
    _shaders.glassComposite.SetFloat("contrastAmount", contrastAmount);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gBuffer.lightingTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gBuffer.glassTexture);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, _blurBuffers[0].textureB);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, _blurBuffers[1].textureB);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, _blurBuffers[2].textureB);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, _blurBuffers[3].textureB);
    glDisable(GL_DEPTH_TEST);
    unsigned int attachments2[1] = { GL_COLOR_ATTACHMENT5 };
    glDrawBuffers(1, attachments2);
    DrawFullscreenQuad();
    glReadBuffer(GL_COLOR_ATTACHMENT5);
    glDrawBuffer(GL_COLOR_ATTACHMENT3);
    float x0 = 0;
    float y0 = 0;
    float x1 = playerRenderTarget.gBuffer.GetWidth();
    float y1 = playerRenderTarget.gBuffer.GetHeight();
    float d_x0 = 0;
    float d_y0 = 0;
    float d_x1 = playerRenderTarget.gBuffer.GetWidth();
    float d_y1 = playerRenderTarget.gBuffer.GetHeight();
    glBlitFramebuffer(x0, y0, x1, y1, d_x0, d_y0, d_x1, d_y1, GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

void DrawFullScreenQuad() {
    static GLuint VAO = 0;
    if (VAO == 0) {
        float quadVertices[] = {
            // positions         texcoords
            -1.0f,  1.0f, 0.0f,  0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f,  0.0f, 0.0f,
             1.0f,  1.0f, 0.0f,  1.0f, 1.0f,
             1.0f, -1.0f, 0.0f,  1.0f, 0.0f,
        };
        unsigned int VBO;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void BlurEmissiveBulbs(Player* player) {

    if (!_blurBuffers.size()) {

		int width = 1536;
		int height = 864;

        for (int i = 0; i < 4; i++) {

			BlurBuffer& blurBuffer = _blurBuffers.emplace_back(BlurBuffer());
			blurBuffer.width = width;
			blurBuffer.height = height;

            glGenFramebuffers(1, &blurBuffer.FBO);
            glGenTextures(1, &blurBuffer.textureA);
            glGenTextures(1, &blurBuffer.textureB);

            glBindFramebuffer(GL_FRAMEBUFFER, blurBuffer.FBO);

            glBindTexture(GL_TEXTURE_2D, blurBuffer.textureA);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, blurBuffer.textureA, 0);

            glBindTexture(GL_TEXTURE_2D, blurBuffer.textureB);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, blurBuffer.textureB, 0);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);

			width = (int)(width * 0.5f);
            height = (int)(height * 0.5f);
        }
    }


	int playerIndex = GetPlayerIndexFromPlayerPointer(player);
	PlayerRenderTarget& playerRenderTarget = GetPlayerRenderTarget(playerIndex);
	GBuffer& gBuffer = playerRenderTarget.gBuffer;

	

    // first round

	glBindFramebuffer(GL_FRAMEBUFFER, _blurBuffers[0].FBO);
	glViewport(0, 0, _blurBuffers[0].width, _blurBuffers[0].height);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gBuffer.emissiveTexture);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
	_shaders.blurHorizontal.Use();
	_shaders.blurHorizontal.SetFloat("targetWidth", _blurBuffers[0].width);
	DrawFullScreenQuad();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, _blurBuffers[0].textureA);
	glDrawBuffer(GL_COLOR_ATTACHMENT1);
	_shaders.blurVertical.Use();
	_shaders.blurVertical.SetFloat("targetHeight", _blurBuffers[0].height);
	DrawFullScreenQuad();

	// second round

	glBindFramebuffer(GL_FRAMEBUFFER, _blurBuffers[1].FBO);
	glViewport(0, 0, _blurBuffers[1].width, _blurBuffers[1].height);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, _blurBuffers[0].textureB);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	_shaders.blurHorizontal.Use();
	_shaders.blurHorizontal.SetFloat("targetWidth", _blurBuffers[1].width);
	DrawFullScreenQuad();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, _blurBuffers[1].textureA);
	glDrawBuffer(GL_COLOR_ATTACHMENT1);
	_shaders.blurVertical.Use();
	_shaders.blurVertical.SetFloat("targetHeight", _blurBuffers[1].height);
	DrawFullScreenQuad();

	// third round
	glBindFramebuffer(GL_FRAMEBUFFER, _blurBuffers[2].FBO);
	glViewport(0, 0, _blurBuffers[2].width, _blurBuffers[2].height);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, _blurBuffers[1].textureB);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	_shaders.blurHorizontal.Use();
	_shaders.blurHorizontal.SetFloat("targetWidth", _blurBuffers[2].width);
	DrawFullScreenQuad();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, _blurBuffers[2].textureA);
	glDrawBuffer(GL_COLOR_ATTACHMENT1);
	_shaders.blurVertical.Use();
	_shaders.blurVertical.SetFloat("targetHeight", _blurBuffers[2].height);
	DrawFullScreenQuad();

    
	// forth round

	glBindFramebuffer(GL_FRAMEBUFFER, _blurBuffers[3].FBO);
	glViewport(0, 0, _blurBuffers[3].width, _blurBuffers[3].height);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, _blurBuffers[2].textureB);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	_shaders.blurHorizontal.Use();
	_shaders.blurHorizontal.SetFloat("targetWidth", _blurBuffers[3].width);
	DrawFullScreenQuad();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, _blurBuffers[3].textureA);
	glDrawBuffer(GL_COLOR_ATTACHMENT1);
	_shaders.blurVertical.Use();
	_shaders.blurVertical.SetFloat("targetHeight", _blurBuffers[3].height);
	DrawFullScreenQuad();
  
  
}

#include "../Core/Gizmo.hpp"

long MapRange(long x, long in_min, long in_max, long out_min, long out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

#define if_likely(e)   if(!!(e))
constexpr float Pi = 3.14159265359f;
constexpr float TwoPi = 2.0f * Pi;
constexpr float HalfPi = 0.5f * Pi;

Im3d::Vec3 ToEulerXYZ(const Im3d::Mat3& _m)
{
    // http://www.staff.city.ac.uk/~sbbh653/publications/euler.pdf
    Im3d::Vec3 ret;
    if_likely(fabs(_m(2, 0)) < 1.0f)
    {
        ret.y = -asinf(_m(2, 0));
        float c = 1.0f / cosf(ret.y);
        ret.x = atan2f(_m(2, 1) * c, _m(2, 2) * c);
        ret.z = atan2f(_m(1, 0) * c, _m(0, 0) * c);
    }
    else
    {
        ret.z = 0.0f;
        if (!(_m(2, 0) > -1.0f))
        {
            ret.x = ret.z + atan2f(_m(0, 1), _m(0, 2));
            ret.y = HalfPi;
        }
        else
        {
            ret.x = -ret.z + atan2f(-_m(0, 1), -_m(0, 2));
            ret.y = -HalfPi;
        }
    }
    return ret;
}




void RenderGameObjectOutline(Shader& shader, GameObject* gameObject) {

	shader.SetInt("offsetY", 0); // reset y offset
	shader.SetInt("offsetX", 1);
    shader.SetMat4("model", gameObject->GetModelMatrix());

    gameObject->UpdateRenderItems();
    for (RenderItem3D& renderItem : gameObject->GetRenderItems()) {
        shader.SetMat4("model", gameObject->GetModelMatrix());
        DrawRenderItemm(renderItem);
    }
    shader.SetInt("offsetX", -1);
    shader.SetMat4("model", gameObject->GetModelMatrix());
    for (RenderItem3D& renderItem : gameObject->GetRenderItems()) {
        shader.SetMat4("model", gameObject->GetModelMatrix());
        DrawRenderItemm(renderItem);
    }
    shader.SetInt("offsetX", 0); // reset x offset
    shader.SetInt("offsetY", 1);
    shader.SetMat4("model", gameObject->GetModelMatrix());
    for (RenderItem3D& renderItem : gameObject->GetRenderItems()) {
        shader.SetMat4("model", gameObject->GetModelMatrix());
        DrawRenderItemm(renderItem);
    }
    shader.SetInt("offsetY", -1);
    shader.SetMat4("model", gameObject->GetModelMatrix());
    for (RenderItem3D& renderItem : gameObject->GetRenderItems()) {
        shader.SetMat4("model", gameObject->GetModelMatrix());
        DrawRenderItemm(renderItem);
    }
}

// Editor constants
constexpr double _orbitRadius = 2.5f;
constexpr double _orbiteSpeed = 0.003f;
constexpr double _zoomSpeed = 0.5f;
constexpr double _panSpeed = 0.004f;

// Editor globals
glm::dmat4x4 _editorViewMatrix;
glm::dvec3 _viewTarget;
glm::dvec3 _camPos;
double _yawAngle = 0.0;
double _pitchAngle = 0.0;

void Renderer_OLD::EnteredEditorMode() {

    // Force switch to fullscreen and player 1
    // EngineState::_viewportMode = FULLSCREEN;
    // EngineState::_currentPlayer = 0;
    // above is broken. find out why.

    Audio::PlayAudio(AUDIO_SELECT, 1.00f);

    Player* player = &Scene::_players[0];
    _editorViewMatrix = player->GetViewMatrix();

    glm::dmat4x4 inverseViewMatrix = glm::inverse(_editorViewMatrix);
    glm::dvec3 forward = inverseViewMatrix[2];
    glm::dvec3 viewPos = inverseViewMatrix[3];
    _viewTarget = viewPos - (forward * _orbitRadius);
    _camPos = viewPos;

    glm::dvec3 delta = _viewTarget - _camPos;
    _yawAngle = std::atan2(delta.z, delta.x) + HELL_PI * 0.5f;
    _pitchAngle = std::asin(delta.y / glm::length(delta));
}

glm::dvec3 rot3D(glm::dvec3 v, glm::dvec2 rot) {
    glm::vec2 c = cos(rot);
    glm::vec2 s = sin(rot);
    glm::dmat3 rm = glm::dmat3(c.x, c.x * s.y, s.x * c.y,
        0.0, c.y, s.y,
        -c.x, s.y * c.x, c.y * c.x);
    return v * rm;
}

void Renderer_OLD::RenderEditorMode() {

    Player* player = &Scene::_players[0]; 
        
    
           
    PlayerRenderTarget& playerRenderTarget = GetPlayerRenderTarget(0);
    GBuffer& gBuffer = playerRenderTarget.gBuffer;
    PresentFrameBuffer& presentFrameBuffer = playerRenderTarget.presentFrameBuffer;               
    glm::mat4 projection = player->GetProjectionMatrix();
    glm::mat4 view = player->GetViewMatrix();
    glm::vec3 viewPos = player->GetViewPos();
    glm::vec3 viewDir = player->GetCameraForward() * glm::vec3(-1);
    float mouseX = MapRange(Input::GetMouseX(), 0, BackEnd::GetCurrentWindowWidth(), 0, presentFrameBuffer.GetWidth());
    float mouseY = MapRange(Input::GetMouseY(), 0, BackEnd::GetCurrentWindowHeight(), 0, presentFrameBuffer.GetHeight());

    // Start the gizmo out of view
    Transform transform;
    transform.position.y = -1000.0f;
    glm::mat4 matrix = transform.to_mat4();

    // Update gizmo with correct matrix if required
    if (_selectedEditorObject.ptr) {
        GameObject* gameObject = (GameObject*)_selectedEditorObject.ptr;
        matrix = gameObject->GetModelMatrix();
    }

    // Update the gizmo
    Gizmo::Update(viewPos, viewDir, mouseX, mouseY, projection, view, Input::LeftMouseDown(), presentFrameBuffer.GetWidth(), presentFrameBuffer.GetHeight(), matrix);
    Im3d::Mat4 gizmoMatrix = Gizmo::GetTransform();
    Im3d::Vec3 pos = gizmoMatrix.getTranslation();
    Im3d::Vec3 euler = ToEulerXYZ(gizmoMatrix.getRotation());
    Im3d::Vec3 scale = gizmoMatrix.getScale();

    // Update any selected object with its new transform data
    if (_selectedEditorObject.ptr) {
        GameObject* gameObject = (GameObject*)_selectedEditorObject.ptr;
        gameObject->SetPosition(glm::vec3(pos.x, pos.y, pos.z));
        gameObject->SetRotation(glm::vec3(euler.x, euler.y, euler.z));
        gameObject->SetScale(glm::vec3(scale.x, scale.y, scale.z));
    }

    // Check for hover
    _hoveredEditorObject.ptr = nullptr;
    _hoveredEditorObject.rigidBody = nullptr;
    _hoveredEditorObject.type = PhysicsObjectType::UNDEFINED;
    PxU32 hitFlags = RaycastGroup::RAYCAST_ENABLED;
    glm::vec3 rayOrigin = Scene::_players[0].GetViewPos();
    glm::vec3 rayDirection = Util::GetMouseRay(projection, view, BackEnd::GetCurrentWindowWidth(), BackEnd::GetCurrentWindowHeight(), Input::GetMouseX(), Input::GetMouseY());
    auto hitResult = Util::CastPhysXRay(rayOrigin, rayDirection, 100, hitFlags, true);
    if (hitResult.hitFound) {
        if (hitResult.physicsObjectType == PhysicsObjectType::GAME_OBJECT) {
            _hoveredEditorObject.ptr = hitResult.parent;
            _hoveredEditorObject.rigidBody = hitResult.hitActor;
            _hoveredEditorObject.type = hitResult.physicsObjectType;
        }
    }

    if (!Input::KeyDown(HELL_KEY_LEFT_CONTROL_GLFW) && !Input::KeyDown(HELL_KEY_LEFT_ALT)) {

        // Select clicked hovered object
        if (Input::LeftMousePressed() && _hoveredEditorObject.ptr && !Gizmo::HasHover()) {
            _selectedEditorObject = _hoveredEditorObject;
                std::cout << "Selected an editor object\n";
        }

        // Clicked on nothing, so unselect any selected object
        if (Input::LeftMousePressed() && !_hoveredEditorObject.ptr && !Gizmo::HasHover()) {
            _selectedEditorObject.ptr = nullptr;
                _selectedEditorObject.rigidBody = nullptr;
                _selectedEditorObject.type = PhysicsObjectType::UNDEFINED;
            std::cout << "Unelected an editor object\n";
        }
    }

    //////////////////////
    //                  //
    //  Object outline  //

    _shaders.outline.Use();
    _shaders.outline.SetMat4("projection", projection);
    _shaders.outline.SetMat4("view", view);
    _shaders.outline.SetFloat("viewportWidth", presentFrameBuffer.GetWidth());
    _shaders.outline.SetFloat("viewportHeight", presentFrameBuffer.GetHeight());

    // Draw hovered object outline
    if (_hoveredEditorObject.ptr && !Input::LeftMouseDown() && !Gizmo::HasHover()) {
        GameObject* hoveredGameObject = nullptr;
        for (auto& gameObject : Scene::_gameObjects) {
            if (gameObject.raycastRigidStatic.pxRigidStatic == _hoveredEditorObject.rigidBody) {
                hoveredGameObject = &gameObject;
                break;
            }
        }
        if (hoveredGameObject) {
            // Render outline mask
            glDisable(GL_DEPTH_TEST);
            glEnable(GL_STENCIL_TEST);
            glStencilMask(0xff);
            glClearStencil(0);
            glClear(GL_STENCIL_BUFFER_BIT);
            glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
            glStencilFunc(GL_ALWAYS, 1, 1);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            _shaders.outline.SetInt("offsetX", 0); // reset x offset
            _shaders.outline.SetInt("offsetY", 0); // reset y offset
            _shaders.outline.SetMat4("model", hoveredGameObject->GetModelMatrix());

            for (RenderItem3D& renderItem : hoveredGameObject->GetRenderItems()) {
                DrawRenderItemm(renderItem);
            }
            //
            //for (int i = 0; i < hoveredGameObject->_meshMaterialIndices.size(); i++) {
            //    hoveredGameObject->_model_OLD->_meshes[i].Draw();
            //}
            // Render outline
            glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
            glStencilMask(0x00);
            glEnable(GL_STENCIL_TEST);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            glDisable(GL_DEPTH_TEST);
            _shaders.outline.SetVec3("Color", WHITE);
            RenderGameObjectOutline(_shaders.outline, hoveredGameObject);
        }
    }

    // Draw selected object outline
    if (_selectedEditorObject.ptr) {
        GameObject* selectedGameObject = nullptr;
        for (auto& gameObject : Scene::_gameObjects) {
            if (gameObject.raycastRigidStatic.pxRigidStatic == _selectedEditorObject.rigidBody) {
                selectedGameObject = &gameObject;
                break;
            }
        }
        if (selectedGameObject) {
            // Render outline mask
            glDisable(GL_DEPTH_TEST);
            glEnable(GL_STENCIL_TEST);
            glStencilMask(0xff);
            glClearStencil(0);
            glClear(GL_STENCIL_BUFFER_BIT);
            glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
            glStencilFunc(GL_ALWAYS, 1, 1);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            _shaders.outline.SetInt("offsetX", 0); // reset x offset
            _shaders.outline.SetInt("offsetY", 0); // reset y offset
            _shaders.outline.SetMat4("model", selectedGameObject->GetModelMatrix());

            for (RenderItem3D& renderItem : selectedGameObject->GetRenderItems()) {
                DrawRenderItemm(renderItem);
            }

            //for (int i = 0; i < selectedGameObject->_meshMaterialIndices.size(); i++) {
            //    selectedGameObject->_model_OLD->_meshes[i].Draw();
            //}
            // Render outline
            glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
            glStencilMask(0x00);
            glEnable(GL_STENCIL_TEST);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            glDisable(GL_DEPTH_TEST);
            _shaders.outline.SetVec3("Color", YELLOW);
            RenderGameObjectOutline(_shaders.outline, selectedGameObject);
        }
    }

    // Cleanup
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_TRUE);
    glDisable(GL_STENCIL_TEST);
    _shaders.outline.SetInt("offsetX", 0); // reset x offset
    _shaders.outline.SetInt("offsetY", 0); // reset y offset
       
    // Draw the gizmo
    if (_selectedEditorObject.ptr) {
        glm::mat4 projection = player->GetProjectionMatrix();
        glm::mat4 view = player->GetViewMatrix();
        int playerIndex = GetPlayerIndexFromPlayerPointer(player);
        PlayerRenderTarget& playerRenderTarget = GetPlayerRenderTarget(playerIndex);
        PresentFrameBuffer& presentFrameBuffer = playerRenderTarget.presentFrameBuffer;
        Gizmo::Draw(projection, view, presentFrameBuffer.GetWidth(), presentFrameBuffer.GetHeight());
    }

    // Camera orbit
    if (Input::LeftMouseDown() && Input::KeyDown(HELL_KEY_LEFT_ALT)) {
        _yawAngle += Input::GetMouseOffsetX() * _orbiteSpeed;
        _pitchAngle -= Input::GetMouseOffsetY() * _orbiteSpeed;
        _camPos = _orbitRadius * glm::dvec3(0, 0, 1);
        _camPos = rot3D(_camPos, { -_yawAngle, -_pitchAngle });
        _camPos += _viewTarget;
        _editorViewMatrix = glm::lookAt(_camPos, _viewTarget, glm::dvec3(0.0, 1.0, 0.0));
    }

    // Camera Zoom
    glm::dmat4 inverseViewMatrix = glm::inverse(_editorViewMatrix);
    glm::dvec3 forward = inverseViewMatrix[2];
    glm::dvec3 right = inverseViewMatrix[0];
    glm::dvec3 up = inverseViewMatrix[1];
    if (Input::MouseWheelUp()) {
        _camPos += (forward * -_zoomSpeed);
        _viewTarget += (forward * -_zoomSpeed);
    }
    if (Input::MouseWheelDown()) {
        _camPos += (forward * _zoomSpeed);
        _viewTarget += (forward * _zoomSpeed);
    }

    // Camera Pan
    if (Input::KeyDown(HELL_KEY_LEFT_CONTROL_GLFW) && Input::LeftMouseDown()) {
        _camPos -= (right * _panSpeed * (double)Input::GetMouseOffsetX());
        _camPos -= (up * _panSpeed * -(double)Input::GetMouseOffsetY());
        _viewTarget -= (right * _panSpeed * (double)Input::GetMouseOffsetX());
        _viewTarget -= (up * _panSpeed * -(double)Input::GetMouseOffsetY());
    }

    _editorViewMatrix = glm::lookAt(_camPos, _viewTarget, glm::dvec3(0.0, 1.0, 0.0));
    player->ForceSetViewMatrix(_editorViewMatrix);
}


struct CubemapTexutre {
    struct face_info{
        uint8_t *texture{ nullptr };
        int32_t width{};
        int32_t height{};
        int32_t format{ GL_RGB };
        uint8_t id{};
    };

	GLuint ID = 0;

	void Create(std::vector<std::string>& textures) {

        glGenTextures(1, &ID);
        glBindTexture(GL_TEXTURE_CUBE_MAP, ID);
        int width, height, nrChannels;
        for (unsigned int i = 0; i < textures.size(); i++) {
            unsigned char* data = stbi_load(textures[i].c_str(), &width, &height, &nrChannels, 0);
            if (data) {
                GLint format = GL_RGB;
                if (nrChannels == 4)
                    format = GL_RGBA;
                if (nrChannels == 1)
                    format = GL_RED;
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, width, height, 0, format, GL_UNSIGNED_BYTE, data);
                stbi_image_free(data);
            }
            else {
                std::cout << "Failed to load cubemap\n";
                stbi_image_free(data);
            }
        }
        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	}
};

GLuint _cubeVao = 0;
CubemapTexutre _skyboxTexture;

void SkyBoxPass(Player* player) {

    if (_cubeVao == 0) {

		// Init cube vertices (for skybox)	
		std::vector<glm::vec3> cubeVertices;
		std::vector<unsigned int> cubeIndices;
		float d = 0.5f;
		cubeVertices.push_back(glm::vec3(-d, d, d));	// Top
		cubeVertices.push_back(glm::vec3(-d, d, -d));
		cubeVertices.push_back(glm::vec3(d, d, -d));
		cubeVertices.push_back(glm::vec3(d, d, d));
		cubeVertices.push_back(glm::vec3(-d, -d, d));	// Bottom
		cubeVertices.push_back(glm::vec3(-d, -d, -d));
		cubeVertices.push_back(glm::vec3(d, -d, -d));
		cubeVertices.push_back(glm::vec3(d, -d, d));
		cubeVertices.push_back(glm::vec3(-d, d, d));	// Z front
		cubeVertices.push_back(glm::vec3(-d, -d, d));
		cubeVertices.push_back(glm::vec3(d, -d, d));
		cubeVertices.push_back(glm::vec3(d, d, d));
		cubeVertices.push_back(glm::vec3(-d, d, -d));	// Z back
		cubeVertices.push_back(glm::vec3(-d, -d, -d));
		cubeVertices.push_back(glm::vec3(d, -d, -d));
		cubeVertices.push_back(glm::vec3(d, d, -d));
		cubeVertices.push_back(glm::vec3(d, d, -d));	// X front
		cubeVertices.push_back(glm::vec3(d, -d, -d));
		cubeVertices.push_back(glm::vec3(d, -d, d));
		cubeVertices.push_back(glm::vec3(d, d, d));
		cubeVertices.push_back(glm::vec3(-d, d, -d));	// X back
		cubeVertices.push_back(glm::vec3(-d, -d, -d));
		cubeVertices.push_back(glm::vec3(-d, -d, d));
		cubeVertices.push_back(glm::vec3(-d, d, d));
		cubeIndices = { 0, 1, 3, 1, 2, 3, 7, 5, 4, 7, 6, 5, 11, 9, 8, 11, 10, 9, 12, 13, 15, 13, 14, 15, 16, 17, 19, 17, 18, 19, 23, 21, 20, 23, 22, 21 };
		unsigned int vbo;
		unsigned int ebo;
		glGenVertexArrays(1, &_cubeVao);
		glGenBuffers(1, &vbo);
		glGenBuffers(1, &ebo);
		glBindVertexArray(_cubeVao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, cubeVertices.size() * sizeof(glm::vec3), &cubeVertices[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, cubeIndices.size() * sizeof(unsigned int), &cubeIndices[0], GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);


		// Load cubemap textures
		std::vector<std::string> skyboxTextureFilePaths;
		skyboxTextureFilePaths.push_back("res/textures/skybox/right.png");
		skyboxTextureFilePaths.push_back("res/textures/skybox/left.png");
		skyboxTextureFilePaths.push_back("res/textures/skybox/top.png");
		skyboxTextureFilePaths.push_back("res/textures/skybox/bottom.png");
		skyboxTextureFilePaths.push_back("res/textures/skybox/front.png");
		skyboxTextureFilePaths.push_back("res/textures/skybox/back.png");
		_skyboxTexture.Create(skyboxTextureFilePaths);

    }







    glm::mat4 projection = player->GetProjectionMatrix();// glm::perspective(1.0f, 1920.0f / 1080.0f, NEAR_PLANE, FAR_PLANE);
	glm::mat4 view = player->GetViewMatrix();

    glEnable(GL_DEPTH_TEST);
    glDrawBuffer(GL_COLOR_ATTACHMENT3);

	// Render skybox
	static Transform skyBoxTransform;
    skyBoxTransform.position = player->GetViewPos();
	//skyBoxTransform.rotation.y -= 0.00015f;
	skyBoxTransform.scale = glm::vec3(50.0f);
	_shaders.skybox.Use();
    _shaders.skybox.SetMat4("projection", projection);
    _shaders.skybox.SetMat4("view", view);
    _shaders.skybox.SetMat4("model", skyBoxTransform.to_mat4());
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, _skyboxTexture.ID);
	//glBindTexture(GL_TEXTURE_CUBE_MAP, _envMap.colorTex);
	glBindVertexArray(_cubeVao);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
//	glClear(GL_DEPTH_BUFFER_BIT);



}

void BlitDebugTexture(GLint fbo, GLenum colorAttachment, GLint srcWidth, GLint srcHeight) {

	glBindFramebuffer(GL_READ_FRAMEBUFFER, _blurBuffers[1].FBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glReadBuffer(colorAttachment);
	float x0 = 0;
	float y0 = 0;
	float x1 = srcWidth;
	float y1 = srcHeight;
	float d_x0 = 0;
	float d_y0 = 0;
	float d_x1 = 1536 * 1.0f;
	float d_y1 = 864 * 1.0f;
	glBlitFramebuffer(x0, y0, x1, y1, d_x0, d_y0, d_x1, d_y1, GL_COLOR_BUFFER_BIT, GL_NEAREST);

}

void DrawHudLowRes(Player* player) {

    int playerIndex = GetPlayerIndexFromPlayerPointer(player);
    PlayerRenderTarget& playerRenderTarget = GetPlayerRenderTarget(playerIndex);
    PresentFrameBuffer& presentFrameBuffer = playerRenderTarget.presentFrameBuffer;
    presentFrameBuffer.Bind();
    glDrawBuffer(GL_COLOR_ATTACHMENT1);
    
    // Crosshair
    if (!player->_isDead) {
        std::string texture = "CrosshairDot";
        if (player->CursorShouldBeInterect()) {
            texture = "CrosshairSquare";
        }
        Renderer_OLD::QueueUIForRendering(texture, presentFrameBuffer.GetWidth() / 2, presentFrameBuffer.GetHeight() / 2, true, WHITE);
    }

    // "Press Start"
    if (player->_isDead && player->_timeSinceDeath > 3.25f) {
        Renderer_OLD::QueueUIForRendering("PressStart", presentFrameBuffer.GetWidth() / 2, presentFrameBuffer.GetHeight() / 2, true, WHITE);
    }

    // Debug lighting mode
    _shaders.lighting.Use();
    if (_mode == RenderMode::COMPOSITE) {
        _shaders.lighting.SetInt("mode", 0);
    }
    else if (_mode == RenderMode::POINT_CLOUD) {
        _shaders.lighting.SetInt("mode", 1);
    }
    else if (_mode == RenderMode::DIRECT_LIGHT) {
        _shaders.lighting.SetInt("mode", 2);
    }
    else if (_mode == RenderMode::INDIRECT_LIGHT) {
        _shaders.lighting.SetInt("mode", 3);
    }

    // Debug text
    if (_toggles.drawDebugText) {

        // Render mode
        if (_mode == RenderMode::COMPOSITE) {
            TextBlitter::_debugTextToBilt = "Mode: COMPOSITE\n";
        }
        else if (_mode == RenderMode::POINT_CLOUD) {
            TextBlitter::_debugTextToBilt = "Mode: POINT CLOUD\n";
        }
        else if (_mode == RenderMode::DIRECT_LIGHT) {
            TextBlitter::_debugTextToBilt = "Mode: DIRECT LIGHT\n";
        }
        else if (_mode == RenderMode::INDIRECT_LIGHT) {
            TextBlitter::_debugTextToBilt = "Mode: INDIRECT LIGHT\n";
        }

        // Debug line mode
        if (_debugLineRenderMode_OLD == DebugLineRenderMode::SHOW_NO_LINES) {
            TextBlitter::_debugTextToBilt += "Debug Mode: NONE\n";
        }
        else if (_debugLineRenderMode_OLD == DebugLineRenderMode::PHYSX_ALL) {
            TextBlitter::_debugTextToBilt += "Debug Mode: PHYSX ALL\n";
        }
        else if (_debugLineRenderMode_OLD == DebugLineRenderMode::PHYSX_RAYCAST) {
            TextBlitter::_debugTextToBilt += "Debug Mode: PHYSX RAYCAST\n";
        }
        else if (_debugLineRenderMode_OLD == DebugLineRenderMode::PHYSX_COLLISION) {
            TextBlitter::_debugTextToBilt += "Debug Mode: PHYSX COLLISION\n";
        }
        else if (_debugLineRenderMode_OLD == DebugLineRenderMode::RAYTRACE_LAND) {
            TextBlitter::_debugTextToBilt += "Debug Mode: COMPUTE RAY WORLD\n";
        }
        else if (_debugLineRenderMode_OLD == DebugLineRenderMode::PHYSX_EDITOR) {
            TextBlitter::_debugTextToBilt += "Debug Mode: PHYSX EDITOR\n";
        }
        else if (_debugLineRenderMode_OLD == DebugLineRenderMode::BOUNDING_BOXES) {
            TextBlitter::_debugTextToBilt += "Debug Mode: BOUNDING BOXES\n";
        }

        // Misc debug info
        TextBlitter::_debugTextToBilt += "View pos: " + Util::Vec3ToString(player->GetViewPos()) + "\n";
        TextBlitter::_debugTextToBilt += "View rot: " + Util::Vec3ToString(player->GetViewRotation()) + "\n";
        TextBlitter::_debugTextToBilt += "Weapon Action: " + Util::WeaponActionToString(Scene::_players[playerIndex].GetWeaponAction()) + "\n";
        TextBlitter::_debugTextToBilt += "Blood decal count: " + std::to_string(Scene::_bloodDecals.size()) + "\n";
    
    }
    // Health and killcount
    else {
        TextBlitter::_debugTextToBilt = "Health: " + std::to_string(player->_health);
        TextBlitter::_debugTextToBilt += "\nKills: " + std::to_string(player->_killCount);
        TextBlitter::_debugTextToBilt += "\n";
    }

    // Pickup text
    if (Game::GetSplitscreenMode() == Game::SplitscreenMode::NONE) {
        TextBlitter::BlitAtPosition(player->_pickUpText, 60, presentFrameBuffer.GetHeight() - 60, false, 1.0f);
    }
    else if (Game::GetSplitscreenMode() == Game::SplitscreenMode::TWO_PLAYER) {
        TextBlitter::BlitAtPosition(player->_pickUpText, 40, presentFrameBuffer.GetHeight() - 35, false, 1.0f);
    }

    //TextBlitter::_debugTextToBilt = "OpenGL c++\n";
    //TextBlitter::_debugTextToBilt += "First quarter medley";
    //TextBlitter::_debugTextToBilt = "\n";

    // Draw it
    glBindFramebuffer(GL_FRAMEBUFFER, presentFrameBuffer.GetID());
    glViewport(0, 0, presentFrameBuffer.GetWidth(), presentFrameBuffer.GetHeight());
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    TextBlitter::Update(1.0f / 60.0f);
    Renderer_OLD::RenderUI(presentFrameBuffer.GetWidth(), presentFrameBuffer.GetHeight());
}
void DrawHudAmmo(Player* player) {

    if (player->_isDead) {
        return;
    }

	int playerIndex = GetPlayerIndexFromPlayerPointer(player);
	PlayerRenderTarget& playerRenderTarget = GetPlayerRenderTarget(playerIndex);
	GBuffer& gBuffer = playerRenderTarget.gBuffer;

	_shaders.UI.Use();
	_shaders.UI.SetMat4("model", glm::mat4(1));
    AssetManager::GetTextureByName("NumSheet")->GetGLTexture().Bind(0);

	float scale = 1.1f;
	unsigned int slashXPos = gBuffer.GetWidth() - 180.0f;
    unsigned int slashYPos = gBuffer.GetHeight() - 120.0f;
	float viewportWidth = gBuffer.GetWidth();
	float viewportHeight = gBuffer.GetHeight();

    if (Game::GetSplitscreenMode() == Game::SplitscreenMode::TWO_PLAYER) {
        slashYPos = gBuffer.GetHeight() - 70.0f;
        scale = 1.05f;
    }
    glm::vec3 ammoColor = glm::vec3(0.16f, 0.78f, 0.23f);


    if (player->GetCurrentWeaponIndex() != Weapon::KNIFE) {
        std::string clipAmmo = std::to_string(player->GetCurrentWeaponClipAmmo());
        std::string totalAmmo = std::to_string(player->GetCurrentWeaponTotalAmmo());
        if (player->GetCurrentWeaponClipAmmo() == 0) {
            ammoColor = glm::vec3(0.8f, 0.05f, 0.05f);
        }
        _shaders.UI.SetVec3("color", ammoColor);
        NumberBlitter::Draw(clipAmmo.c_str(), slashXPos - 4, slashYPos, viewportWidth, viewportHeight, scale, NumberBlitter::Justification::RIGHT);
        _shaders.UI.SetVec3("color", WHITE);
        NumberBlitter::Draw("/", slashXPos, slashYPos, viewportWidth, viewportHeight, scale, NumberBlitter::Justification::LEFT);
        NumberBlitter::Draw(totalAmmo.c_str(), slashXPos + 17, slashYPos, viewportWidth, viewportHeight, scale * 0.8f, NumberBlitter::Justification::LEFT);
    } 
}

void GeometryPass(Player* player) {
    glm::mat4 projection = player->GetProjectionMatrix();// Renderer::GetProjectionMatrix(_depthOfFieldScene); // 1.0 for weapon, 0.9 for scene.
    glm::mat4 view = player->GetViewMatrix();
    
    int playerIndex = GetPlayerIndexFromPlayerPointer(player);
    PlayerRenderTarget& playerRenderTarget = GetPlayerRenderTarget(playerIndex);
    GBuffer& gBuffer = playerRenderTarget.gBuffer;

	gBuffer.Bind();
	unsigned int attachments[5] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT6, GL_COLOR_ATTACHMENT7 };
    glDrawBuffers(5, attachments);
    glViewport(0, 0, gBuffer.GetWidth(), gBuffer.GetHeight());
   // glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
   // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    _shaders.geometry.Use();
    _shaders.geometry.SetMat4("projection", projection);
    _shaders.geometry.SetMat4("view", view);
    _shaders.geometry.SetVec3("viewPos", player->GetViewPos());
    _shaders.geometry.SetVec3("camForward", player->GetCameraForward());
    DrawAnimatedScene(_shaders.geometry, player);
    DrawScene(_shaders.geometry);

    // Draw scope
    if (player->GetCurrentWeaponIndex() == AKS74U && player->_hasAKS74UScope) {
        AnimatedGameObject* ak = &player->GetFirstPersonWeapon();
        SkinnedModel* skinnedModel = ak->_skinnedModel;
        int boneIndex = skinnedModel->m_BoneMapping["Weapon"];
        if (ak->_animatedTransforms.worldspace.size()) {
            glm::mat4 boneMatrix = ak->_animatedTransforms.worldspace[boneIndex];
            glm::mat4 m = ak->GetModelMatrix() * boneMatrix;
            m = m * player->GetWeaponSwayMatrix();
            _shaders.geometry.SetMat4("model", m);
         //   AssetManager::BindMaterialByIndex(AssetManager::GetMaterialIndex("ScopeMount"));
         //   OpenGLAssetManager::GetModel("ScopeACOG")->_meshes[0].Draw();
         //   AssetManager::BindMaterialByIndex(AssetManager::GetMaterialIndex("Scope"));
         //   OpenGLAssetManager::GetModel("ScopeACOG")->_meshes[1].Draw();
        }
    }
}

void LightingPass(Player* player) {

    int playerIndex = GetPlayerIndexFromPlayerPointer(player);
    PlayerRenderTarget& playerRenderTarget = GetPlayerRenderTarget(playerIndex);
    GBuffer& gBuffer = playerRenderTarget.gBuffer;

    static float totalTime = 0;
    totalTime += 1.0f / 60.0f;

    float sinTime = sin(totalTime);

    _shaders.lighting.Use();
    _shaders.lighting.SetFloat("time", totalTime);
    _shaders.lighting.SetFloat("sinTime", sinTime);    

    gBuffer.Bind();
    glDrawBuffer(GL_COLOR_ATTACHMENT3);
    glViewport(0, 0, gBuffer.GetWidth(), gBuffer.GetHeight());
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gBuffer.baseColorTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gBuffer.normalTexture);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gBuffer.RMATexture);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, gBuffer.depthTexture);
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
    _shaders.lighting.SetFloat("screenWidth", gBuffer.GetWidth());
    _shaders.lighting.SetFloat("screenHeight", gBuffer.GetHeight());
    _shaders.lighting.SetMat4("projectionScene", player->GetProjectionMatrix());
    _shaders.lighting.SetMat4("projectionWeapon", player->GetProjectionMatrix());
    _shaders.lighting.SetMat4("inverseProjectionScene", glm::inverse(player->GetProjectionMatrix()));
    _shaders.lighting.SetMat4("inverseProjectionWeapon", glm::inverse(player->GetProjectionMatrix()));
    _shaders.lighting.SetMat4("view", player->GetViewMatrix());
    _shaders.lighting.SetMat4("inverseView", glm::inverse(player->GetViewMatrix()));
    _shaders.lighting.SetVec3("viewPos", player->GetViewPos());
    _shaders.lighting.SetFloat("propogationGridSpacing", _propogationGridSpacing);


  //  GLint texture_units;
  //  glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &texture_units);
  //  std::cout << "texture_units: " << texture_units << "\n";


	_shaders.lighting.SetVec3("player1MuzzleFlashPosition", Scene::_players[0].GetViewPos());
	_shaders.lighting.SetVec3("player2MuzzleFlashPosition", Scene::_players[1].GetViewPos());
	_shaders.lighting.SetBool("player1NeedsMuzzleFlash", Scene::_players[0].MuzzleFlashIsRequired());
	_shaders.lighting.SetBool("player2NeedsMuzzleFlash", Scene::_players[1].MuzzleFlashIsRequired());

    DrawFullscreenQuad();
}

void DebugPass(Player* player) {

    int playerIndex = GetPlayerIndexFromPlayerPointer(player);
    PlayerRenderTarget& playerRenderTarget = GetPlayerRenderTarget(playerIndex);
    PresentFrameBuffer& presentFrameBuffer = playerRenderTarget.presentFrameBuffer;


    presentFrameBuffer.Bind();
    glDrawBuffer(GL_COLOR_ATTACHMENT1);
    glViewport(0, 0, presentFrameBuffer.GetWidth(), presentFrameBuffer.GetHeight());
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    // Point cloud
    if (_mode == RenderMode::POINT_CLOUD) {
        DrawPointCloud(player);
    }

    _shaders.solidColor.Use();
    _shaders.solidColor.SetMat4("projection", player->GetProjectionMatrix());
    _shaders.solidColor.SetMat4("view", player->GetViewMatrix());
    _shaders.solidColor.SetVec3("viewPos", player->GetViewPos());
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
        //    OpenGLAssetManager::GetModel("Cube")->_meshes[0].Draw();
           // _cubeMesh.Draw();
        }
    }

    // Draw casings  
      _points.clear();
      _shaders.solidColor.Use();
      _shaders.solidColor.SetBool("uniformColor", false);
      _shaders.solidColor.SetMat4("model", glm::mat4(1));
      /*for (auto& casing : Scene::_bulletCasings) {
          Point point;
          //point.pos = casing.position;
          point.color = LIGHT_BLUE;
         // Renderer::QueuePointForDrawing(point);
      }*/
      glDisable(GL_DEPTH_TEST);
      glDisable(GL_CULL_FACE);
      RenderImmediate();
      

    _points.clear();
    _shaders.solidColor.Use();
    _shaders.solidColor.SetBool("uniformColor", false);
    _shaders.solidColor.SetMat4("model", glm::mat4(1));



    //////////////////////////////////////////////
    //                                          //
    //  !!! DEBUG POINTS ARE RENDERED HERE !!!  //   
    //                                          //  and they work, so don't fret, just got for it
    
    /*
    AnimatedGameObject* ak = Scene::GetAnimatedGameObjectByName("AKS74U");
    glm::mat4 matrix = ak->GetBoneWorldMatrixFromBoneName("Magazine");
    glm::mat4 magWorldMatrix = ak->GetModelMatrix() * matrix;
    glm::vec3 testPoint = Util::GetTranslationFromMatrix(magWorldMatrix);
    debugPoints.push_back(testPoint);*/


    /*
    AnimatedGameObject* unisexGuy = Scene::GetAnimatedGameObjectByName("UNISEXGUY");
    if (unisexGuy) {
        for (glm::mat4& transform : unisexGuy->_animatedTransforms.worldspace) {
            glm::vec3 point = unisexGuy->_transform.to_mat4() * transform * glm::vec4(0, 0, 0, 1.0);
            //debugPoints.push_back(point);
        }
        for (auto& pos : debugPoints) {
            Point point;
            point.pos = pos;
            point.color = YELLOW;
            Renderer::QueuePointForDrawing(point);
        }
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        RenderImmediate();
        debugPoints.clear();
    }*/

    /*
    static glm::vec3 focalPoint = glm::vec3(2, 1, 2);
    static glm::vec3 position = glm::vec3(2, 1, 2);

    static float angle = 0;
    angle += 0.01f;
    float radius = 0.1f;

    position.x = focalPoint.x + cos(angle) * radius;
    position.z = focalPoint.z + sin(angle) * radius;

    debugPoints.push_back(position);

    */


   // std::cout << Util::Vec3ToString(position) << "\n";

    //this.setPosX(((float)Math.cos(angle) * radius) + center.x);
    //this.setPosY(((float)Math.sin(angle) * radius) + center.y);


    /*
    Point point;
    point.pos = debugBonePosition;
    point.color = YELLOW;
    Renderer::QueuePointForDrawing(point);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    RenderImmediate();
    debugPoints.clear();
    */

    /*
    unisexGuy->UpdateBoneTransformsFromBindPose();
    for (glm::mat4& transform : unisexGuy->_animatedTransforms.worldspace) {
        glm::vec3 point = unisexGuy->_transform.to_mat4() * transform * glm::vec4(0, 0, 0, 1.0);
      //  debugPoints.push_back(point);
    }

    for (glm::mat4& transform : unisexGuy->_debugTransformsB) {
        glm::vec3 point = unisexGuy->_transform.to_mat4() * transform * glm::vec4(0, 0, 0, 1.0);
       // debugPoints.push_back(point);
    }*/

   // AnimatedGameObject* animatedGameObject = Scene::GetAnimatedGameObjectByName("ShotgunTest");
   // glm::vec3 point2 =  animatedGameObject->GetShotgunBarrelPosition();
   // debugPoints.push_back(point2);

/*   for (AnimatedGameObject& animatedGameObject : Scene::_animatedGameObjects) {

        for (glm::mat4 transform : animatedGameObject._animatedTransforms.worldspace) {
            //debugPoints.push_back(Util::GetTranslationFromMatrix(animatedGameObject.GetModelMatrix() * transform));
        }


    }*/ 

    /*
    AnimatedGameObject* unisexGuy = &Scene::_players[1]._characterModel;
    if (unisexGuy) {
        for (glm::mat4& transform : unisexGuy->_animatedTransforms.worldspace) {
            glm::vec3 point = unisexGuy->_transform.to_mat4() * transform * glm::vec4(0, 0, 0, 1.0);
            debugPoints.push_back(point);
        }
        for (auto& pos : debugPoints) {
            Point point;
            point.pos = pos;
            point.color = YELLOW;
            Renderer::QueuePointForDrawing(point);
        }
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        RenderImmediate();
        debugPoints.clear();
    }*/


    // BROKEN!!!
    for (GameObject& gameObject : Scene::_gameObjects) {
        //debugPoints.push_back(gameObject.GetWorldSpaceOABBCenter());
    }

    for (Window& window : Scene::_windows) {

	//	debugPoints.push_back(window.GetFrontLeftCorner());
		//debugPoints.push_back(window.GetBackRightCorner());

		////debugPoints.push_back(window.GetFrontRightCorner());
		//debugPoints.push_back(window.GetBackLeftCorner());
    }

    /*
    AnimatedGameObject* glock = Scene::GetAnimatedGameObjectByName("GLOCK_TEST");
    int boneIndex = glock->_skinnedModel->m_BoneMapping["Barrel"];
    glm::mat4 boneMatrix = glock->_animatedTransforms.worldspace[boneIndex];
    Transform offset;
    offset.position = glm::vec3(0, 2 + 2, 11);
    glm::mat4 m = glock->GetModelMatrix() * boneMatrix * offset.to_mat4();
    float x = m[3][0];
    float y = m[3][1];
    float z = m[3][2];
    glm::vec3 pos = glm::vec3(x, y, z);   
    debugPoints.push_back(pos);*/

   /* auto& player1 = Scene::_players[1];

    for (auto& point : player1._characterModel._debugBones) {
        debugPoints.push_back(point);
    }
    player1._characterModel._debugBones.clear();*/


    for (Player& player : Scene::_players) {
        if (player._characterModel._renderDebugBones) {
            for (auto& boneInfo : player._characterModel._debugBoneInfo) {
                Point point;
                point.color = RED;
                point.pos = boneInfo.worldPos;
                Renderer_OLD::QueuePointForDrawing(point);
            }
        }
    }

    for (auto& pos : debugPoints) {
        Point point;
        point.pos = pos;
        point.color = RED;
        Renderer_OLD::QueuePointForDrawing(point);
    }
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    RenderImmediate();
    debugPoints.clear();

    /*

    for (auto& point : player1._characterModel._debugRigids) {
        debugPoints.push_back(point);
    }
    player1._characterModel._debugRigids.clear();
    for (auto& pos : debugPoints) {
        Point point;
        point.pos = pos + glm::vec3(0, 0.01, 0);
        point.color = YELLOW;
       // Renderer::QueuePointForDrawing(point);
    }
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    RenderImmediate();
    debugPoints.clear();
    */

    //////////////////////////////////////////////////////////////////////////////////////////////
    


    glm::vec3 color = GREEN;
    PxScene* scene = Physics::GetScene();
    PxU32 nbActors = scene->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC);
    if (nbActors) {
        std::vector<PxRigidActor*> actors(nbActors);
        scene->getActors(PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC, reinterpret_cast<PxActor**>(&actors[0]), nbActors);
        for (PxRigidActor* actor : actors) {

			if (actor == Physics::GetGroundPlane()) {
				actor->setActorFlag(PxActorFlag::eVISUALIZATION, false);
                continue;
            }

            PxShape* shape;
            actor->getShapes(&shape, 1);
            actor->setActorFlag(PxActorFlag::eVISUALIZATION, true);

            if (_debugLineRenderMode_OLD == DebugLineRenderMode::PHYSX_RAYCAST) {
                color = RED;
                if (shape->getQueryFilterData().word0 == RaycastGroup::RAYCAST_DISABLED) {
                    actor->setActorFlag(PxActorFlag::eVISUALIZATION, false);
                }
            }    
            else if (_debugLineRenderMode_OLD == DebugLineRenderMode::PHYSX_COLLISION) {
                color = LIGHT_BLUE;
                if (shape->getQueryFilterData().word1 == CollisionGroup::NO_COLLISION) {
                    actor->setActorFlag(PxActorFlag::eVISUALIZATION, false);
                } 
            }
        }
    }


    if (true) {

    }


    // Debug lines
    if (_debugLineRenderMode_OLD == DebugLineRenderMode::BOUNDING_BOXES) {
        for (GameObject& gameObject : Scene::_gameObjects) {
            if (gameObject.raycastRigidStatic.pxRigidStatic) {
                if (gameObject.HasMovedSinceLastFrame()) {
                    QueueAABBForRenering(gameObject._aabb, RED);
                }
                else {
                    QueueAABBForRenering(gameObject._aabb, YELLOW);
                }
            }
        }
        for (Door& door: Scene::_doors) {
            if (door.raycastBody) {
                if (door.HasMovedSinceLastFrame()) {
                    QueueAABBForRenering(door._aabb, RED);
                }
                else {
                    QueueAABBForRenering(door._aabb, YELLOW);
                }
            }
        }
    }

    if (_debugLineRenderMode_OLD == DebugLineRenderMode::PHYSX_ALL ||
        _debugLineRenderMode_OLD == DebugLineRenderMode::PHYSX_COLLISION ||
        _debugLineRenderMode_OLD == DebugLineRenderMode::PHYSX_RAYCAST ||
        _debugLineRenderMode_OLD == DebugLineRenderMode::PHYSX_EDITOR) {

        _lines.clear();
        _shaders.solidColor.Use();
        _shaders.solidColor.SetBool("uniformColor", false);
        _shaders.solidColor.SetMat4("model", glm::mat4(1));
        auto* renderBuffer = &scene->getRenderBuffer();

        for (unsigned int i = 0; i < renderBuffer->getNbLines(); i++) {
            auto pxLine = renderBuffer->getLines()[i];
            Line line;
            line.p1.pos = Util::PxVec3toGlmVec3(pxLine.pos0);
            line.p2.pos = Util::PxVec3toGlmVec3(pxLine.pos1);
            line.p1.color = color;
            line.p2.color = color;
            Renderer_OLD::QueueLineForDrawing(line);
        }
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        RenderImmediate();
    }
    else if (_debugLineRenderMode_OLD == RAYTRACE_LAND) {
        _shaders.solidColor.Use();
        _shaders.solidColor.SetBool("uniformColor", true);
        _shaders.solidColor.SetVec3("color", YELLOW);
        _shaders.solidColor.SetMat4("model", glm::mat4(1));

        for (RTInstance& instance : Scene::_rtInstances) {
            RTMesh& mesh = Scene::_rtMesh[instance.meshIndex];

            for (unsigned int i = mesh.baseVertex; i < mesh.baseVertex + mesh.vertexCount; i+=3) {
                Triangle t;
                t.p1 = instance.modelMatrix * glm::vec4(Scene::_rtVertices[i+0], 1.0);
                t.p2 = instance.modelMatrix * glm::vec4(Scene::_rtVertices[i+1], 1.0);
                t.p3 = instance.modelMatrix * glm::vec4(Scene::_rtVertices[i+2], 1.0);
                t.color = YELLOW;
                Renderer_OLD::QueueTriangleForLineRendering(t);
            }
        }
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        RenderImmediate();
    }    

    // Draw collision world
    /*if (_toggles.drawCollisionWorld) {
        _shaders.solidColor.Use();
        _shaders.solidColor.SetBool("uniformColor", false);
        _shaders.solidColor.SetMat4("model", glm::mat4(1));

        for (Line& collisionLine : Scene::_collisionLines) {
            Renderer::QueueLineForDrawing(collisionLine);
        }

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        RenderImmediate();

        // Draw player sphere
        auto* sphere = OpenGLAssetManager::GetModel("Sphere");
        Transform transform;
        transform.position = player->GetFeetPosition() + glm::vec3(0, 0.101f, 0);
        transform.scale.x = player->GetRadius();
        transform.scale.y = 0.0f;
        transform.scale.z = player->GetRadius();
        _shaders.solidColor.SetBool("uniformColor", true);
        _shaders.solidColor.SetVec3("color", LIGHT_BLUE);
        _shaders.solidColor.SetMat4("model", transform.to_mat4());
        sphere->Draw();
    }*/

}

void Renderer_OLD::RecreateFrameBuffers(int currentPlayer) {

    float width = (float)PRESENT_WIDTH;
    float height = (float)PRESENT_HEIGHT;
    int playerCount = EngineState::GetPlayerCount();

    // Adjust for splitscreen
    if (Game::GetSplitscreenMode() == Game::SplitscreenMode::TWO_PLAYER) {
        height *= 0.5f;
    }

    // Cleanup any existing player render targets
    for (PlayerRenderTarget& playerRenderTarget : _playerRenderTargets) {
        playerRenderTarget.gBuffer.Destroy();
        playerRenderTarget.presentFrameBuffer.Destroy();
    }
    _playerRenderTargets.clear();

    // Create a PlayerRenderTarget for each player
	for (int i = 0; i < playerCount; i++) {
		//if (_viewportMode == FULLSCREEN && i != currentPlayer) {
		//	continue;
		//}
        PlayerRenderTarget& playerRenderTarget = _playerRenderTargets.emplace_back(PlayerRenderTarget());
        playerRenderTarget.gBuffer.Configure(width * 2, height * 2);
        playerRenderTarget.presentFrameBuffer.Configure(width, height);
        
    }

	//std::cout << "Player count: " << playerCount << "\n";
	std::cout << "Render Size: " << width * 2 << " x " << height * 2 << "\n";
    std::cout << "Present Size: " << width << " x " << height << "\n";
    //std::cout << "PlayerRenderTarget Count: " << _playerRenderTargets.size() << "\n";
}

void DrawScene(Shader& shader) {

    



    shader.SetMat4("model", glm::mat4(1));

    static int ceiling2MaterialIndex = AssetManager::GetMaterialIndex("Ceiling2");
    //static int ceiling2MaterialIndex = AssetManager::GetMaterialIndex("Gold");
    static int trimCeilingMaterialIndex = AssetManager::GetMaterialIndex("Trims");
    static int trimFloorMaterialIndex = AssetManager::GetMaterialIndex("Door");
    static int ceilingMaterialIndex = AssetManager::GetMaterialIndex("Ceiling");
    static int wallPaperMaterialIndex = AssetManager::GetMaterialIndex("WallPaper");
    static Model* ceilingTrimModel = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("TrimCeiling"));
    static Model* floorTrimModel = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("TrimFloor"));
    static Model* lampGlobeModel = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("LampGlobe"));

    for (Wall& wall : Scene::_walls) {
        //AssetManager::BindMaterialByIndex(wall.materialIndex);
        AssetManager::BindMaterialByIndex(ceiling2MaterialIndex);
        wall.Draw();
    }

    for (Floor& floor : Scene::_floors) {
        AssetManager::BindMaterialByIndex(floor.materialIndex);
        floor.Draw();
    }

    for (Ceiling& ceiling : Scene::_ceilings) {
        //AssetManager::BindMaterialByIndex(ceiling.materialIndex);
        AssetManager::BindMaterialByIndex(ceiling2MaterialIndex);
        ceiling.Draw();
    }

    // Ceiling trims
    for (Wall& wall : Scene::_walls) {
		for (auto& transform : wall.ceilingTrims) {
			shader.SetMat4("model", transform.to_mat4());
            DrawModel(ceilingTrimModel);
        }
    } 
    // Floor trims
    //AssetManager::BindMaterialByIndex(trimFloorMaterialIndex);
    for (Wall& wall : Scene::_walls) {
		for (auto& transform : wall.floorTrims) {
			shader.SetMat4("model", transform.to_mat4());
            if (wall.materialIndex == ceilingMaterialIndex) {
                DrawModel(floorTrimModel);
			}
			else if (wall.materialIndex == wallPaperMaterialIndex) {
                DrawModel(floorTrimModel);
			}
        }
    }

    // Toilets
    for (Toilet& toilet: Scene::_toilets) {
        toilet.Draw(shader);
    }
  
    // Render game objects
    for (GameObject& gameObject : Scene::_gameObjects) {

        if (gameObject.IsCollectable() && gameObject.IsCollected()) {
            continue;
        }

        shader.SetMat4("model", gameObject.GetModelMatrix());


        // Test render green mag REMOVEEEEEEEEEEEEEEEEEEEEEEEEEE
        if (gameObject.GetName() == "TEST_MAG") {

           // std::cout << "fuck\n";

            //AnimatedGameObject& ak2 = player->GetFirstPersonWeapon();
            AnimatedGameObject* ak2 = Scene::GetAnimatedGameObjectByName("AKS74U_TEST");
            glm::mat4 matrix = ak2->GetBoneWorldMatrixFromBoneName("Magazine");
            glm::mat4 magWorldMatrix = ak2->GetModelMatrix() * matrix;

            Transform t;
            t.position.z = 0.5;
            magWorldMatrix = t.to_mat4() * magWorldMatrix;

            /*
            if (Input::RightMousePressed()) {
                PhysicsFilterData magFilterData;
                magFilterData.raycastGroup = RAYCAST_DISABLED;
                magFilterData.collisionGroup = CollisionGroup::GENERIC_BOUNCEABLE;
                magFilterData.collidesWith = CollisionGroup(ENVIROMENT_OBSTACLE | GENERIC_BOUNCEABLE);
                float magDensity = 750.0f;
                GameObject& mag = Scene::_gameObjects.emplace_back();
                mag.SetModel("AKS74UMag");
                mag.SetName("AKS74UMag");
                mag.SetMeshMaterial("AKS74U_3");
                mag.CreateRigidBody(magWorldMatrix, false);
                mag.SetRaycastShapeFromModel(OpenGLAssetManager::GetModel("AKS74UMag"));
                mag.AddCollisionShapeFromConvexMesh(&OpenGLAssetManager::GetModel("AKS74UMag_ConvexMesh")->_meshes[0], magFilterData, glm::vec3(1));
                mag.SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
                mag.UpdateRigidBodyMassAndInertia(magDensity);
                mag.CreateEditorPhysicsObject();

                PxMat44 mat = Util::GlmMat4ToPxMat44(matrix);
                PxTransform transform(mat);
               // mag._collisionBody->setGlobalPose(transform);

            }*/

            shader.SetMat4("model", magWorldMatrix);
            shader.SetMat4("model", t.to_mat4());


          //  std::cout << "\n" << Util::Mat4ToString(magWorldMatrix) << "\n";

            /*   Player* player = &Scene::_players[0];
               if (player->GetCurrentWeaponIndex() == AKS74U) {
                   AnimatedGameObject& ak2 = player->GetFirstPersonWeapon();
                   glm::mat4 matrix = ak2.GetBoneWorldMatrixFromBoneName("Magazine");
                   glm::mat4 magWorldMatrix = ak2.GetModelMatrix() * player->GetWeaponSwayMatrix() * matrix;
                   shader.SetMat4("model", magWorldMatrix);
               }*/
        }


        gameObject.UpdateRenderItems();
        for (int i = 0; i < gameObject.GetRenderItems().size(); i++) {
            AssetManager::BindMaterialByIndex(gameObject._meshMaterialIndices[i]);
            DrawRenderItemm(*gameObject.GetRenderItemByIndex(i));
        }



        //for (int i = 0; i < gameObject._meshMaterialIndices.size(); i++) {
            //AssetManager::BindMaterialByIndex(gameObject._meshMaterialIndices[i]);
            //gameObject._model_OLD->_meshes[i].Draw();
        //}

        // Add Christmas bows to Christmas cubes
        if (gameObject.GetName() == "Present") {
			AssetManager::BindMaterialByIndex(AssetManager::GetMaterialIndex("Gold"));
            static Model* model = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("ChristmasBow"));
            RenderItem3D renderItem;
            renderItem.meshIndex = model->GetMeshIndices()[0];
            DrawRenderItemm(renderItem);
        }


        // Add globe back to lamp
        if (gameObject.GetName() == "Lamp") {        

            glm::mat4 lampMatrix = gameObject.GetModelMatrix();

            //AssetManager::BindMaterialByIndex(AssetManager::GetMaterialIndex("Gold"));
            //OpenGLAssetManager::GetModel("LampWire")->Draw();


          //  OpenGLAssetManager::GetModel("LampShadeFaceA")->Draw();
          /*  OpenGLAssetManager::GetModel("LampShadeFaceB")->Draw();
            OpenGLAssetManager::GetModel("LampShadeFaceC")->Draw();
            OpenGLAssetManager::GetModel("LampShadeFaceD")->Draw();
            OpenGLAssetManager::GetModel("LampShadeFaceE")->Draw();
            OpenGLAssetManager::GetModel("LampShadeFaceF")->Draw();

            */
            Transform globeTransform;
            globeTransform.position = glm::vec3(0, 0.45f, 0);
            shader.SetMat4("model", lampMatrix * globeTransform.to_mat4());
            shader.SetBool("sampleEmissive", true);
            shader.SetVec3("lightColor", Scene::_lights[0].color);
            AssetManager::BindMaterialByIndex(AssetManager::GetMaterialIndex("Gold"));
            DrawModel(lampGlobeModel);

            shader.SetBool("writeEmissive", false);
            glEnable(GL_BLEND);
            AssetManager::BindMaterialByIndex(AssetManager::GetMaterialIndex("Lamp2"));
            //OpenGLAssetManager::GetModel("LampShade")->Draw();
            shader.SetBool("writeEmissive", true);
            glDisable(GL_BLEND);

            shader.SetBool("sampleEmissive", false);
        }
    };






    static Model* doorModel = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Door"));
    static Model* doorFrameModel = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("DoorFrame"));
    static Model* windowModel = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Window"));

    for (Door& door : Scene::_doors) {

        AssetManager::BindMaterialByIndex(AssetManager::GetMaterialIndex("Door"));

        shader.SetMat4("model", door.GetFrameModelMatrix());
        for (unsigned int meshIndex : doorFrameModel->GetMeshIndices()) {
            DrawMeshh(meshIndex);
        }

        shader.SetMat4("model", door.GetDoorModelMatrix());
        for (unsigned int meshIndex : doorModel->GetMeshIndices()) {
            DrawMeshh(meshIndex);
        }

        /*
        auto* doorFrameModel = OpenGLAssetManager::GetModel("DoorFrame");
        doorFrameModel->Draw();

        shader.SetMat4("model", door.GetDoorModelMatrix());
        auto* doorModel = OpenGLAssetManager::GetModel("Door");
        doorModel->Draw();*/
    }

	for (Window& window : Scene::_windows) {
		shader.SetMat4("model", window.GetModelMatrix());
		AssetManager::BindMaterialByIndex(AssetManager::GetMaterialIndex("Window"));
        DrawMeshh(windowModel->GetMeshIndices()[0]);
        DrawMeshh(windowModel->GetMeshIndices()[1]);
        DrawMeshh(windowModel->GetMeshIndices()[2]);
        DrawMeshh(windowModel->GetMeshIndices()[3]);
        AssetManager::BindMaterialByIndex(AssetManager::GetMaterialIndex("WindowExterior"));
        DrawMeshh(windowModel->GetMeshIndices()[4]);
        DrawMeshh(windowModel->GetMeshIndices()[5]);
        DrawMeshh(windowModel->GetMeshIndices()[6]);
	}

    shader.SetBool("sampleEmissive", true);
	// Light bulbs
	for (Light& light : Scene::_lights) {
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
        }
	}
	shader.SetBool("sampleEmissive", false);

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

    // Animated transforms
    for (unsigned int i = 0; i < animatedGameObject->_animatedTransforms.local.size(); i++) {
        glm::mat4 matrix = animatedGameObject->_animatedTransforms.local[i];
        shader.SetMat4("skinningMats[" + std::to_string(i) + "]", matrix);
    }

    // Model matrix 
    if (animatedGameObject->_hasRagdoll && animatedGameObject->_animationMode == AnimatedGameObject::AnimationMode::RAGDOLL) {
        shader.SetMat4("model", glm::mat4(1));
    }
    else {
        shader.SetMat4("model", animatedGameObject->GetModelMatrix());
    }

    

    // Draw
    //glBindVertexArray(animatedGameObject->_skinnedModel->m_VAO);
    for (MeshRenderingEntry& meshRenderingEntry : animatedGameObject->_meshRenderingEntries) {

        const int materialIndex = meshRenderingEntry.materialIndex;
        const int emissiveColorTexutreIndex = meshRenderingEntry.emissiveColorTexutreIndex;
        const bool blendingEnabled = meshRenderingEntry.blendingEnabled;
        const bool drawingEnabled = meshRenderingEntry.drawingEnabled;
        const bool renderAsGlass = meshRenderingEntry.renderAsGlass;
        const std::string& meshName = meshRenderingEntry.meshName;

        SetBlendState(blendingEnabled);

        if (emissiveColorTexutreIndex != -1) {
            shader.SetBool("hasEmissiveColor", true);
            AssetManager::GetTextureByIndex(emissiveColorTexutreIndex)->GetGLTexture().Bind(3);
        } 
        else {
            shader.SetBool("hasEmissiveColor", false);
        }

        if (drawingEnabled && !renderAsGlass) {
            AssetManager::BindMaterialByIndex(materialIndex);
            DrawSkinnedMeshh(meshRenderingEntry.meshIndex);
        }
    }
    SetBlendState(false);
    shader.SetBool("hasEmissiveColor", false);
}

void DrawAnimatedScene(Shader& shader, Player* player) {

    if (EngineState::GetEngineMode() == EDITOR) {
        return;
    }

    // Hack to hide the dead guy model if the current player is dead
    // otherwise it's possible to see it on screen
    // because the player cam when dead is pinned to the ragdoll head
    // and not the head of the dying guy animation
    /*AnimatedGameObject* dyingGuy = Scene::GetAnimatedGameObjectByName("DyingGuy");
    if (dyingGuy) {
        if (player->_isDead) {
            dyingGuy->SetScale(0.001f);
        }
        else {
            dyingGuy->SetScale(1.000f);
        }
    }
    dyingGuy->SetScale(0.001f);*/

    // This is a temporary hack so multiple animated game objects can have glocks which are queued to be rendered later by DrawScene()
    _glockMatrices.clear();
    _aks74uMatrices.clear();

    /*
    for (Player& otherPlayer : Scene::_players) {
        if (&otherPlayer != player) {
            for (int i = 0; i < otherPlayer._characterModel._animatedTransforms.worldspace.size(); i++) {
                auto& bone = otherPlayer._characterModel._animatedTransforms.worldspace[i];
                glm::mat4 m = otherPlayer._characterModel.GetModelMatrix() * bone;
                if (otherPlayer._characterModel._animatedTransforms.names[i] == "Glock") {
                    if (otherPlayer.GetCurrentWeaponIndex() == GLOCK) {
                        _glockMatrices.push_back(m);
                    }
                    if (otherPlayer.GetCurrentWeaponIndex() == AKS74U) {
                        _aks74uMatrices.push_back(m);
                    }
                }
            }
        }
    }*/

    shader.Use();
    shader.SetBool("isAnimated", true);
    shader.SetMat4("model", glm::mat4(1));

    // Render other players
    for (Player& otherPlayer : Scene::_players) {
        //if (&otherPlayer != player && !otherPlayer._isDead) {
        if (&otherPlayer != player) {
            DrawAnimatedObject(shader, &otherPlayer._characterModel);
        }



        if (Input::KeyPressed(HELL_KEY_L)) {

            std::cout << "\nBONE TRANSFORMS\n";
            for (int i = 0; i < otherPlayer._characterModel._animatedTransforms.names.size(); i++) {

                glm::mat4 matrix = otherPlayer._characterModel._animatedTransforms.worldspace[i];
                std::string& name = otherPlayer._characterModel._animatedTransforms.names[i];

                std::cout << i << ": " << name << "\n";
               // std::cout << Util::Mat4ToString(matrix) << "\n";
            }


            std::cout << "\JOINT NAMES FROM SKINNED MODEL\n";
            for (int i = 0; i < otherPlayer._characterModel._skinnedModel->m_joints.size(); i++) {
                std::string name = otherPlayer._characterModel._skinnedModel->m_joints[i].m_name;
                std::cout << i << ": " << name << "\n";
                // std::cout << Util::Mat4ToString(matrix) << "\n";
            }

        }

    }

    shader.SetMat4("model", glm::mat4(1));
    shader.SetFloat("projectionMatrixIndex", 0.0f);
    for (auto& animatedObject : Scene::GetAnimatedGameObjects()) {
        DrawAnimatedObject(shader, &animatedObject);
    }

    // Render player weapon
    if (EngineState::GetEngineMode() == GAME && !player->_isDead) {
        glDisable(GL_CULL_FACE);
        shader.SetFloat("projectionMatrixIndex", 1.0f);
        shader.SetMat4("projection", player->GetProjectionMatrix()); // 1.0 for weapon, 0.9 for scene.
        DrawAnimatedObject(shader, &player->GetFirstPersonWeapon());
        shader.SetFloat("projectionMatrixIndex", 0.0f);
        glEnable(GL_CULL_FACE);
    }


    // Debug: draw first person weapon for other players
    /*for (Player& otherPlayer : Scene::_players) {
        if (&otherPlayer != player) {
            glDisable(GL_CULL_FACE);
            shader.SetFloat("projectionMatrixIndex", 1.0f);
            shader.SetMat4("projection", Renderer::GetProjectionMatrix(_depthOfFieldWeapon)); // 1.0 for weapon, 0.9 for scene.
            DrawAnimatedObject(shader, &otherPlayer.GetFirstPersonWeapon());
            shader.SetFloat("projectionMatrixIndex", 0.0f);
            glEnable(GL_CULL_FACE);
        }
    }*/





    /*
    auto animatedGameObject = Scene::_players[1]._characterModel;
    auto* ptr = animatedGameObject._skinnedModel;
    auto* skinnedModel = animatedGameObject._skinnedModel;
    int vertexCount = ptr->_vertices.size();
    int indexCount = ptr->_indices.size();
   

    static GLuint _skinnedVao = 0;
    static GLuint _skinnedVbo = 0;

    static GLuint orginalVBO = animatedGameObject._skinnedModel->m_Buffers[POS_VB];
    static GLuint orginalEBO = animatedGameObject._skinnedModel->m_Buffers[INDEX_BUFFER];
    static GLuint orginalBoneDataBuffer = animatedGameObject._skinnedModel->m_Buffers[BONE_VB];

    //if (Input::KeyPressed(HELL_KEY_K)) {

        std::cout << "vertex count: " << vertexCount << "\n";

        //int baseVertex = skinnedModel.m_meshEntries[0].BaseVertex;

        if (_skinnedVao == 0) {
            glGenVertexArrays(1, &_skinnedVao);
            glBindVertexArray(_skinnedVao);
            glGenBuffers(1, &_skinnedVbo);
            glBindBuffer(GL_ARRAY_BUFFER, _skinnedVbo);
            glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(glm::vec3), NULL, GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
        }

        _shaders.skin.Use();

        for (unsigned int i = 0; i < animatedGameObject._animatedTransforms.local.size(); i++) {
            glm::mat4 matrix = animatedGameObject._animatedTransforms.local[i];
            _shaders.skin.SetMat4("skinningMats[" + std::to_string(i) + "]", matrix);
        }

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, orginalVBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, _skinnedVbo);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, orginalBoneDataBuffer);
      //  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, orginalEBO);
        glDispatchCompute((vertexCount + 63) / 64, 1, 1);
  //  }



    // Try draw it

    Transform transform;
    transform.position = glm::vec3(2, 0.1f, 3.5f);
    transform.rotation.x = -HELL_PI * 0.5f;
    transform.scale = glm::vec3(0.01f);

    _shaders.test.Use();
    _shaders.test.SetMat4("projection", Renderer::GetProjectionMatrix(_depthOfFieldScene));
    _shaders.test.SetMat4("view", player->GetViewMatrix());

    

    for (int x = -2;  x < 2; x++) {

        for (int z = -2; z < 2; z++) {
            
            transform.position = glm::vec3(2 + x, 0.1f, 3.5f + z);
            _shaders.test.SetMat4("model", transform.to_mat4());
            
            if (_skinnedVao != 0) {
                glBindVertexArray(_skinnedVao);
                glBindBuffer(GL_ARRAY_BUFFER, _skinnedVbo);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, orginalEBO);
                for (int i = 0; i < skinnedModel->m_meshEntries.size(); i++) {
                    glDrawElementsBaseVertex(GL_TRIANGLES, skinnedModel->m_meshEntries[i].NumIndices, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * skinnedModel->m_meshEntries[i].BaseIndex), skinnedModel->m_meshEntries[i].BaseVertex);
                }
            }
        }
    }
    */

  


    // handle this better
    shader.Use();
    shader.SetBool("isAnimated", false);
    /////////////////////
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
    for (Toilet& toilet : Scene::_toilets) {
        toilet.Draw(shader, false);
    }

    for (Door& door : Scene::_doors) {
        shader.SetMat4("model", door.GetFrameModelMatrix());
        DrawModel(AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("DoorFrame")));
        shader.SetMat4("model", door.GetDoorModelMatrix());
        DrawModel(AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Door")));
    }

    for (GameObject& gameObject : Scene::_gameObjects) {

        if (gameObject.IsCollected()) {
            continue;
        }
        if (gameObject.GetName() == "Lamp") {
            continue;
        }

        gameObject.UpdateRenderItems();
        for (RenderItem3D& renderItem : gameObject.GetRenderItems()) {
            shader.SetMat4("model", gameObject.GetModelMatrix());
            DrawRenderItemm(renderItem);
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



void Renderer_OLD::DrawInstancedBloodDecals(Shader* shader, Player* player) {
    static unsigned int upFacingPlaneVAO = 0;

    // Setup if you haven't already
    if (upFacingPlaneVAO == 0) {
        Vertex vert0, vert1, vert2, vert3;
        vert0.position = glm::vec3(-0.5, 0, 0.5);
        vert1.position = glm::vec3(0.5, 0, 0.5f);
        vert2.position = glm::vec3(0.5, 0, -0.5);
        vert3.position = glm::vec3(-0.5, 0, -0.5);
        vert0.uv = glm::vec2(0, 1);
        vert1.uv = glm::vec2(1, 1);
        vert2.uv = glm::vec2(1, 0);
        vert3.uv = glm::vec2(0, 0);
        Util::SetNormalsAndTangentsFromVertices(&vert0, &vert1, &vert2);
        Util::SetNormalsAndTangentsFromVertices(&vert3, &vert0, &vert1);
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        unsigned int i = (unsigned int)vertices.size();
        indices.push_back(i);
        indices.push_back(i + 1);
        indices.push_back(i + 2);
        indices.push_back(i + 2);
        indices.push_back(i + 3);
        indices.push_back(i);
        vertices.push_back(vert0);
        vertices.push_back(vert1);
        vertices.push_back(vert2);
        vertices.push_back(vert3);
        unsigned int VBO;
        unsigned int EBO;
        glGenVertexArrays(1, &upFacingPlaneVAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
        glBindVertexArray(upFacingPlaneVAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));
    }

    // create GL buffer to store matrices in if ya haven't already
    static unsigned int instancing_array_buffer = 0;
    if (instancing_array_buffer == 0) {
        glGenBuffers(1, &instancing_array_buffer);
    }


    glActiveTexture(GL_TEXTURE2);
    shader->Use();
    shader->SetMat4("pv", player->GetProjectionMatrix() * player->GetViewMatrix());

        glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    // Type 0
    std::vector<glm::mat4> matrices;
    for (BloodDecal& decal : Scene::_bloodDecals) {
        if (decal.type == 0) {
            matrices.push_back(decal.modelMatrix);
        }
    }
    if (matrices.size()) {
        glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByName("blood_decal_4")->GetGLTexture().GetID());
        DrawInstancedVAO(upFacingPlaneVAO, 6, matrices);
    }
    // Type 1
    matrices.clear();
    for (BloodDecal& decal : Scene::_bloodDecals) {
        if (decal.type == 1) {
            matrices.push_back(decal.modelMatrix);
        }
    }
    if (matrices.size()) {
        glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByName("blood_decal_6")->GetGLTexture().GetID());
        DrawInstancedVAO(upFacingPlaneVAO, 6, matrices);
    }
    // Type 2
    matrices.clear();
    for (BloodDecal& decal : Scene::_bloodDecals) {
        if (decal.type == 2) {
            matrices.push_back(decal.modelMatrix);
        }
    }
    if (matrices.size()) {
        glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByName("blood_decal_7")->GetGLTexture().GetID());
        DrawInstancedVAO(upFacingPlaneVAO, 6, matrices);
    }
    // Type 3
    matrices.clear();
    for (BloodDecal& decal : Scene::_bloodDecals) {
        if (decal.type == 3) {
            matrices.push_back(decal.modelMatrix);
        }
    }
    if (matrices.size()) {
        glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByName("blood_decal_9")->GetGLTexture().GetID());
        DrawInstancedVAO(upFacingPlaneVAO, 6, matrices);
    }

    glDepthMask(GL_TRUE);
}

void Renderer_OLD::HotloadShaders() {
    std::cout << "Hotloaded shaders\n";
    _shaders.solidColor.LoadOLD("solid_color.vert", "solid_color.frag");
    _shaders.tri.LoadOLD("tri.vert", "tri.frag");
    _shaders.UI.LoadOLD("ui.vert", "ui.frag");
    _shaders.editorSolidColor.LoadOLD("editor_solid_color.vert", "editor_solid_color.frag");
    _shaders.composite.LoadOLD("composite.vert", "composite.frag");
    _shaders.fxaa.LoadOLD("fxaa.vert", "fxaa.frag");
    _shaders.animatedQuad.LoadOLD("animated_quad.vert", "animated_quad.frag");
    _shaders.depthOfField.LoadOLD("depth_of_field.vert", "depth_of_field.frag");
    _shaders.debugViewPointCloud.LoadOLD("debug_view_point_cloud.vert", "debug_view_point_cloud.frag");
    _shaders.geometry.LoadOLD("geometry.vert", "geometry.frag");
    _shaders.lighting.LoadOLD("lighting.vert", "lighting.frag");
    _shaders.debugViewPropgationGrid.LoadOLD("debug_view_propogation_grid.vert", "debug_view_propogation_grid.frag");
    _shaders.editorTextured.LoadOLD("editor_textured.vert", "editor_textured.frag");
    _shaders.bulletDecals.LoadOLD("bullet_decals.vert", "bullet_decals.frag");
    _shaders.geometry_instanced.LoadOLD("geometry_instanced.vert", "geometry_instanced.frag");
	_shaders.glass.LoadOLD("glass.vert", "glass.frag");
	_shaders.glassComposite.LoadOLD("glass_composite.vert", "glass_composite.frag");
	_shaders.skybox.LoadOLD("skybox.vert", "skybox.frag");
	_shaders.outline.LoadOLD("outline.vert", "outline.frag");
    _shaders.envMapShader.LoadOLD("envMap.vert", "envMap.frag", "envMap.geom");
    _shaders.test.LoadOLD("test.vert", "test.frag");
    _shaders.bloodVolumetric.LoadOLD("blood_volumetric.vert", "blood_volumetric.frag");
    _shaders.bloodDecals.LoadOLD("blood_decals.vert", "blood_decals.frag");
    _shaders.compute.LoadOLD("res/shaders/OpenGL_OLD/compute.comp");
    _shaders.pointCloud.LoadOLD("res/shaders/OpenGL_OLD/point_cloud.comp");
    _shaders.propogateLight.LoadOLD("res/shaders/OpenGL_OLD/propogate_light.comp");
    _shaders.computeTest.LoadOLD("res/shaders/OpenGL_OLD/test.comp");
    _shaders.skin.LoadOLD("res/shaders/OpenGL_OLD/skin.comp");
    _shaders.toiletWater.LoadOLD("toilet_water.vert", "toilet_water.frag");
}

void Renderer_OLD::RenderFloorplanFrame() {

	RenderShadowMaps();
	CalculateDirtyCloudPoints();
	CalculateDirtyProbeCoords();
	ComputePass(); // Fills the indirect lighting data structures

	GBuffer& gBuffer = _playerRenderTargets[0].gBuffer;
    PresentFrameBuffer& presentFrameBuffer = _playerRenderTargets[0].presentFrameBuffer;

	// GEOMETRY PASS
	gBuffer.Bind();
	unsigned int attachments[5] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT6, GL_COLOR_ATTACHMENT7 };
	glDrawBuffers(5, attachments);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

    float renderWidth = (float)gBuffer.GetWidth();
    float renderHeight = (float)gBuffer.GetHeight();

    glViewport(0, 0, renderWidth, renderHeight);
    glClearColor(0.0f, 0.1f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_DEPTH_TEST);

    glDisable(GL_BLEND);

	_shaders.geometry.Use();
	_shaders.geometry.SetMat4("projection", Floorplan::GetProjectionMatrix());
	_shaders.geometry.SetMat4("view", Floorplan::GetViewMatrix());
	_shaders.geometry.SetVec3("viewPos", Scene::_players[0].GetViewPos());
	_shaders.geometry.SetVec3("camForward", glm::inverse(Floorplan::GetViewMatrix())[3]);
    _shaders.geometry.SetMat4("model", glm::mat4(1));

    // Draw the scene
    for (Floor& floor : Scene::_floors) {
        AssetManager::BindMaterialByIndex(floor.materialIndex);
        floor.Draw();
    }
    for (GameObject& gameObject : Scene::_gameObjects) {
        if (gameObject.IsCollectable() && gameObject.IsCollected()) {
            continue;
        }

      /*  gameObject.UpdateRenderItems();
      /  for (RenderItem3D& renderItem : gameObject.GetRenderItems()) {
            _shaders.geometry.SetMat4("model", gameObject.GetModelMatrix());
            DrawRenderItemm(renderItem);
        }*/


        _shaders.geometry.SetMat4("model", gameObject.GetModelMatrix());
       for (int i = 0; i < gameObject._meshMaterialIndices.size(); i++) {
            _shaders.geometry.SetMat4("model", gameObject.GetModelMatrix());
            AssetManager::BindMaterialByIndex(gameObject._meshMaterialIndices[i]);
            DrawMeshh(gameObject.model->GetMeshIndices()[i]);
        }
        if (gameObject.GetName() == "Present") {
            AssetManager::BindMaterialByIndex(AssetManager::GetMaterialIndex("Gold"));
            DrawModel(AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("ChristmasBow")));
        }
    }
    for (Door& door : Scene::_doors) {
        AssetManager::BindMaterialByIndex(AssetManager::GetMaterialIndex("Door"));
        _shaders.geometry.SetMat4("model", door.GetFrameModelMatrix());
        DrawModel(AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("DoorFrame")));
        _shaders.geometry.SetMat4("model", door.GetDoorModelMatrix());
        DrawModel(AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Door")));
    }
    for (Window& window : Scene::_windows) {
        _shaders.geometry.SetMat4("model", window.GetModelMatrix());
        AssetManager::BindMaterialByIndex(AssetManager::GetMaterialIndex("Window"));

        DrawMeshh(AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Window"))->GetMeshIndices()[0]);
        DrawMeshh(AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Window"))->GetMeshIndices()[1]);
        DrawMeshh(AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Window"))->GetMeshIndices()[2]);
        DrawMeshh(AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Window"))->GetMeshIndices()[3]);
        AssetManager::BindMaterialByIndex(AssetManager::GetMaterialIndex("WindowExterior"));
        DrawMeshh(AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Window"))->GetMeshIndices()[4]);
        DrawMeshh(AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Window"))->GetMeshIndices()[5]);
        DrawMeshh(AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Window"))->GetMeshIndices()[6]);
    }

    // Lighting pass
	_shaders.lighting.Use();
	gBuffer.Bind();
	glDrawBuffer(GL_COLOR_ATTACHMENT3);
	glViewport(0, 0, gBuffer.GetWidth(), gBuffer.GetHeight());
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gBuffer.baseColorTexture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gBuffer.normalTexture);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, gBuffer.RMATexture);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, gBuffer.depthTexture);
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
	_shaders.lighting.SetFloat("screenWidth", gBuffer.GetWidth());
	_shaders.lighting.SetFloat("screenHeight", gBuffer.GetHeight());
	_shaders.lighting.SetMat4("projectionScene", Floorplan::GetProjectionMatrix());
	_shaders.lighting.SetMat4("projectionWeapon", Floorplan::GetProjectionMatrix());
	_shaders.lighting.SetMat4("inverseProjectionScene", glm::inverse(Floorplan::GetProjectionMatrix()));
	_shaders.lighting.SetMat4("inverseProjectionWeapon", glm::inverse(Floorplan::GetProjectionMatrix()));
	_shaders.lighting.SetMat4("view", Floorplan::GetViewMatrix());
	_shaders.lighting.SetMat4("inverseView", glm::inverse(Floorplan::GetViewMatrix()));
    _shaders.lighting.SetVec3("viewPos", Scene::_players[0].GetViewPos());
	_shaders.lighting.SetFloat("propogationGridSpacing", _propogationGridSpacing);

	DrawFullscreenQuad();

	// Blit final image from large FBO down into a smaller FBO
	glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer.GetID());
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, presentFrameBuffer.GetID());
	glReadBuffer(GL_COLOR_ATTACHMENT3);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glBlitFramebuffer(0, 0, gBuffer.GetWidth(), gBuffer.GetHeight(), 0, 0, presentFrameBuffer.GetWidth(), presentFrameBuffer.GetHeight(), GL_COLOR_BUFFER_BIT, GL_LINEAR);

	//glEnable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, presentFrameBuffer.GetID());
	glViewport(0, 0, presentFrameBuffer.GetWidth(), presentFrameBuffer.GetHeight());
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
    Renderer_OLD::RenderUI(presentFrameBuffer.GetWidth(), presentFrameBuffer.GetHeight());
    
	_shaders.editorSolidColor.Use();
	_shaders.editorSolidColor.SetMat4("projection", Floorplan::GetProjectionMatrix());
	_shaders.editorSolidColor.SetMat4("view", Floorplan::GetViewMatrix());
	_shaders.editorSolidColor.SetBool("uniformColor", false);

	RenderImmediate();

	float _gridSpacing = 0.1f;
	// Draw grid
	float gridY = -5.0f;
	for (float x = 0; x <= _mapWidth + _gridSpacing / 2; x += _gridSpacing) {
        Renderer_OLD::QueueLineForDrawing(Line(glm::vec3(x, gridY, 0), glm::vec3(x, gridY, _mapWidth), GRID_COLOR));
	}
	for (float z = 0; z <= _mapDepth + _gridSpacing / 2; z += _gridSpacing) {
        Renderer_OLD::QueueLineForDrawing(Line(glm::vec3(0, gridY, z), glm::vec3(_mapDepth, gridY, z), GRID_COLOR));
	}
	glEnable(GL_DEPTH_TEST);
	RenderImmediate();
   
	// Blit image back to frame buffer
    glReadBuffer(GL_COLOR_ATTACHMENT0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, presentFrameBuffer.GetID());
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBlitFramebuffer(0, 0, presentFrameBuffer.GetWidth(), presentFrameBuffer.GetHeight(), 0, 0, BackEnd::GetCurrentWindowWidth(), BackEnd::GetCurrentWindowHeight(), GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

void DrawQuad2(int viewportWidth, int viewPortHeight, int xPos, int yPos, int xSize, int ySize, bool centered) {
	float quadWidth = (float)xSize;
	float quadHeight = (float)ySize;
	float renderTargetWidth = (float)viewportWidth;
	float renderTargetHeight = (float)viewPortHeight;
	float width = (1.0f / renderTargetWidth) * quadWidth;
	float height = (1.0f / renderTargetHeight) * quadHeight;
	float ndcX = ((xPos + (quadWidth / 2.0f)) / renderTargetWidth) * 2 - 1;
	float ndcY = ((yPos + (quadHeight / 2.0f)) / renderTargetHeight) * 2 - 1;
	width = (1.0f / renderTargetWidth) * xSize;
    height = (1.0f / renderTargetHeight) * ySize;
    Transform transform;
	transform.scale.x = width;
	transform.scale.y = height;
	_shaders.UI.SetMat4("model", transform.to_mat4());
    DrawMeshh(AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Quad"))->GetMeshIndices()[0]);
}

void Renderer_OLD::RenderDebugMenu() {

	glBindFramebuffer(GL_FRAMEBUFFER, _menuRenderTarget.fbo);
	glViewport(0, 0, PRESENT_WIDTH, PRESENT_HEIGHT);
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	_shaders.UI.Use();
	_shaders.UI.SetMat4("model", glm::mat4(1));

	float minMenuHeight = 160.0f;
    float lineHeight = TextBlitter::GetLineHeight();
    float paddingBetweenHeading = 8.0f;
    float paddingBetweenLeftAndRightText = 10.0f;
	float viewportCenterX = PRESENT_WIDTH * 0.5f;
	float viewportCenterY = PRESENT_HEIGHT * 0.5f;

    float subMenuHeight = DebugMenu::GetSubMenuItemCount() * lineHeight;
    float headingHeight = lineHeight;

    float menuWidth, textLeftX, textRightX;
	float totalMenuHeight = subMenuHeight + headingHeight;

	float textWidthHeading = TextBlitter::GetTextWidth(DebugMenu::GetHeading());
	float textWidthLeft = TextBlitter::GetTextWidth(DebugMenu::GetTextLeft());
	float textWidthRight = TextBlitter::GetTextWidth(DebugMenu::GetHeading());
    float leftTextWidth = TextBlitter::GetTextWidth(DebugMenu::GetTextLeft());
    float rightTextWidth = TextBlitter::GetTextWidth(DebugMenu::GetTextRight());

	if (DebugMenu::SubMenuHasValues()) {
		menuWidth = std::max(textWidthHeading, textWidthLeft);
        textLeftX = viewportCenterX - leftTextWidth * 0.5f;
        textRightX = 0;
    }
    else {
		menuWidth = std::max(textWidthHeading, textWidthLeft + textWidthRight + (paddingBetweenLeftAndRightText * 2));
		textLeftX = viewportCenterX - leftTextWidth - paddingBetweenLeftAndRightText;
		textRightX = viewportCenterX + paddingBetweenLeftAndRightText;
    }

	// Add padding
    totalMenuHeight += (paddingBetweenHeading * 2) + (lineHeight * 2);
    totalMenuHeight = std::max(minMenuHeight, totalMenuHeight);

	float headingY = viewportCenterY - (totalMenuHeight * 0.5f) + (lineHeight * 1.5f);
	float subMenuY = headingY + lineHeight;

    subMenuY += paddingBetweenHeading;
	menuWidth += (lineHeight * 6);

    float paddingLeftRight = 24;
	menuWidth = 372;
	textLeftX = viewportCenterX - (menuWidth * 0.5f) + paddingLeftRight;
	textRightX = viewportCenterX + 0;

	_shaders.UI.SetVec3("overrideColor", GREEN);
    AssetManager::GetTextureByName("MenuBG")->GetGLTexture().Bind(0);
	DrawQuad(PRESENT_WIDTH, PRESENT_HEIGHT, menuWidth, totalMenuHeight, viewportCenterX, viewportCenterY, true);

    AssetManager::GetTextureByName("MenuBorderHorizontal")->GetGLTexture().Bind(0);
	DrawQuad(PRESENT_WIDTH, PRESENT_HEIGHT, menuWidth, 3, viewportCenterX, viewportCenterY - (totalMenuHeight * 0.5f), true);
	DrawQuad(PRESENT_WIDTH, PRESENT_HEIGHT, menuWidth, 3, viewportCenterX, viewportCenterY + (totalMenuHeight * 0.5f), true);
    AssetManager::GetTextureByName("MenuBorderVertical")->GetGLTexture().Bind(0);
	DrawQuad(PRESENT_WIDTH, PRESENT_HEIGHT, 3, totalMenuHeight, viewportCenterX - (menuWidth * 0.5f), viewportCenterY, true);
	DrawQuad(PRESENT_WIDTH, PRESENT_HEIGHT, 3, totalMenuHeight, viewportCenterX + (menuWidth * 0.5f), viewportCenterY, true);

    AssetManager::GetTextureByName("MenuBorderCornerTL")->GetGLTexture().Bind(0);
	DrawQuad(PRESENT_WIDTH, PRESENT_HEIGHT, 3, 3, viewportCenterX - (menuWidth * 0.5f), viewportCenterY - (totalMenuHeight * 0.5f), true);
    AssetManager::GetTextureByName("MenuBorderCornerTR")->GetGLTexture().Bind(0);
	DrawQuad(PRESENT_WIDTH, PRESENT_HEIGHT, 3, 3, viewportCenterX + (menuWidth * 0.5f), viewportCenterY - (totalMenuHeight * 0.5f), true);
    AssetManager::GetTextureByName("MenuBorderCornerBL")->GetGLTexture().Bind(0);
	DrawQuad(PRESENT_WIDTH, PRESENT_HEIGHT, 3, 3, viewportCenterX - (menuWidth * 0.5f), viewportCenterY + (totalMenuHeight * 0.5f), true);
    AssetManager::GetTextureByName("MenuBorderCornerBR")->GetGLTexture().Bind(0);
	DrawQuad(PRESENT_WIDTH, PRESENT_HEIGHT, 3, 3, viewportCenterX + (menuWidth * 0.5f), viewportCenterY + (totalMenuHeight * 0.5f), true);

    // Draw menu text
    TextBlitter::BlitAtPosition(DebugMenu::GetHeading(), viewportCenterX, headingY, true, 1.0f);
    TextBlitter::BlitAtPosition(DebugMenu::GetTextLeft(), textLeftX, subMenuY, false, 1.0f);
	TextBlitter::BlitAtPosition(DebugMenu::GetTextRight(), textRightX, subMenuY, false, 1.0f);

	_shaders.UI.SetVec3("overrideColor", WHITE);
    TextBlitter::Update(1.0f / 60.0f);
    Renderer_OLD::RenderUI(PRESENT_WIDTH, PRESENT_HEIGHT);
	_shaders.UI.SetVec3("overrideColor", WHITE);

    // Draw the menu into the main frame buffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, BackEnd::GetCurrentWindowWidth(), BackEnd::GetCurrentWindowHeight());
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, _menuRenderTarget.texture);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	_shaders.UI.Use();
	_shaders.UI.SetMat4("model", glm::mat4(1));
    DrawFullscreenQuadWithNormals();
}

void Renderer_OLD::WipeShadowMaps() {

    for (ShadowMap& shadowMap : _shadowMaps) {
        shadowMap.Clear();
    }
}

void Renderer_OLD::ToggleDrawingLights() {
    _toggles.drawLights = !_toggles.drawLights;
}
void Renderer_OLD::ToggleDrawingProbes() {
    _toggles.drawProbes = !_toggles.drawProbes;
}
void Renderer_OLD::ToggleDrawingLines() {
    _toggles.drawLines = !_toggles.drawLines;
}
void Renderer_OLD::ToggleDrawingRagdolls() {
	_toggles.drawRagdolls = !_toggles.drawRagdolls;
}
void Renderer_OLD::ToggleDebugText() {
	_toggles.drawDebugText = !_toggles.drawDebugText;
}

void DrawQuad(int viewportWidth, int viewPortHeight, int textureWidth, int textureHeight, int xPos, int yPos, bool centered, float scale, int xSize, int ySize) {

    float quadWidth = (float)xSize;
    float quadHeight = (float)ySize;
    if (xSize == -1) {
        quadWidth = (float)textureWidth;
    }
    if (ySize == -1) {
        quadHeight = (float)textureHeight;
    }
    if (centered) {
        xPos -= (int)(quadWidth / 2);
        yPos -= (int)(quadHeight / 2);
    }
    float renderTargetWidth = (float)viewportWidth;
    float renderTargetHeight = (float)viewPortHeight;
    float width = (1.0f / renderTargetWidth) * quadWidth * scale;
    float height = (1.0f / renderTargetHeight) * quadHeight * scale;
    float ndcX = ((xPos + (quadWidth / 2.0f)) / renderTargetWidth) * 2 - 1;
    float ndcY = ((yPos + (quadHeight / 2.0f)) / renderTargetHeight) * 2 - 1;
    Transform transform;
    transform.position.x = ndcX;
    transform.position.y = ndcY * -1;
    transform.scale = glm::vec3(width, height * -1, 1);
    _shaders.UI.SetMat4("model", transform.to_mat4());
    DrawMeshh(AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Quad"))->GetMeshIndices()[0]);
}

void Renderer_OLD::QueueUIForRendering(std::string textureName, int screenX, int screenY, bool centered, glm::vec3 color) {
    UIRenderInfo info;
    info.textureName = textureName;
    info.screenX = screenX;
	info.screenY = screenY;
	info.centered = centered;
	info.color = color;
    _UIRenderInfos.push_back(info);
}
void Renderer_OLD::QueueUIForRendering(UIRenderInfo renderInfo) {
    _UIRenderInfos.push_back(renderInfo);
}

void Renderer_OLD::RenderUI(float viewportWidth, float viewportHeight) {

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);

	_shaders.UI.Use();

    //if (EngineState::GetViewportMode() == SPLITSCREEN) {
      //  viewportHeight *= 0.5f;
    //}

    for (UIRenderInfo& uiRenderInfo : _UIRenderInfos) {
        AssetManager::GetTextureByName(uiRenderInfo.textureName)->GetGLTexture().Bind(0);
        Texture* texture = AssetManager::GetTextureByName(uiRenderInfo.textureName);
        _shaders.UI.SetVec3("color", uiRenderInfo.color);
        DrawQuad(viewportWidth, viewportHeight, texture->GetWidth(), texture->GetHeight(), uiRenderInfo.screenX, uiRenderInfo.screenY, uiRenderInfo.centered);
    }

    _UIRenderInfos.clear();
    glDisable(GL_BLEND);
}

int Renderer_OLD::GetRenderWidth() {
    return PRESENT_WIDTH;
}

int Renderer_OLD::GetRenderHeight() {
    return PRESENT_HEIGHT;
}

float Renderer_OLD::GetPointCloudSpacing() {
    return _pointCloudSpacing;
}

void Renderer_OLD::NextMode() {
    _mode = (RenderMode)(int(_mode) + 1);
    if (_mode == MODE_COUNT)
        _mode = (RenderMode)0;
}

void Renderer_OLD::PreviousMode() {
    if (int(_mode) == 0)
        _mode = RenderMode(int(MODE_COUNT) - 1);
    else
        _mode = (RenderMode)(int(_mode) - 1);
}

void Renderer_OLD::NextDebugLineRenderMode() {

    _debugLineRenderMode_OLD = (DebugLineRenderMode)(int(_debugLineRenderMode_OLD) + 1);

    /*
    SHOW_NO_LINES,
    PHYSX_ALL,
    PHYSX_RAYCAST,
    PHYSX_COLLISION,
    RAYTRACE_LAND,
    PHYSX_EDITOR,
    BOUNDING_BOXES,
    */

    if (_debugLineRenderMode_OLD == PHYSX_ALL) {
        _debugLineRenderMode_OLD = (DebugLineRenderMode)(int(_debugLineRenderMode_OLD) + 1);
    }
    if (_debugLineRenderMode_OLD == PHYSX_RAYCAST) {
      //  _debugLineRenderMode_OLD = (DebugLineRenderMode)(int(_debugLineRenderMode_OLD) + 1);
    }
    if (_debugLineRenderMode_OLD == PHYSX_COLLISION) {
   //     _debugLineRenderMode_OLD = (DebugLineRenderMode)(int(_debugLineRenderMode_OLD) + 1);
    }
    if (_debugLineRenderMode_OLD == RAYTRACE_LAND) {
    //    _debugLineRenderMode_OLD = (DebugLineRenderMode)(int(_debugLineRenderMode_OLD) + 1);
    }
    if (_debugLineRenderMode_OLD == PHYSX_EDITOR) {
        _debugLineRenderMode_OLD = (DebugLineRenderMode)(int(_debugLineRenderMode_OLD) + 1);
    }
    if (_debugLineRenderMode_OLD == BOUNDING_BOXES) {
   //     _debugLineRenderMode_OLD = (DebugLineRenderMode)(int(_debugLineRenderMode_OLD) + 1);
    }

    if (_debugLineRenderMode_OLD == DEBUG_LINE_MODE_COUNT) {
        _debugLineRenderMode_OLD = (DebugLineRenderMode)0;
    }     
}

void Renderer_OLD::QueueLineForDrawing(Line line) {
    _lines.push_back(line);
}

void Renderer_OLD::QueuePointForDrawing(Point point) {
    _points.push_back(point);
}

void Renderer_OLD::QueueTriangleForLineRendering(Triangle& triangle) {
    _lines.push_back(Line(triangle.p1, triangle.p2, triangle.color));
    _lines.push_back(Line(triangle.p2, triangle.p3, triangle.color));
    _lines.push_back(Line(triangle.p3, triangle.p1, triangle.color));
}

void Renderer_OLD::QueueTriangleForSolidRendering(Triangle& triangle) {
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

void DrawFullscreenQuadWithNormals() {
	static GLuint vao2 = 0;
	if (vao2 == 0) {
		float vertices[] = {
			// positions         normals            texcoords
            -1.0f,  1.0f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f,
			 1.0f,  1.0f, 0.0f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,
			 1.0f, -1.0f, 0.0f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f,
		};
		unsigned int vbo;
		glGenVertexArrays(1, &vao2);
		glGenBuffers(1, &vbo);
		glBindVertexArray(vao2);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		glEnableVertexAttribArray(2);
	}
	glBindVertexArray(vao2);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

void DrawMuzzleFlashes(Player* player) {

    int playerIndex = GetPlayerIndexFromPlayerPointer(player);
    PlayerRenderTarget& playerRenderTarget = GetPlayerRenderTarget(playerIndex);
    GBuffer& gBuffer = playerRenderTarget.gBuffer;

    static MuzzleFlash muzzleFlash; // has init on the first use. DISGUSTING. Fix if you ever see this when you aren't on another mission.

    // Bail if no flash    
    if (player->GetMuzzleFlashTime() < 0)
        return;
    if (player->GetMuzzleFlashTime() > 1)
        return;

    gBuffer.Bind();
    glDrawBuffer(GL_COLOR_ATTACHMENT3);
    glViewport(0, 0, gBuffer.GetWidth(), gBuffer.GetHeight());

    muzzleFlash.m_CurrentTime = player->GetMuzzleFlashTime();
    glm::vec3 worldPosition = glm::vec3(0);
    if (player->GetCurrentWeaponIndex() == GLOCK) {
        worldPosition = player->GetGlockBarrelPosition();
    }
    else if (player->GetCurrentWeaponIndex() == AKS74U) {
        worldPosition = player->GetFirstPersonWeapon().GetAKS74UBarrelPostion();
    }
    else if (player->GetCurrentWeaponIndex() == SHOTGUN) {
        worldPosition = player->GetFirstPersonWeapon().GetShotgunBarrelPosition();
    }
    else {
        return;
    }

    Transform t;
    t.position = worldPosition;
    t.rotation = player->GetViewRotation();

    // draw to lighting shader
    glDrawBuffer( GL_COLOR_ATTACHMENT3);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    /// this is sketchy. add this to player class.
    glm::mat4 projection = player->GetProjectionMatrix();
    glm::mat4 view = player->GetViewMatrix();
    glm::vec3 viewPos = player->GetViewPos();

    _shaders.animatedQuad.Use();
    _shaders.animatedQuad.SetMat4("u_MatrixProjection", projection);
    _shaders.animatedQuad.SetMat4("u_MatrixView", view);
    _shaders.animatedQuad.SetVec3("u_ViewPos", viewPos);

    glActiveTexture(GL_TEXTURE0);
    AssetManager::GetTextureByName("MuzzleFlash_ALB")->GetGLTexture().Bind(0);

    muzzleFlash.Draw(&_shaders.animatedQuad, t, player->GetMuzzleFlashRotation());
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
}

void DrawInstanced(Mesh* mesh, std::vector<glm::mat4>& matrices) {
    if (_ssbos.instanceMatrices == 0) {
        glCreateBuffers(1, &_ssbos.instanceMatrices);
        glNamedBufferStorage(_ssbos.instanceMatrices, 4096 * sizeof(glm::mat4), NULL, GL_DYNAMIC_STORAGE_BIT);
    }
    if (matrices.size()) {
        glNamedBufferSubData(_ssbos.instanceMatrices, 0, matrices.size() * sizeof(glm::mat4), &matrices[0]);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, _ssbos.instanceMatrices);
        glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
        glDrawElementsInstancedBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), matrices.size(), mesh->baseVertex);
    }
}


void DrawInstancedVAO(GLuint vao, GLsizei indexCount, std::vector<glm::mat4>& matrices) {
    if (_ssbos.instanceMatrices == 0) {
        glCreateBuffers(1, &_ssbos.instanceMatrices);
        glNamedBufferStorage(_ssbos.instanceMatrices, 4096 * sizeof(glm::mat4), NULL, GL_DYNAMIC_STORAGE_BIT);
    }
    if (matrices.size()) {
        glNamedBufferSubData(_ssbos.instanceMatrices, 0, matrices.size() * sizeof(glm::mat4), &matrices[0]);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, _ssbos.instanceMatrices);
        glBindVertexArray(vao);
        glDrawElementsInstanced(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0, matrices.size());
    }
}

void DrawBulletDecals(Player* player) {

    static Model* quadModel = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Quad"));

    unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments);
    glEnable(GL_CULL_FACE);        

    _shaders.bulletDecals.Use();
    _shaders.bulletDecals.SetMat4("projection", player->GetProjectionMatrix());
    _shaders.bulletDecals.SetMat4("view", player->GetViewMatrix());

    std::vector<glm::mat4> matrices;
    matrices.reserve(Scene::_decals.size());

    // Bullet holes
    for (Decal& decal : Scene::_decals) {
        if (decal.type == Decal::Type::REGULAR) {
            matrices.push_back(decal.GetModelMatrix());
        }
    }
    AssetManager::BindMaterialByIndex(AssetManager::GetMaterialIndex("BulletHole_Plaster"));
    DrawInstanced(AssetManager::GetMeshByIndex(quadModel->GetMeshIndices()[0]), matrices);

	// Glass bullet holes
    matrices.clear();
    for (Decal& decal : Scene::_decals) {
        if (decal.type == Decal::Type::GLASS) {
            matrices.push_back(decal.GetModelMatrix());
        }
    }
    AssetManager::BindMaterialByIndex(AssetManager::GetMaterialIndex("BulletHole_Glass"));
    DrawInstanced(AssetManager::GetMeshByIndex(quadModel->GetMeshIndices()[0]), matrices);
}


void DrawCasingProjectiles(Player* player) {

    glm::mat4 projection = player->GetProjectionMatrix();
    glm::mat4 view = player->GetViewMatrix();
    _shaders.geometry_instanced.Use();
    _shaders.geometry_instanced.SetMat4("projection", projection);
    _shaders.geometry_instanced.SetMat4("view", view);

    // GLOCK
    std::vector<glm::mat4> matrices;
    for (BulletCasing& casing : Scene::_bulletCasings) {
        if (casing.type == GLOCK) {
            matrices.push_back(casing.GetModelMatrix());
        }
    }
    AssetManager::BindMaterialByIndex(AssetManager::GetMaterialIndex("BulletCasing"));
    static Model* glockCasingModel = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("BulletCasing"));
    DrawInstanced(AssetManager::GetMeshByIndex(glockCasingModel->GetMeshIndices()[0]), matrices);
    
    // AKS74U
    matrices.clear();
    for (BulletCasing& casing : Scene::_bulletCasings) {
        if (casing.type == AKS74U) {
            matrices.push_back(casing.GetModelMatrix());
        }
    }
    AssetManager::BindMaterialByIndex(AssetManager::GetMaterialIndex("Casing_AkS74U"));
    static Model* akCasingModel = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("BulletCasing_AK"));
    DrawInstanced(AssetManager::GetMeshByIndex(akCasingModel->GetMeshIndices()[0]), matrices);

    // SHOTGUN
    matrices.clear();
    for (BulletCasing& casing : Scene::_bulletCasings) {
        if (casing.type == SHOTGUN) {
            matrices.push_back(casing.GetModelMatrix());
        }
    }
    AssetManager::BindMaterialByIndex(AssetManager::GetMaterialIndex("Shell"));

    static Model* shellModel = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Shell"));
    DrawInstanced(AssetManager::GetMeshByIndex(shellModel->GetMeshIndices()[0]), matrices);
}

void RenderShadowMaps() {

    _shaders.shadowMap.Use();
    _shaders.shadowMap.SetFloat("far_plane", SHADOW_FAR_PLANE);
    _shaders.shadowMap.SetMat4("model", glm::mat4(1));
    glDepthMask(true);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);

    for (int i = 0; i < Scene::_lights.size(); i++) {

        bool skip = false;

        if (EngineState::GetEngineMode() == EngineMode::EDITOR) {
            skip = false;
        }

        if (skip) {
            continue;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, _shadowMaps[i]._ID);
        glClear(GL_DEPTH_BUFFER_BIT);

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
}

void Renderer_OLD::CreatePointCloudBuffer() {

    if (Scene::_cloudPoints.empty()) {
        return;
    }

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
}

void DrawPointCloud(Player* player) {
    _shaders.debugViewPointCloud.Use();
    _shaders.debugViewPointCloud.SetMat4("projection", player->GetProjectionMatrix());// Renderer::GetProjectionMatrix(_depthOfFieldScene));
    _shaders.debugViewPointCloud.SetMat4("view", player->GetViewMatrix());
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
    glDeleteBuffers(1, &_ssbos.rtVertices);
    glDeleteBuffers(1, &_ssbos.rtMesh);
    glGenBuffers(1, &_ssbos.rtInstances);
    glGenBuffers(1, &_ssbos.dirtyPointCloudIndices);

    _shaders.compute.LoadOLD("res/shaders/OpenGL_OLD/compute.comp");
    _shaders.pointCloud.LoadOLD("res/shaders/OpenGL_OLD/point_cloud.comp");
    _shaders.propogateLight.LoadOLD("res/shaders/OpenGL_OLD/propogate_light.comp");
    
    Scene::CreatePointCloud();
    Renderer_OLD::CreatePointCloudBuffer();
    Renderer_OLD::CreateTriangleWorldVertexBuffer();
    
   // std::cout << "Point cloud has " << Scene::_cloudPoints.size() << " points\n";
   // std::cout << "Propagation grid has " << (_mapWidth * _mapHeight * _mapDepth / _propogationGridSpacing) << " cells\n";

    // Propogation List
   /*glGenBuffers(1, &_ssbos.propogationList);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _ssbos.propogationList);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, _gridTextureSize * sizeof(glm::uvec4), nullptr, GL_DYNAMIC_STORAGE_BIT);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
    std::cout << "The propogation list has room for " << _gridTextureSize << " uvec4 elements\n";*/
}

void Renderer_OLD::CreateTriangleWorldVertexBuffer() {

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

    //std::cout << "You are raytracing against " << (vertices.size() / 3) << " tris\n";
    //std::cout << "You are raytracing " << Scene::_rtMesh.size() << " mesh\n";
}

void ComputePass() {
    if (Scene::_rtInstances.empty()) {
        return;
    }

    // Hack to disable compute on toggle
    static bool disableCompute = false;
    if (Input::KeyPressed(HELL_KEY_O)) {
        disableCompute = !disableCompute;
    }
	if (disableCompute) {
        return;
	}

    // Update RT Instances
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _ssbos.rtInstances);
    glBufferData(GL_SHADER_STORAGE_BUFFER, Scene::_rtInstances.size() * sizeof(RTInstance), &Scene::_rtInstances[0], GL_DYNAMIC_COPY);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
    UpdatePointCloudLighting();
    UpdatePropogationgGrid();

	// Hack to initially disable compute after the first time it runs
    static bool runOnce = true;
    if (runOnce) {
        disableCompute = true;
        runOnce = false;
    }

	// Composite
	//_shaders.computeTest.Use();
	//glBindImageTexture(0, gBuffer.lightingTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
	//glDispatchCompute(gBuffer.GetWidth() / 8, gBuffer.GetHeight() / 8, 1);
	//glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
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
 
    if (!_dirtyPointCloudIndices.empty()) {
        // Cloud point indices buffer
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, _ssbos.dirtyPointCloudIndices);
        glBufferData(GL_SHADER_STORAGE_BUFFER, _dirtyPointCloudIndices.size() * sizeof(int), &_dirtyPointCloudIndices[0], GL_STATIC_COPY);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, _ssbos.dirtyPointCloudIndices);

        //std::cout << "_cloudPointIndices.size(): " << _cloudPointIndices.size() << "\n";
        glDispatchCompute(std::ceil(_dirtyPointCloudIndices.size() / 64.0f), 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }
}

void FindProbeCoordsWithinMapBounds() {

    _probeCoordsWithinMapBounds.clear();
    _probeWorldPositionsWithinMapBounds.clear();

    //Timer t("FindProbeCoordsWithinMapBounds()");

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
    for (int z = 0; z < _gridTextureWidth; z++) {
        for (int y = 0; y < _gridTextureHeight; y++) {
            for (int x = 0; x < _gridTextureDepth; x++) {

             
                bool foundOne = false;

                glm::vec3 probePosition = glm::vec3(x, y, z) * _propogationGridSpacing;

                // Check floors
                for (int j = 0; j < floorVertices.size(); j += 3) {
                    if (probePosition.y < floorVertices[j].w) {
                        continue;
                    }
                    glm::vec2 probePos = glm::vec2(probePosition.x, probePosition.z);
                    glm::vec2 v1 = glm::vec2(floorVertices[j + 0].x, floorVertices[j + 0].z); // when you remove this gen code then it doesn't generate any gridIndices meaning no indirectLight
                    glm::vec2 v2 = glm::vec2(floorVertices[j + 1].x, floorVertices[j + 1].z);
                    glm::vec2 v3 = glm::vec2(floorVertices[j + 2].x, floorVertices[j + 2].z);

                    // If you are above one, check if you are also below a ceiling
                    if (Util::PointIn2DTriangle(probePos, v1, v2, v3)) {

                        if (probePosition.y < 2.6f) {
                            _probeCoordsWithinMapBounds.emplace_back(x, y, z, 0);
                            _probeWorldPositionsWithinMapBounds.emplace_back(probePosition);
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

    //std::cout << "There are " << _probeCoordsWithinMapBounds.size() << " probes within rooms\n";    
}

void UpdatePropogationgGrid() {
    
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, _ssbos.dirtyPointCloudIndices);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, _ssbos.rtVertices);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, _pointCloud.VBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, _ssbos.rtMesh);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, _ssbos.rtInstances);
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
    glBufferData(GL_SHADER_STORAGE_BUFFER, _dirtyProbeCount * sizeof(glm::uvec4), &_dirtyProbeCoords[0], GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, _ssbos.dirtyGridCoordinates);

    if (_dirtyProbeCount > 0) {
        int invocationCount = (int)(std::ceil(_dirtyProbeCoords.size() / 64.0f));
        glDispatchCompute(invocationCount, 1, 1);
    }
}

void CalculateDirtyCloudPoints() {
    // If the area within the light's radius has been modified, queue all the relevant cloud points
    _dirtyPointCloudIndices.clear();
    _dirtyPointCloudIndices.reserve(Scene::_cloudPoints.size());
    for (int j = 0; j < Scene::_cloudPoints.size(); j++) {
        CloudPoint& cloudPoint = Scene::_cloudPoints[j];
        for (auto& light : Scene::_lights) {
            const float lightRadiusSquared = light.radius * light.radius;
            if (light.isDirty) {
                if (Util::DistanceSquared(cloudPoint.position, light.position) < lightRadiusSquared) {
                    _dirtyPointCloudIndices.push_back(j);
                    break;
                }
            }
        }
    }
}

void CalculateDirtyProbeCoords() {

    if (_dirtyProbeCoords.size() == 0) {
        _dirtyProbeCoords.resize(_gridTextureSize);
    }

    //Timer t("_dirtyProbeCoords");
    _dirtyProbeCount = 0;

    // Early out AABB around entire dirty point cloud
    float cloudMinX = 9999;
    float cloudMinY = 9999;
    float cloudMinZ = 9999;
    float cloudMaxX = -9999;
    float cloudMaxY = -9999;
    float cloudMaxZ = -9999;
    for (int& index : _dirtyPointCloudIndices) {
        cloudMinX = std::min(cloudMinX, Scene::_cloudPoints[index].position.x);
        cloudMaxX = std::max(cloudMaxX, Scene::_cloudPoints[index].position.x);
        cloudMinY = std::min(cloudMinY, Scene::_cloudPoints[index].position.y);
        cloudMaxY = std::max(cloudMaxY, Scene::_cloudPoints[index].position.y);
        cloudMinZ = std::min(cloudMinZ, Scene::_cloudPoints[index].position.z);
        cloudMaxZ = std::max(cloudMaxZ, Scene::_cloudPoints[index].position.z);
    }

    for (int i = 0; i < _probeCoordsWithinMapBounds.size(); i++) {

        const glm::vec3 probePosition = _probeWorldPositionsWithinMapBounds[i];

        for (int& index : _dirtyPointCloudIndices) {
            const glm::vec3& cloudPointPosition = Scene::_cloudPoints[index].position;

            // AABB early out
            if (probePosition.x - _maxPropogationDistance > cloudPointPosition.x ||
                probePosition.y - _maxPropogationDistance > cloudPointPosition.y ||
                probePosition.z - _maxPropogationDistance > cloudPointPosition.z ||
                probePosition.x + _maxPropogationDistance < cloudPointPosition.x ||
                probePosition.y + _maxPropogationDistance < cloudPointPosition.y ||
                probePosition.z + _maxPropogationDistance < cloudPointPosition.z) {
                continue;
            }

            if (Util::DistanceSquared(cloudPointPosition, probePosition) < _maxDistanceSquared) {

                // skip probe if cloud point faces away from probe 
                glm::vec3 cloudPointNormal = Scene::_cloudPoints[index].normal;
                if (dot(cloudPointPosition - probePosition, cloudPointNormal) > 0.0) {
                    continue;
                }
                _dirtyProbeCoords[_dirtyProbeCount] = _probeCoordsWithinMapBounds[i];;
                _dirtyProbeCount++;
                break;
            }
        }
    }
}


void QueueAABBForRenering(AABB& aabb, glm::vec3 color) {

    glm::vec3 b = aabb.extents;
    glm::vec3 frontBottomLeft = aabb.position + glm::vec3(-b.x, -b.y, -b.z);
    glm::vec3 frontBottomRight = aabb.position + glm::vec3(b.x, -b.y, -b.z);
    glm::vec3 frontTopLeft = aabb.position + glm::vec3(-b.x, b.y, -b.z);
    glm::vec3 frontTopRight = aabb.position + glm::vec3(b.x, b.y, -b.z);
    glm::vec3 backBottomLeft = aabb.position + glm::vec3(-b.x, -b.y, b.z);
    glm::vec3 backBottomRight = aabb.position + glm::vec3(b.x, -b.y, b.z);
    glm::vec3 backTopLeft = aabb.position + glm::vec3(-b.x, b.y, b.z);
    glm::vec3 backTopRight = aabb.position + glm::vec3(b.x, b.y, b.z);

    Line line;
    line.p1.color = color;
    line.p2.color = color;
    line.p1.pos = frontBottomLeft;
    line.p2.pos = frontBottomRight;
    Renderer_OLD::QueueLineForDrawing(line);
    line.p1.pos = frontTopLeft;
    line.p2.pos = frontTopRight;
    Renderer_OLD::QueueLineForDrawing(line);
    line.p1.pos = frontBottomLeft;
    line.p2.pos = frontTopLeft;
    Renderer_OLD::QueueLineForDrawing(line);
    line.p1.pos = frontBottomRight;
    line.p2.pos = frontTopRight;
    Renderer_OLD::QueueLineForDrawing(line);

    line.p1.pos = backBottomLeft;
    line.p2.pos = backBottomRight;
    Renderer_OLD::QueueLineForDrawing(line);
    line.p1.pos = backTopLeft;
    line.p2.pos = backTopRight;
    Renderer_OLD::QueueLineForDrawing(line);
    line.p1.pos = backBottomLeft;
    line.p2.pos = backTopLeft;
    Renderer_OLD::QueueLineForDrawing(line);
    line.p1.pos = backBottomRight;
    line.p2.pos = backTopRight;
    Renderer_OLD::QueueLineForDrawing(line);

    line.p1.pos = frontBottomLeft;
    line.p2.pos = backBottomLeft;
    Renderer_OLD::QueueLineForDrawing(line);
    line.p1.pos = frontBottomRight;
    line.p2.pos = backBottomRight;
    Renderer_OLD::QueueLineForDrawing(line);
    line.p1.pos = frontTopLeft;
    line.p2.pos = backTopLeft;
    Renderer_OLD::QueueLineForDrawing(line);
    line.p1.pos = frontTopRight;
    line.p2.pos = backTopRight;
    Renderer_OLD::QueueLineForDrawing(line);
}

void QueueAABBForRenering(PxRigidBody* body) {

    PxVec3 worldBounds = body->getWorldBounds().getExtents();
    PxVec3 worldPosition = body->getWorldBounds().getCenter();

    glm::vec3 b = Util::PxVec3toGlmVec3(worldBounds);
    glm::vec3 frontBottomLeft = Util::PxVec3toGlmVec3(worldPosition) + glm::vec3(-b.x, -b.y, -b.z);
    glm::vec3 frontBottomRight = Util::PxVec3toGlmVec3(worldPosition) + glm::vec3(b.x, -b.y, -b.z);
    glm::vec3 frontTopLeft = Util::PxVec3toGlmVec3(worldPosition) + glm::vec3(-b.x, b.y, -b.z);
    glm::vec3 frontTopRight = Util::PxVec3toGlmVec3(worldPosition) + glm::vec3(b.x, b.y, -b.z);
    glm::vec3 backBottomLeft = Util::PxVec3toGlmVec3(worldPosition) + glm::vec3(-b.x, -b.y, b.z);
    glm::vec3 backBottomRight = Util::PxVec3toGlmVec3(worldPosition) + glm::vec3(b.x, -b.y, b.z);
    glm::vec3 backTopLeft = Util::PxVec3toGlmVec3(worldPosition) + glm::vec3(-b.x, b.y, b.z);
    glm::vec3 backTopRight = Util::PxVec3toGlmVec3(worldPosition) + glm::vec3(b.x, b.y, b.z);

    Line line;
    line.p1.color = YELLOW;
    line.p2.color = YELLOW;
    line.p1.pos = frontBottomLeft;
    line.p2.pos = frontBottomRight;
    Renderer_OLD::QueueLineForDrawing(line);
    line.p1.pos = frontTopLeft;
    line.p2.pos = frontTopRight;
    Renderer_OLD::QueueLineForDrawing(line);
    line.p1.pos = frontBottomLeft;
    line.p2.pos = frontTopLeft;
    Renderer_OLD::QueueLineForDrawing(line);
    line.p1.pos = frontBottomRight;
    line.p2.pos = frontTopRight;
    Renderer_OLD::QueueLineForDrawing(line);

    line.p1.pos = backBottomLeft;
    line.p2.pos = backBottomRight;
    Renderer_OLD::QueueLineForDrawing(line);
    line.p1.pos = backTopLeft;
    line.p2.pos = backTopRight;
    Renderer_OLD::QueueLineForDrawing(line);
    line.p1.pos = backBottomLeft;
    line.p2.pos = backTopLeft;
    Renderer_OLD::QueueLineForDrawing(line);
    line.p1.pos = backBottomRight;
    line.p2.pos = backTopRight;
    Renderer_OLD::QueueLineForDrawing(line);

    line.p1.pos = frontBottomLeft;
    line.p2.pos = backBottomLeft;
    Renderer_OLD::QueueLineForDrawing(line);
    line.p1.pos = frontBottomRight;
    line.p2.pos = backBottomRight;
    Renderer_OLD::QueueLineForDrawing(line);
    line.p1.pos = frontTopLeft;
    line.p2.pos = backTopLeft;
    Renderer_OLD::QueueLineForDrawing(line);
    line.p1.pos = frontTopRight;
    line.p2.pos = backTopRight;
    Renderer_OLD::QueueLineForDrawing(line);
}

void QueueAABBForRenering(PxRigidStatic* body) {
    QueueAABBForRenering((PxRigidBody*)body);
}