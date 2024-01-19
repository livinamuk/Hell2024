#include "BS_thread_pool.hpp"
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
#include "../Core/Physics.h"
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
#include "../Core/DebugMenu.h"

std::vector<glm::vec3> debugPoints;


struct RenderTarget {
    GLuint fbo = { 0 };
	GLuint texture = { 0 };
	GLuint width = { 0 };
	GLuint height = { 0 };

    void Create(int width, int height) {
		if (fbo != 0) {
			glDeleteFramebuffers(1, &fbo);
		}
		glGenFramebuffers(1, &fbo);
		this->width = width;
		this->height = height;
    }
};

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
    ComputeShader compute;
	ComputeShader pointCloud;
	ComputeShader propogateLight;
	ComputeShader computeTest;
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
Mesh _cubeMesh;
unsigned int _pointLineVAO;
unsigned int _pointLineVBO;
int _renderWidth = 768;// 512 * 1.5f;
int _renderHeight = 432;// 288 * 1.5f;

std::vector<PlayerRenderTarget> _playerRenderTargets;
std::vector<Point> _points;
std::vector<Point> _solidTrianglePoints;
std::vector<Line> _lines;
std::vector<UIRenderInfo> _UIRenderInfos;
std::vector<ShadowMap> _shadowMaps;
GLuint _imageStoreTexture = { 0 };
bool _depthOfFieldScene = 0.9f;
bool _depthOfFieldWeapon = 1.0f;
const float _propogationGridSpacing = 0.4f;
const float _pointCloudSpacing = 0.4f;
const float _maxPropogationDistance = 2.6f; 
const float _maxDistanceSquared = _maxPropogationDistance * _maxPropogationDistance;
float _mapWidth = 16;
float _mapHeight = 8;
float _mapDepth = 16; 
//std::vector<int> _newDirtyPointCloudIndices;
int _floorVertexCount;
Mesh _quadMesh;

const int _gridTextureWidth = (int)(_mapWidth / _propogationGridSpacing);
const int _gridTextureHeight = (int)(_mapHeight / _propogationGridSpacing);
const int _gridTextureDepth = (int)(_mapDepth / _propogationGridSpacing);
const int _gridTextureSize = (int)(_gridTextureWidth * _gridTextureHeight * _gridTextureDepth);
//const int _xSize = std::ceil(_gridTextureWidth * 0.25f);
//const int _ySize = std::ceil(_gridTextureHeight * 0.25f);
//const int _zSize = std::ceil(_gridTextureDepth * 0.25f);

//std::vector<glm::uvec4> _gridIndices;
//std::vector<glm::uvec4> _newGridIndices;
static std::vector<glm::uvec4> _probeCoordsWithinMapBounds;
static std::vector<glm::vec3> _probeWorldPositionsWithinMapBounds;
std::vector<glm::uvec4> _dirtyProbeCoords;
std::vector<int> _dirtyPointCloudIndices;
int _dirtyProbeCount;

std::vector<glm::mat4> _glockMatrices;
std::vector<glm::mat4> _aks74uMatrices;



enum RenderMode { COMPOSITE, DIRECT_LIGHT, INDIRECT_LIGHT, POINT_CLOUD, MODE_COUNT } _mode;
enum DebugLineRenderMode { SHOW_NO_LINES, PHYSX_ALL, PHYSX_RAYCAST, PHYSX_COLLISION, RAYTRACE_LAND, DEBUG_LINE_MODE_COUNT} _debugLineRenderMode;

void DrawHud(Player* player);
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
void BlitDebugTexture(GLint fbo, GLenum colorAttachment, GLint srcWidth, GLint srcHeight);
void DrawQuad(int viewportWidth, int viewPortHeight, int textureWidth, int textureHeight, int xPos, int yPos, bool centered = false, float scale = 1.0f, int xSize = -1, int ySize = -1);
void SkyBoxPass(Player* player);


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

void GlassPass(Player* player) {


	glm::mat4 projection = Renderer::GetProjectionMatrix(_depthOfFieldScene);
	glm::mat4 view = player->GetViewMatrix();

	int playerIndex = GetPlayerIndexFromPlayerPointer(player);
	PlayerRenderTarget& playerRenderTarget = GetPlayerRenderTarget(playerIndex);
	GBuffer& gBuffer = playerRenderTarget.gBuffer;
    

	gBuffer.Bind();
	unsigned int attachments[1] = { GL_COLOR_ATTACHMENT4 };
	glDrawBuffers(1, attachments);
	glViewport(0, 0, gBuffer.GetWidth(), gBuffer.GetHeight());
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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
	glBindTexture(GL_TEXTURE_2D, gBuffer._gLightingTexture);

	//glDisable(GL_CULL_FACE);
	for (Window& window : Scene::_windows) {
		_shaders.glass.SetMat4("model", window.GetModelMatrix());
        AssetManager::GetModel("Glass")->Draw();
	}


	_shaders.glassComposite.Use();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gBuffer._gLightingTexture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gBuffer._gGlassTexture);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, _blurBuffers[0].textureB);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, _blurBuffers[1].textureB);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, _blurBuffers[2].textureB);
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, _blurBuffers[3].textureB);

	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, gBuffer._gDepthTexture);


    glDisable(GL_DEPTH_TEST);
	unsigned int attachments2[1] = { GL_COLOR_ATTACHMENT5 };
	glDrawBuffers(1, attachments2);
    DrawFullscreenQuad();


	//glBindFramebuffer(GL_READ_FRAMEBUFFER, playerRenderTarget.gBuffer.GetID());
	//glBindFramebuffer(GL_DRAW_FRAMEBUFFER, playerRenderTarget.gBuffer.GetID());
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

//std::vector<glm::uvec4> GridIndicesUpdate(std::vector<glm::uvec4> allGridIndicesWithinRooms);

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

void Renderer::InitMinimumToRenderLoadingFrame() {

	_shaders.UI.Load("ui.vert", "ui.frag");

	// Menu FBO
	_menuRenderTarget.Create(_renderWidth, _renderHeight);
	glBindFramebuffer(GL_FRAMEBUFFER, _menuRenderTarget.fbo);
	glGenTextures(1, &_menuRenderTarget.texture);
	glBindTexture(GL_TEXTURE_2D, _menuRenderTarget.texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _renderWidth, _renderHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _menuRenderTarget.texture, 0);

}

