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

Shader _testShader;
Shader _solidColorShader;
Shader _shadowMapShader;
Shader _UIShader;
Shader _editorSolidColorShader;

Mesh _cubeMesh;

Model _keyModel;
Model _sphereModel;

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

struct Toggles {
    bool drawLights = false;
    bool drawProbes = false;
    bool drawLines = false;
} _toggles;

enum RenderMode { COMPOSITE = 0, POINT_CLOUD, DIRECT_LIGHT, INDIRECT_LIGHT, MODE_COUNT} _mode;

void QueueLineForDrawing(Line line);
void QueuePointForDrawing(Point point);
void QueueTriangleForLineRendering(Triangle& triangle);
void QueueTriangleForSolidRendering(Triangle& triangle);
void DrawScene(Shader& shader);

int _voxelTextureWidth = { 0 };
int _voxelTextureHeight = { 0 };
int _voxelTextureDepth = { 0 };
float _voxelSize2 = 0.2f;

GLuint _imageStoreTexture = { 0 };

void Renderer::Init() {

    Scene::Init();

    glGenVertexArrays(1, &_pointLineVAO);
    glGenBuffers(1, &_pointLineVBO);
    glPointSize(4);

    _keyModel.Load("res/models/SmallKey.obj");
    _sphereModel.Load("res/models/Sphere.obj");

    _testShader.Load("test.vert", "test.frag");
    _solidColorShader.Load("solid_color.vert", "solid_color.frag");
    _shadowMapShader.Load("shadowmap.vert", "shadowmap.frag", "shadowmap.geom");    
    _UIShader.Load("ui.vert", "ui.frag");
    _editorSolidColorShader.Load("editor_solid_color.vert", "editor_solid_color.frag");

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

    _gBuffer.Bind();
    _gBuffer.EnableAllDrawBuffers();

    glViewport(0, 0, _gBuffer.GetWidth(), _gBuffer.GetHeight());
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)GL::GetWindowWidth() / (float)GL::GetWindowHeight(), 0.1f, 100.0f);
    glm::mat4 view = Player::GetViewMatrix();

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

    _testShader.SetMat4("model", glm::mat4(1));
    DrawScene(_testShader);

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
            
    glEnable(GL_DEPTH_TEST);

    _solidColorShader.SetMat4("model", glm::mat4(1));
    _solidColorShader.SetBool("uniformColor", false);
    RenderImmediate();

    glDisable(GL_DEPTH_TEST);
 
    if (_toggles.drawLines) {
        for (Line& line : Scene::_worldLines) {
            QueueLineForDrawing(line);
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

    RayCastResult cameraRayData;
    glm::vec3 rayOrigin = Player::GetViewPos();
    glm::vec3 rayDirection = Player::GetCameraFront() * glm::vec3(-1);
    Util::EvaluateRaycasts(rayOrigin, rayDirection, 9999, Scene::_triangleWorld, RaycastObjectType::FLOOR, glm::mat4(1), cameraRayData);
   // if (cameraRayData.found)
   //     QueueTriangleForSolidRendering(cameraRayData.triangle);

  //  std::cout << cameraRayData.rayCount << "\n";

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
    }

    // Blit image back to a smaller FBO
    glBindFramebuffer(GL_READ_FRAMEBUFFER, _gBuffer.GetID());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _gBufferFinalSize.GetID());
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