void Renderer::Init() {

    glGenVertexArrays(1, &_pointLineVAO);
    glGenBuffers(1, &_pointLineVBO);
    glPointSize(2);

    _shaders.solidColor.Load("solid_color.vert", "solid_color.frag");
    _shaders.shadowMap.Load("shadowmap.vert", "shadowmap.frag", "shadowmap.geom");
    _shaders.editorSolidColor.Load("editor_solid_color.vert", "editor_solid_color.frag");
    _shaders.composite.Load("composite.vert", "composite.frag");
    _shaders.fxaa.Load("fxaa.vert", "fxaa.frag");
    _shaders.animatedQuad.Load("animated_quad.vert", "animated_quad.frag");
    _shaders.depthOfField.Load("depth_of_field.vert", "depth_of_field.frag");
    _shaders.debugViewPointCloud.Load("debug_view_point_cloud.vert", "debug_view_point_cloud.frag");
    _shaders.geometry.Load("geometry.vert", "geometry.frag");
    _shaders.geometry_instanced.Load("geometry_instanced.vert", "geometry_instanced.frag");
    _shaders.lighting.Load("lighting.vert", "lighting.frag");
    _shaders.debugViewPropgationGrid.Load("debug_view_propogation_grid.vert", "debug_view_propogation_grid.frag");
	_shaders.editorTextured.Load("editor_textured.vert", "editor_textured.frag");
	_shaders.bulletDecals.Load("bullet_decals.vert", "bullet_decals.frag");
	_shaders.glass.Load("glass.vert", "glass.frag");
	_shaders.glassComposite.Load("glass_composite.vert", "glass_composite.frag");
	_shaders.skybox.Load("skybox.vert", "skybox.frag");
	_shaders.computeTest.Load("res/shaders/test.comp");
    
	_shaders.blurVertical.Load("blurVertical.vert", "blur.frag");
	_shaders.blurHorizontal.Load("blurHorizontal.vert", "blur.frag");
    
    _cubeMesh = MeshUtil::CreateCube(1.0f, 1.0f, true);

    RecreateFrameBuffers(0);


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

void Renderer::RenderLoadingFrame() {

    glBindFramebuffer(GL_FRAMEBUFFER, _menuRenderTarget.fbo);
	glViewport(0, 0, _menuRenderTarget.width, _menuRenderTarget.height);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	glClearColor(0, 0, 0.1f, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);

	_shaders.UI.Use();
	_shaders.UI.SetVec3("color", WHITE);
	_shaders.UI.SetVec3("overrideColor", WHITE);

    Transform transform;
    transform.scale = glm::vec3(0.15f, 0.15f, 1.0f);
	_shaders.UI.SetMat4("model", transform.to_mat4());

    std::string text = "We are all alone on life's journey, held captive by the limitations of human consciousness.\n";
    for (auto& str : AssetManager::_loadLog) {
        text += str + "\n";
    }
	TextBlitter::_debugTextToBilt = text;
    TextBlitter::Update(1.0f / 60.0f);
	Renderer::RenderUI();    

	glViewport(0, 0, GL::GetWindowWidth(), GL::GetWindowHeight());
	glBindFramebuffer(GL_READ_FRAMEBUFFER, _menuRenderTarget.fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glBlitFramebuffer(0, 0, _menuRenderTarget.width, _menuRenderTarget.height, 0, 0, GL::GetWindowWidth(), GL::GetWindowHeight(), GL_COLOR_BUFFER_BIT, GL_NEAREST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


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

			width *= 0.5f;
            height *= 0.5f;
        }
    }


	int playerIndex = GetPlayerIndexFromPlayerPointer(player);
	PlayerRenderTarget& playerRenderTarget = GetPlayerRenderTarget(playerIndex);
	GBuffer& gBuffer = playerRenderTarget.gBuffer;

	

    // first round

	glBindFramebuffer(GL_FRAMEBUFFER, _blurBuffers[0].FBO);
	glViewport(0, 0, _blurBuffers[0].width, _blurBuffers[0].height);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gBuffer._gEmissive);
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

void Renderer::RenderFrame(Player* player) {

    if (!player) {
        return;
    }

    int playerIndex = GetPlayerIndexFromPlayerPointer(player);

    if (playerIndex == -1) {
        return;
    }

    PlayerRenderTarget& playerRenderTarget = GetPlayerRenderTarget(playerIndex);
    GBuffer& gBuffer = playerRenderTarget.gBuffer;
    PresentFrameBuffer& presentFrameBuffer = playerRenderTarget.presentFrameBuffer;

	_shaders.UI.Use();
	_shaders.UI.SetVec3("overrideColor", WHITE);

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

    DrawBulletDecals(player);
    DrawCasingProjectiles(player);
    LightingPass(player);
    SkyBoxPass(player);


    BlurEmissiveBulbs(player);

	GlassPass(player);

    DrawMuzzleFlashes(player);
    
    // Propagation Grid
    if (_toggles.drawProbes) {
        Transform cubeTransform;
        cubeTransform.scale = glm::vec3(0.025f);
        _shaders.debugViewPropgationGrid.Use();
        _shaders.debugViewPropgationGrid.SetMat4("projection", GetProjectionMatrix(_depthOfFieldScene));
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
        glBindVertexArray(_cubeMesh.GetVAO());
        glDrawElementsInstanced(GL_TRIANGLES, _cubeMesh.GetIndexCount(), GL_UNSIGNED_INT, 0, count);
	}

    // HUD
    glDisable(GL_DEPTH_TEST);
	glDrawBuffer(GL_COLOR_ATTACHMENT3);
	DrawHud(player);

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

    // Render UI
    presentFrameBuffer.Bind();
    glDrawBuffer(GL_COLOR_ATTACHMENT1);
    std::string texture = "CrosshairDot";
    if (player->CursorShouldBeInterect()) {
        texture = "CrosshairSquare";
    }
    QueueUIForRendering(texture, presentFrameBuffer.GetWidth() / 2, presentFrameBuffer.GetHeight() / 2, true, WHITE);

    /*if (Scene::_cameraRayData.found) {
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
    */

	TextBlitter::_debugTextToBilt += "View pos: " + Util::Vec3ToString(player->GetViewPos()) + "\n";
	TextBlitter::_debugTextToBilt += "View rot: " + Util::Vec3ToString(player->GetViewRotation()) + "\n";
	//TextBlitter::_debugTextToBilt += "Grounded: " + std::to_string(Scene::_players[0]._isGrounded) + "\n";
	//TextBlitter::_debugTextToBilt += "Y vel: " + std::to_string(Scene::_players[0]._yVelocity) + "\n";
    //TextBlitter::_debugTextToBilt = "";
    
    if (!_toggles.drawDebugText) {
        TextBlitter::_debugTextToBilt = "";
    }

    TextBlitter::BlitAtPosition(player->_pickUpText, 60, presentFrameBuffer.GetHeight() - 60, false, 1.0f);

    glBindFramebuffer(GL_FRAMEBUFFER, presentFrameBuffer.GetID());
    glViewport(0, 0, presentFrameBuffer.GetWidth(), presentFrameBuffer.GetHeight());
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    Renderer::RenderUI();



    // Blit that smaller FBO into the main frame buffer 
    if (_viewportMode == FULLSCREEN) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, playerRenderTarget.presentFrameBuffer.GetID());
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glReadBuffer(GL_COLOR_ATTACHMENT1);
        glBlitFramebuffer(0, 0, playerRenderTarget.presentFrameBuffer.GetWidth(), playerRenderTarget.presentFrameBuffer.GetHeight(), 0, 0, GL::GetWindowWidth(), GL::GetWindowHeight(), GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }
    else if (_viewportMode == SPLITSCREEN) {

        if (GetPlayerIndexFromPlayerPointer(player) == 0) {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, playerRenderTarget.presentFrameBuffer.GetID());
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
            glReadBuffer(GL_COLOR_ATTACHMENT1);
            glBlitFramebuffer(0, 0, playerRenderTarget.presentFrameBuffer.GetWidth(), playerRenderTarget.presentFrameBuffer.GetHeight(), 0, GL::GetWindowHeight() / 2, GL::GetWindowWidth(), GL::GetWindowHeight(), GL_COLOR_BUFFER_BIT, GL_NEAREST);
        }
        if (GetPlayerIndexFromPlayerPointer(player) == 1) {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, playerRenderTarget.presentFrameBuffer.GetID());
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
            glReadBuffer(GL_COLOR_ATTACHMENT1);
            glBlitFramebuffer(0, 0, playerRenderTarget.presentFrameBuffer.GetWidth(), playerRenderTarget.presentFrameBuffer.GetHeight(), 0, 0, GL::GetWindowWidth(), GL::GetWindowHeight() / 2, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        }
    }

  //  BlitDebugTexture(_blurBuffers[2].FBO, GL_COLOR_ATTACHMENT1, _blurBuffers[2].width, _blurBuffers[2].height);


    // Wipe all light dirty flags back to false
    for (Light& light : Scene::_lights) {
        light.isDirty = false;
    }
}


struct CubemapTexutre {
    struct face_info{
        uint8_t *texture{ nullptr };
        int32_t width{};
        int32_t height{};
        int32_t format{ GL_RGB };
        uint8_t id{};
    };

	GLuint ID;

	void Create(std::vector<std::string>& textures) {
		glGenTextures(1, &ID);
		glBindTexture(GL_TEXTURE_CUBE_MAP, ID);

        BS::thread_pool pool(textures.size());
        std::vector<face_info> result(textures.size());

        pool.detach_loop(size_t{}, textures.size(), [&textures, &result](const size_t i) {
            face_info &info{ result[i] };
		    int32_t nrChannels;
            info.id = static_cast<uint8_t>(i);
			info.texture = stbi_load(textures[i].c_str(), &info.width, &info.height, &nrChannels, 0);
			if (info.texture) {
				if (nrChannels == 4)
					info.format = GL_RGBA;
				if (nrChannels == 1)
					info.format = GL_RED;
			} else {
				std::cout << "Failed to load cubemap\n";
                stbi_image_free(info.texture);
                info.texture = nullptr;
			}
            return info;
        });
        pool.wait();

        for (auto info : result) {
            if (info.texture == nullptr) {
                continue;
            }
		    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + info.id, 0, GL_RGBA,
                info.width, info.height, 0, info.format, GL_UNSIGNED_BYTE, info.texture);

            stbi_image_free(info.texture);
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
		skyboxTextureFilePaths.push_back("res/textures/right.png");
		skyboxTextureFilePaths.push_back("res/textures/left.png");
		skyboxTextureFilePaths.push_back("res/textures/top.png");
		skyboxTextureFilePaths.push_back("res/textures/bottom.png");
		skyboxTextureFilePaths.push_back("res/textures/front.png");
		skyboxTextureFilePaths.push_back("res/textures/back.png");
		_skyboxTexture.Create(skyboxTextureFilePaths);

    }







	glm::mat4 projection = glm::perspective(1.0f, 1920.0f / 1080.0f, NEAR_PLANE, FAR_PLANE);
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


void DrawHud(Player* player) {

	int playerIndex = GetPlayerIndexFromPlayerPointer(player);
	PlayerRenderTarget& playerRenderTarget = GetPlayerRenderTarget(playerIndex);
	GBuffer& gBuffer = playerRenderTarget.gBuffer;

	_shaders.UI.Use();
	_shaders.UI.SetMat4("model", glm::mat4(1));
    AssetManager::GetTexture("NumSheet")->Bind(0);

	float scale = 1.1f;
	unsigned int slashXPos = gBuffer.GetWidth() - 180.0f;
    unsigned int slashYPos = gBuffer.GetHeight() - 120.0f;
	float viewportWidth = gBuffer.GetWidth();
	float viewportHeight = gBuffer.GetHeight();

    if (Renderer::_viewportMode == SPLITSCREEN) {
        slashYPos = gBuffer.GetHeight() - 70.0f;
        scale = 1.05f;
    }
    glm::vec3 ammoColor = glm::vec3(0.16f, 0.78f, 0.23f);

    /*
	int Player::GetCurrentWeaponClipAmmo() {
	int Player::GetCurrentWeaponTotalAmmo() {
    */

    if (player->GetCurrentWeaponIndex() == Weapon::KNIFE) {
        return;
    }

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

void GeometryPass(Player* player) {
    glm::mat4 projection = Renderer::GetProjectionMatrix(_depthOfFieldScene); // 1.0 for weapon, 0.9 for scene.
    glm::mat4 view = player->GetViewMatrix();
    
    int playerIndex = GetPlayerIndexFromPlayerPointer(player);
    PlayerRenderTarget& playerRenderTarget = GetPlayerRenderTarget(playerIndex);
    GBuffer& gBuffer = playerRenderTarget.gBuffer;

    gBuffer.Bind();
    unsigned int attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT6 };
    glDrawBuffers(4, attachments);
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
}

void LightingPass(Player* player) {

    int playerIndex = GetPlayerIndexFromPlayerPointer(player);
    PlayerRenderTarget& playerRenderTarget = GetPlayerRenderTarget(playerIndex);
    GBuffer& gBuffer = playerRenderTarget.gBuffer;

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


    if (_debugLineRenderMode == DebugLineRenderMode::SHOW_NO_LINES) {
        // Do nothing
    }
    else if (_debugLineRenderMode == DebugLineRenderMode::PHYSX_ALL) {
        TextBlitter::_debugTextToBilt += "Line Mode: PHYSX ALL\n";
    }
    else if (_debugLineRenderMode == DebugLineRenderMode::PHYSX_RAYCAST) {
        TextBlitter::_debugTextToBilt += "Line Mode: PHYSX RAYCAST\n";
    }
    else if (_debugLineRenderMode == DebugLineRenderMode::PHYSX_COLLISION) {
        TextBlitter::_debugTextToBilt += "Line Mode: PHYSX COLLISION\n";
    }
    else if (_debugLineRenderMode == DebugLineRenderMode::RAYTRACE_LAND) {
        TextBlitter::_debugTextToBilt += "Line Mode: COMPUTE RAY WORLD\n";
    }

    
  
    gBuffer.Bind();
    glDrawBuffer(GL_COLOR_ATTACHMENT3);
    glViewport(0, 0, gBuffer.GetWidth(), gBuffer.GetHeight());
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gBuffer._gBaseColorTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gBuffer._gNormalTexture);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gBuffer._gRMATexture);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, gBuffer._gDepthTexture);
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
    _shaders.lighting.SetMat4("projectionScene", Renderer::GetProjectionMatrix(_depthOfFieldScene));
    _shaders.lighting.SetMat4("projectionWeapon", Renderer::GetProjectionMatrix(_depthOfFieldWeapon));
    _shaders.lighting.SetMat4("inverseProjectionScene", glm::inverse(Renderer::GetProjectionMatrix(_depthOfFieldScene)));
    _shaders.lighting.SetMat4("inverseProjectionWeapon", glm::inverse(Renderer::GetProjectionMatrix(_depthOfFieldWeapon)));
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


	glActiveTexture(GL_TEXTURE22);
	glBindTexture(GL_TEXTURE_CUBE_MAP, Scene::_players[0]._shadowMap._depthTexture);
	glActiveTexture(GL_TEXTURE23);
	glBindTexture(GL_TEXTURE_CUBE_MAP, Scene::_players[1]._shadowMap._depthTexture);

    DrawFullscreenQuad();
}

void DebugPass(Player* player) {

    int playerIndex = GetPlayerIndexFromPlayerPointer(player);
    PlayerRenderTarget& playerRenderTarget = GetPlayerRenderTarget(playerIndex);
    //GBuffer& gBuffer = playerRenderTarget.gBuffer;
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
    _shaders.solidColor.SetMat4("projection", Renderer::GetProjectionMatrix(_depthOfFieldScene));
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
            _cubeMesh.Draw();
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
   

    for (Window& window : Scene::_windows) {

	//	debugPoints.push_back(window.GetFrontLeftCorner());
		//debugPoints.push_back(window.GetBackRightCorner());

		////debugPoints.push_back(window.GetFrontRightCorner());
		//debugPoints.push_back(window.GetBackLeftCorner());
    }


    for (auto& pos : debugPoints) {
        Point point;
        point.pos = pos;
        point.pos.y = 0.0125f;
        point.color = RED;
        Renderer::QueuePointForDrawing(point);
    }
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    RenderImmediate();
    debugPoints.clear();


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

            if (_debugLineRenderMode == DebugLineRenderMode::PHYSX_RAYCAST) {
                color = RED;
                if (shape->getQueryFilterData().word0 == RaycastGroup::RAYCAST_DISABLED) {
                    actor->setActorFlag(PxActorFlag::eVISUALIZATION, false);
                }
            }    
            else if (_debugLineRenderMode == DebugLineRenderMode::PHYSX_COLLISION) {
                color = LIGHT_BLUE;
                if (shape->getQueryFilterData().word1 == CollisionGroup::NO_COLLISION) {
                    actor->setActorFlag(PxActorFlag::eVISUALIZATION, false);
                } 
            }
        }
    }


        // Debug lines

        if (_debugLineRenderMode == DebugLineRenderMode::PHYSX_ALL ||
            _debugLineRenderMode == DebugLineRenderMode::PHYSX_COLLISION ||
            _debugLineRenderMode == DebugLineRenderMode::PHYSX_RAYCAST) {

            _lines.clear();
            _shaders.solidColor.Use();
            _shaders.solidColor.SetBool("uniformColor", false);
            _shaders.solidColor.SetMat4("model", glm::mat4(1));
            auto& renderBuffer = scene->getRenderBuffer();
            for (unsigned int i = 0; i < renderBuffer.getNbLines(); i++) {
                auto pxLine = renderBuffer.getLines()[i];
                Line line;
                line.p1.pos = Util::PxVec3toGlmVec3(pxLine.pos0);
                line.p2.pos = Util::PxVec3toGlmVec3(pxLine.pos1);
                line.p1.color = color;
                line.p2.color = color;
                Renderer::QueueLineForDrawing(line);
            }
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);
            RenderImmediate();
        }
        else if (_debugLineRenderMode == RAYTRACE_LAND) {
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
                    Renderer::QueueTriangleForLineRendering(t);
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
        auto* sphere = AssetManager::GetModel("Sphere");
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

void Renderer::RecreateFrameBuffers(int currentPlayer) {

    float width = (float)_renderWidth;
    float height = (float)_renderHeight;
    int playerCount = Scene::_playerCount;

    // Adjust for splitscreen
    if (_viewportMode == SPLITSCREEN) {
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
    AssetManager::BindMaterialByIndex(AssetManager::GetMaterialIndex("Ceiling"));
    //AssetManager::BindMaterialByIndex(AssetManager::GetMaterialIndex("NumGrid"));
    for (Wall& wall : Scene::_walls) {
        AssetManager::BindMaterialByIndex(wall.materialIndex);
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
	static int ceilingMaterialIndex = AssetManager::GetMaterialIndex("Ceiling");
	static int wallPaperMaterialIndex = AssetManager::GetMaterialIndex("WallPaper");

    // Ceiling trims
   AssetManager::BindMaterialByIndex(AssetManager::GetMaterialIndex("Ceiling"));
    for (Wall& wall : Scene::_walls) {
		for (auto& transform : wall.ceilingTrims) {
			shader.SetMat4("model", transform.to_mat4());
			if (wall.materialIndex == ceilingMaterialIndex) {
				AssetManager::BindMaterialByIndex(ceilingMaterialIndex);
			}
			else if (wall.materialIndex == wallPaperMaterialIndex) {
				AssetManager::BindMaterialByIndex(trimCeilingMaterialIndex);
			}
            AssetManager::GetModel("TrimCeiling")->Draw();
        }
    } 
    // Floor trims
    AssetManager::BindMaterialByIndex(trimFloorMaterialIndex);
    for (Wall& wall : Scene::_walls) {
		for (auto& transform : wall.floorTrims) {
			shader.SetMat4("model", transform.to_mat4());
			if (wall.materialIndex == ceilingMaterialIndex) {
				AssetManager::BindMaterialByIndex(ceilingMaterialIndex);
				AssetManager::GetModel("TrimFloor2")->Draw();
			}
			else if (wall.materialIndex == wallPaperMaterialIndex) {
				AssetManager::BindMaterialByIndex(trimCeilingMaterialIndex);
				AssetManager::GetModel("TrimFloor")->Draw();
			}
        }
    }

    // Render game objects
    for (GameObject& gameObject : Scene::_gameObjects) {

        if (gameObject.collected) {
            continue;
        }

        shader.SetMat4("model", gameObject.GetModelMatrix());
        for (int i = 0; i < gameObject._meshMaterialIndices.size(); i++) {
            AssetManager::BindMaterialByIndex(gameObject._meshMaterialIndices[i]);
            gameObject._model->_meshes[i].Draw();
        }

        if (gameObject.GetName() == "Present") {
			AssetManager::BindMaterialByIndex(AssetManager::GetMaterialIndex("Gold"));
            AssetManager::GetModel("ChristmasBow")->Draw();
        }
    }



    // Render pickups
    for (PickUp& pickup : Scene::_pickUps) {

        if (pickup.pickedUp) {
            continue;
        }

        if (pickup.type == PickUp::Type::GLOCK_AMMO) {

            glm::mat4 parentMatrix = glm::mat4(1);

            if (pickup.parentGameObjectName != "") {
                parentMatrix = Scene::GetGameObjectByName(pickup.parentGameObjectName)->GetModelMatrix();
            }

			shader.SetMat4("model", parentMatrix * pickup.GetModelMatrix());
            
            AssetManager::BindMaterialByIndex(AssetManager::GetMaterialIndex("GlockAmmoBox"));
            AssetManager::GetModel("GlockAmmoBox")->Draw();
        }
    }




    for (Door& door : Scene::_doors) {

        AssetManager::BindMaterialByIndex(AssetManager::GetMaterialIndex("Door"));
        shader.SetMat4("model", door.GetFrameModelMatrix());
        auto* doorFrameModel = AssetManager::GetModel("DoorFrame");
        doorFrameModel->Draw();

        shader.SetMat4("model", door.GetDoorModelMatrix());
        auto* doorModel = AssetManager::GetModel("Door");
        doorModel->Draw();
    }

	for (Window& window : Scene::_windows) {
		shader.SetMat4("model", window.GetModelMatrix());
		AssetManager::BindMaterialByIndex(AssetManager::GetMaterialIndex("Window"));
		AssetManager::GetModel("Window")->_meshes[0].Draw();
		AssetManager::GetModel("Window")->_meshes[1].Draw();
		AssetManager::GetModel("Window")->_meshes[2].Draw();
		AssetManager::GetModel("Window")->_meshes[3].Draw();
		AssetManager::BindMaterialByIndex(AssetManager::GetMaterialIndex("WindowExterior"));
		AssetManager::GetModel("Window")->_meshes[4].Draw();
		AssetManager::GetModel("Window")->_meshes[5].Draw();
		AssetManager::GetModel("Window")->_meshes[6].Draw();
	}

    // This is a hack. Fix this.
    AssetManager::BindMaterialByIndex(AssetManager::GetMaterialIndex("Glock"));
    for (auto& matrix : _glockMatrices) {
        shader.SetMat4("model", matrix);
        AssetManager::GetModel("Glock_Isolated")->Draw();
    }
    for (auto& matrix : _aks74uMatrices) {
        shader.SetMat4("model", matrix);
        AssetManager::GetModel("AKS74U_Isolated")->Draw();
    }


    // Draw physics objects
    /*
    // remove everything below here soon
    PxScene* scene = Physics::GetScene();
    PxU32 nbActors = scene->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC);
    if (nbActors)
    {
        std::vector<PxRigidActor*> actors(nbActors);
        scene->getActors(PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC, reinterpret_cast<PxActor**>(&actors[0]), nbActors);
        
        for (PxRigidActor* actor : actors) {

            if (!actor) {
                std::cout << "you have an null ptr actor mate, and " << nbActors << " total actors\n";
                continue;
            }

            const PxU32 nbShapes = actor->getNbShapes();
            PxShape* shapes[32];
            actor->getShapes(shapes, nbShapes);

            for (PxU32 j = 0; j < nbShapes; j++) {
                const PxGeometry& geom = shapes[j]->getGeometry();

                auto& shape = shapes[j];
                if (shape->getQueryFilterData().word3 == PhysicsObjectType::CUBE ) {
                  //  shape->getQueryFilterData().word3 == PhysicsObjectType::CASING_PROJECTILE) {

                //if (geom.getType() == PxGeometryType::eBOX) {
              
                    //const PxMat44 shapePose(PxShapeExt::getGlobalPose(*shapes[j], *actor));
                    
                    const PxBoxGeometry& boxGeom = static_cast<const PxBoxGeometry&>(geom);
                    Transform localTransform;
                    localTransform.scale = glm::vec3(boxGeom.halfExtents.x, boxGeom.halfExtents.y, boxGeom.halfExtents.z);
                    localTransform.scale *= glm::vec3(2.0f);

                    glm::mat4 model = Util::PxMat44ToGlmMat4(actor->getGlobalPose()) * localTransform.to_mat4();
                    shader.SetMat4("model", model);
                    AssetManager::BindMaterialByIndex(AssetManager::GetMaterialIndex("Ceiling"));
                   // AssetManager::GetModel("Cube").Draw();
                }
            }
        }
    }
    */

/*
    Player& player = Scene::_players[0];
    const glm::vec3& origin = player.GetViewPos();
    const glm::vec3& direction = player.GetCameraForward() * glm::vec3(-1, -1, -1);;
    PhysXRayResult rayResult = Util::CastPhysXRay(origin, direction, 250);
       
    static int count = 0;

    // Check for any hits
    if (rayResult.hitFound) {
        count++;
        //std::cout << count << " you hit a cube bro\n";

        if (Input::LeftMousePressed()) {
            PxVec3 force = PxVec3(direction.x, direction.y, direction.z) * 100;
            PxRigidDynamic* actor = (PxRigidDynamic*)rayResult.hitActor;
            actor->addForce(force);
        }
    }*/


    shader.SetBool("outputEmissive", true);
	// Light bulbs
	for (Light& light : Scene::_lights) {
		Transform transform;
		transform.position = light.position;
		shader.SetVec3("lightColor", light.color);
		shader.SetMat4("model", transform.to_mat4());
		AssetManager::BindMaterialByIndex(AssetManager::GetMaterialIndex("Light"));
		AssetManager::GetModel("Light1")->Draw();
	}
	shader.SetBool("outputEmissive", false);

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
    if (animatedGameObject->GetName() == "Glock") {
        glm::vec3 barrelPosition = animatedGameObject->GetGlockCasingSpawnPostion();
        Point point = Point(barrelPosition, BLUE);
        Renderer::QueuePointForDrawing(point);
        //debugPoints.push_back(barrelPosition);
    }*/


    for (int i = 0; i < animatedGameObject->_animatedTransforms.worldspace.size(); i++) {
             
        auto& bone = animatedGameObject->_animatedTransforms.worldspace[i];
        glm::mat4 m = animatedGameObject->GetModelMatrix() * bone;
        float x = m[3][0];
        float y = m[3][1];
        float z = m[3][2];
        Point p2(glm::vec3(x, y, z), RED);
        Renderer::QueuePointForDrawing(p2);

        /*
        if (animatedGameObject->_animatedTransforms.names[i] == "Glock") {
            //    float x = m[3][0];
            //    float y = m[3][1];
            //    float z = m[3][2];
             //   debugPoints.push_back(glm::vec3(x, y, z));
        }

        if (animatedGameObject->GetName() == "Shotgun") {
            glm::vec3 barrelPosition = animatedGameObject->GetShotgunBarrelPostion();
            Point point = Point(barrelPosition, BLUE);
            //   QueuePointForDrawing(point);
        }*/
    }

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
				skinnedModel.m_meshEntries[i].Name == "SK_FPSArms_Female" ||
				skinnedModel.m_meshEntries[i].Name == "SK_FPSArms_Female_LOD0.001") {				
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

void DrawAnimatedScene(Shader& shader, Player* player) {

    // This is a temporary hack so multiple animated game objects can have glocks which are queued to be rendered later by DrawScene()
    _glockMatrices.clear();
    _aks74uMatrices.clear();

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
    }

    shader.Use();
    shader.SetBool("isAnimated", true);
    shader.SetMat4("model", glm::mat4(1)); 

    // Render other players
    for (Player& otherPlayer : Scene::_players) {
        if (&otherPlayer != player) {
            DrawAnimatedObject(shader, &otherPlayer._characterModel);            
        }
    }

    shader.SetMat4("model", glm::mat4(1));
    shader.SetFloat("projectionMatrixIndex", 0.0f);
    for (auto& animatedObject : Scene::GetAnimatedGameObjects()) {
        DrawAnimatedObject(shader, &animatedObject);
    }

    glDisable(GL_CULL_FACE);
    shader.SetFloat("projectionMatrixIndex", 1.0f);
    shader.SetMat4("projection", Renderer::GetProjectionMatrix(_depthOfFieldWeapon)); // 1.0 for weapon, 0.9 for scene.
    DrawAnimatedObject(shader, &player->GetFirstPersonWeapon());
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

        if (gameObject.collected) {
            continue;
        }

        shader.SetMat4("model", gameObject.GetModelMatrix());
        for (int i = 0; i < gameObject._meshMaterialIndices.size(); i++) {
            gameObject._model->_meshes[i].Draw();
        }
    }

    for (Door& door : Scene::_doors) {
        shader.SetMat4("model", door.GetFrameModelMatrix());
        auto* doorFrameModel = AssetManager::GetModel("DoorFrame");
        doorFrameModel->Draw();
        shader.SetMat4("model", door.GetDoorModelMatrix());
        auto* doorModel = AssetManager::GetModel("Door");
        doorModel->Draw();
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
    _shaders.bulletDecals.Load("bullet_decals.vert", "bullet_decals.frag");
    _shaders.geometry_instanced.Load("geometry_instanced.vert", "geometry_instanced.frag");
	_shaders.glass.Load("glass.vert", "glass.frag");
	_shaders.glassComposite.Load("glass_composite.vert", "glass_composite.frag");
	_shaders.skybox.Load("skybox.vert", "skybox.frag");

    _shaders.compute.Load("res/shaders/compute.comp");
    _shaders.pointCloud.Load("res/shaders/point_cloud.comp");
	_shaders.propogateLight.Load("res/shaders/propogate_light.comp");
	_shaders.computeTest.Load("res/shaders/test.comp");
}

void Renderer::RenderEditorFrame() {

    PlayerRenderTarget playerRenderTarget = GetPlayerRenderTarget(0);
    PresentFrameBuffer presentFrameBuffer = playerRenderTarget.presentFrameBuffer;

    presentFrameBuffer.Bind();

    float renderWidth = (float)presentFrameBuffer.GetWidth();
    float renderHeight = (float)presentFrameBuffer.GetHeight();
    //float screenWidth = GL::GetWindowWidth();
    //float screenHeight = GL::GetWindowHeight();

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
    glBindFramebuffer(GL_FRAMEBUFFER, presentFrameBuffer.GetID());
    glViewport(0, 0, presentFrameBuffer.GetWidth(), presentFrameBuffer.GetHeight());
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    Renderer::RenderUI();

    // Blit image back to frame buffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, presentFrameBuffer.GetID());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, presentFrameBuffer.GetWidth(), presentFrameBuffer.GetHeight(), 0, 0, GL::GetWindowWidth(), GL::GetWindowHeight(), GL_COLOR_BUFFER_BIT, GL_NEAREST);
}


void DrawQuad2(int viewportWidth, int viewPortHeight, int xPos, int yPos, int xSize, int ySize, bool centered) {

	float quadWidth = (float)xSize;
	float quadHeight = (float)ySize;

	//if (centered) {
	//	xPos -= (int)(quadWidth / 2);
	//	yPos -= (int)(quadHeight / 2);
	//

	float renderTargetWidth = (float)viewportWidth;
	float renderTargetHeight = (float)viewPortHeight;
	float width = (1.0f / renderTargetWidth) * quadWidth;
	float height = (1.0f / renderTargetHeight) * quadHeight;
	float ndcX = ((xPos + (quadWidth / 2.0f)) / renderTargetWidth) * 2 - 1;
	float ndcY = ((yPos + (quadHeight / 2.0f)) / renderTargetHeight) * 2 - 1;
	Transform transform;
	//transform.position.x = ndcX;
	//transform.position.y = ndcY * -1;
	//transform.scale = glm::vec3(width, height * -1, 1);

	width = (1.0f / renderTargetWidth) * xSize;
	height = (1.0f / renderTargetHeight) * ySize;
	transform.scale.x = width;
	transform.scale.y = height;

	_shaders.UI.SetMat4("model", transform.to_mat4());
	_quadMesh.Draw();
}

void Renderer::RenderDebugMenu() {


	glBindFramebuffer(GL_FRAMEBUFFER, _menuRenderTarget.fbo);
	glViewport(0, 0, _renderWidth, _renderHeight);
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
	float viewportCenterX = _renderWidth * 0.5f;
	float viewportCenterY = _renderHeight * 0.5f;

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


//	std::cout << menuWidth << "\n";
    	
	// Draw menu background

	_shaders.UI.SetVec3("overrideColor", RED);
	AssetManager::GetTexture("MenuBG")->Bind(0);
	DrawQuad(_renderWidth, _renderHeight, menuWidth, totalMenuHeight, viewportCenterX, viewportCenterY, true);

	_shaders.UI.SetVec3("overrideColor", RED);
	AssetManager::GetTexture("MenuBorderHorizontal")->Bind(0);
	DrawQuad(_renderWidth, _renderHeight, menuWidth, 3, viewportCenterX, viewportCenterY - (totalMenuHeight * 0.5f), true);
	DrawQuad(_renderWidth, _renderHeight, menuWidth, 3, viewportCenterX, viewportCenterY + (totalMenuHeight * 0.5f), true);
	AssetManager::GetTexture("MenuBorderVertical")->Bind(0);
	DrawQuad(_renderWidth, _renderHeight, 3, totalMenuHeight, viewportCenterX - (menuWidth * 0.5f), viewportCenterY, true);
	DrawQuad(_renderWidth, _renderHeight, 3, totalMenuHeight, viewportCenterX + (menuWidth * 0.5f), viewportCenterY, true);

	AssetManager::GetTexture("MenuBorderCornerTL")->Bind(0);
	DrawQuad(_renderWidth, _renderHeight, 3, 3, viewportCenterX - (menuWidth * 0.5f), viewportCenterY - (totalMenuHeight * 0.5f), true);
	AssetManager::GetTexture("MenuBorderCornerTR")->Bind(0);
	DrawQuad(_renderWidth, _renderHeight, 3, 3, viewportCenterX + (menuWidth * 0.5f), viewportCenterY - (totalMenuHeight * 0.5f), true);
	AssetManager::GetTexture("MenuBorderCornerBL")->Bind(0);
	DrawQuad(_renderWidth, _renderHeight, 3, 3, viewportCenterX - (menuWidth * 0.5f), viewportCenterY + (totalMenuHeight * 0.5f), true);
	AssetManager::GetTexture("MenuBorderCornerBR")->Bind(0);
	DrawQuad(_renderWidth, _renderHeight, 3, 3, viewportCenterX + (menuWidth * 0.5f), viewportCenterY + (totalMenuHeight * 0.5f), true);

    // Draw menu text
    TextBlitter::BlitAtPosition(DebugMenu::GetHeading(), viewportCenterX, headingY, true, 1.0f);
    TextBlitter::BlitAtPosition(DebugMenu::GetTextLeft(), textLeftX, subMenuY, false, 1.0f);
	TextBlitter::BlitAtPosition(DebugMenu::GetTextRight(), textRightX, subMenuY, false, 1.0f);

	_shaders.UI.SetVec3("overrideColor", RED);
    TextBlitter::Update(1.0f / 60.0f);
	Renderer::RenderUI();
	_shaders.UI.SetVec3("overrideColor", WHITE);

    // Draw the menu into the main frame buffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, GL::GetWindowWidth(), GL::GetWindowHeight());
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, _menuRenderTarget.texture);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	_shaders.UI.Use();
	_shaders.UI.SetMat4("model", glm::mat4(1));
    DrawFullscreenQuadWithNormals();
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
void Renderer::ToggleDrawingRagdolls() {
	_toggles.drawRagdolls = !_toggles.drawRagdolls;
}
void Renderer::ToggleDebugText() {
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
    _quadMesh.Draw();
}

void Renderer::QueueUIForRendering(std::string textureName, int screenX, int screenY, bool centered, glm::vec3 color) {
    UIRenderInfo info;
    info.textureName = textureName;
    info.screenX = screenX;
	info.screenY = screenY;
	info.centered = centered;
	info.color = color;
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

    float viewportWidth = (float)_renderWidth;
    float viewportHeight = (float)_renderHeight;
    if (_viewportMode == SPLITSCREEN) {
        viewportHeight *= 0.5f;
    }

    for (UIRenderInfo& uiRenderInfo : _UIRenderInfos) {
        AssetManager::GetTexture(uiRenderInfo.textureName)->Bind(0);
        Texture* texture = AssetManager::GetTexture(uiRenderInfo.textureName);
        _shaders.UI.SetVec3("color", uiRenderInfo.color);
        DrawQuad(viewportWidth, viewportHeight, texture->GetWidth(), texture->GetHeight(), uiRenderInfo.screenX, uiRenderInfo.screenY, uiRenderInfo.centered);
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

void Renderer::NextDebugLineRenderMode() {
    _debugLineRenderMode = (DebugLineRenderMode)(int(_debugLineRenderMode) + 1);
    if (_debugLineRenderMode == DEBUG_LINE_MODE_COUNT)
        _debugLineRenderMode = (DebugLineRenderMode)0;
}

glm::mat4 Renderer::GetProjectionMatrix(float depthOfField) {

    float width = (float)GL::GetWindowWidth();
    float height = (float)GL::GetWindowHeight();

    if (_viewportMode == SPLITSCREEN) {
        height *= 0.5f;
    }

    return glm::perspective(depthOfField, width / height, NEAR_PLANE, FAR_PLANE);
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
    //PresentFrameBuffer& presentFrameBuffer = playerRenderTarget.presentFrameBuffer;

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
        worldPosition = player->GetFirstPersonWeapon().GetGlockBarrelPostion();
    }
    else if (player->GetCurrentWeaponIndex() == AKS74U) {
        worldPosition = player->GetFirstPersonWeapon().GetAKS74UBarrelPostion();
    }
    else if (player->GetCurrentWeaponIndex() == SHOTGUN) {
        worldPosition = player->GetFirstPersonWeapon().GetShotgunBarrelPostion();
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
    glm::mat4 projection = Renderer::GetProjectionMatrix(_depthOfFieldWeapon); // 1.0 for weapon, 0.9 for scene.
    glm::mat4 view = player->GetViewMatrix();
    glm::vec3 viewPos = player->GetViewPos();

    _shaders.animatedQuad.Use();
    _shaders.animatedQuad.SetMat4("u_MatrixProjection", projection);
    _shaders.animatedQuad.SetMat4("u_MatrixView", view);
    _shaders.animatedQuad.SetVec3("u_ViewPos", viewPos);

    glActiveTexture(GL_TEXTURE0);
    AssetManager::GetTexture("MuzzleFlash_ALB")->Bind(0);

    muzzleFlash.Draw(&_shaders.animatedQuad, t, player->GetMuzzleFlashRotation());
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
}

void DrawInstanced(Mesh& mesh, std::vector<glm::mat4>& matrices) {
    if (_ssbos.instanceMatrices == 0) {
        glCreateBuffers(1, &_ssbos.instanceMatrices);
        glNamedBufferStorage(_ssbos.instanceMatrices, 4096 * sizeof(glm::mat4), NULL, GL_DYNAMIC_STORAGE_BIT);
    }
    if (matrices.size()) {
        glNamedBufferSubData(_ssbos.instanceMatrices, 0, matrices.size() * sizeof(glm::mat4), &matrices[0]);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, _ssbos.instanceMatrices);
        glBindVertexArray(mesh._VAO);
        glDrawElementsInstanced(GL_TRIANGLES, mesh.GetIndexCount(), GL_UNSIGNED_INT, 0, matrices.size());
    }
}

void DrawBulletDecals(Player* player) {

    unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments);

    glEnable(GL_CULL_FACE);
    
    {
        std::vector<glm::mat4> matrices;
        for (Decal& decal : Scene::_decals) {
            if (decal.type == Decal::Type::REGULAR) {
                matrices.push_back(decal.GetModelMatrix());
            }
        }

        glm::mat4 projection = Renderer::GetProjectionMatrix(_depthOfFieldScene); // 1.0 for weapon, 0.9 for scene.
        glm::mat4 view = player->GetViewMatrix();

        _shaders.bulletDecals.Use();
        _shaders.bulletDecals.SetMat4("projection", projection);
        _shaders.bulletDecals.SetMat4("view", view);

        AssetManager::BindMaterialByIndex(AssetManager::GetMaterialIndex("BulletHole_Plaster"));

        DrawInstanced(AssetManager::GetDecalMesh(), matrices);
    }
	{
		std::vector<glm::mat4> matrices;
		for (Decal& decal : Scene::_decals) {
			if (decal.type == Decal::Type::GLASS) {                
                Transform transform;
                transform.scale = glm::vec3(2.0f);                
                matrices.push_back(decal.GetModelMatrix() * transform.to_mat4());
			}
		}

		glm::mat4 projection = Renderer::GetProjectionMatrix(_depthOfFieldScene); // 1.0 for weapon, 0.9 for scene.
		glm::mat4 view = player->GetViewMatrix();

		_shaders.bulletDecals.Use();
		_shaders.bulletDecals.SetMat4("projection", projection);
		_shaders.bulletDecals.SetMat4("view", view);

		AssetManager::BindMaterialByIndex(AssetManager::GetMaterialIndex("BulletHole_Glass"));

		DrawInstanced(AssetManager::GetDecalMesh(), matrices);
	}
}


void DrawCasingProjectiles(Player* player) {

    glm::mat4 projection = Renderer::GetProjectionMatrix(_depthOfFieldScene); // 1.0 for weapon, 0.9 for scene.
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
    Mesh& glockCasingmesh = AssetManager::GetModel("BulletCasing")->_meshes[0];
    DrawInstanced(glockCasingmesh, matrices);

    // AKS74U
    matrices.clear();
    for (BulletCasing& casing : Scene::_bulletCasings) {
        if (casing.type == AKS74U) {
            matrices.push_back(casing.GetModelMatrix());
        }
    }
    AssetManager::BindMaterialByIndex(AssetManager::GetMaterialIndex("Casing_AkS74U"));
    Mesh& aks75uCasingmesh = AssetManager::GetModel("BulletCasing_AK")->_meshes[0];
    DrawInstanced(aks75uCasingmesh, matrices);



	// DRAW GLASS PROJECTILES 
    // 
    // (currently using the mp7 as a tag, thats fucked, rewrite this soon

	matrices.clear();
	for (BulletCasing& casing : Scene::_bulletCasings) {
		if (casing.type == MP7) {
			matrices.push_back(casing.GetModelMatrix());
		}
	}
	AssetManager::BindMaterialByIndex(AssetManager::GetMaterialIndex("BulletHole_Glass"));
	Mesh& shardMesh = AssetManager::GetModel("GlassShard")->_meshes[0];
	DrawInstanced(shardMesh, matrices);

}

void RenderShadowMaps() {


	// Render player shadowmaps (muzzle flashes)
	_shaders.shadowMap.Use();
	_shaders.shadowMap.SetFloat("far_plane", SHADOW_FAR_PLANE);
	_shaders.shadowMap.SetMat4("model", glm::mat4(1));
	glDepthMask(true);
	glDisable(GL_BLEND);
	glDisable(GL_CULL_FACE);

    for (Player& player : Scene::_players) {

		glEnable(GL_DEPTH_TEST);
		glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
		glBindFramebuffer(GL_FRAMEBUFFER, player._shadowMap._ID);
		glClear(GL_DEPTH_BUFFER_BIT);

		std::vector<glm::mat4> projectionTransforms;
        glm::vec3 position = player.GetViewPos();
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



//    if (!Renderer::_shadowMapsAreDirty)
  //      return;

    _shaders.shadowMap.Use();
    _shaders.shadowMap.SetFloat("far_plane", SHADOW_FAR_PLANE);
    _shaders.shadowMap.SetMat4("model", glm::mat4(1));
    glDepthMask(true);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);

    // clear all, in case there are less lights than the previous frame
    // but really you want to only update lights that have an an dynamic object pass through its radius
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
    _shaders.debugViewPointCloud.SetMat4("projection", Renderer::GetProjectionMatrix(_depthOfFieldScene));
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

    _shaders.compute.Load("res/shaders/compute.comp");
    _shaders.pointCloud.Load("res/shaders/point_cloud.comp");
    _shaders.propogateLight.Load("res/shaders/propogate_light.comp");
    
    Scene::CreatePointCloud();
    Renderer::CreatePointCloudBuffer();
    Renderer::CreateTriangleWorldVertexBuffer();
    
   // std::cout << "Point cloud has " << Scene::_cloudPoints.size() << " points\n";
   // std::cout << "Propagation grid has " << (_mapWidth * _mapHeight * _mapDepth / _propogationGridSpacing) << " cells\n";

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
	//glBindImageTexture(0, gBuffer._gLightingTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
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
