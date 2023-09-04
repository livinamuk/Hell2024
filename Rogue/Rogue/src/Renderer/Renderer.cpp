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
#include "../Core/Audio.h"
#include "../Core/GL.h"
#include "../Core/Input.h"
#include "../Core/Player.h"
#include "../Core/VoxelWorld.h"

#include <vector>
#include <cstdlib>

Shader _testShader;
Shader _solidColorShader;
Shader _shadowMapShader;
Mesh _cubeMesh;
Mesh _cubeMeshZFront;
Mesh _cubeMeshZBack;
Mesh _cubeMeshXFront;
Mesh _cubeMeshXBack;
Mesh _cubeMeshYTop;
Mesh _cubeMeshYBottom;
GBuffer _gBuffer;

Texture _floorBoardsTexture;
Texture _wallpaperTexture;

unsigned int _pointLineVAO;
unsigned int _pointLineVBO;
int _renderWidth = 512 * 2;
int _renderHeight = 288 * 2;
bool _drawLights = false;
bool _drawLines = false;
bool _drawProbes = false;

std::vector<Point> _points;
std::vector<Point> _solidTrianglePoints;
std::vector<Line> _lines;

Texture3D _indirectLightingTexture;

std::vector<ShadowMap> _shadowMaps;

void QueueLineForDrawing(Line line) {
    _lines.push_back(line);
}
void QueuePointForDrawing(Point point) {
    _points.push_back(point);
}

void QueueTriangleForLineRendering(Triangle& triangle) {
    _lines.push_back(Line(triangle.p1, triangle.p2, YELLOW));
    _lines.push_back(Line(triangle.p2, triangle.p3, YELLOW));
    _lines.push_back(Line(triangle.p3, triangle.p1, YELLOW));
}

void QueueTriangleForSolidRendering(Triangle& triangle) {
    _solidTrianglePoints.push_back(Point(triangle.p1, YELLOW));
    _solidTrianglePoints.push_back(Point(triangle.p2, YELLOW));
    _solidTrianglePoints.push_back(Point(triangle.p3, YELLOW));
}

void Renderer::Init() {

    glGenVertexArrays(1, &_pointLineVAO);
    glGenBuffers(1, &_pointLineVBO);
    glPointSize(4);

    _testShader.Load("test.vert", "test.frag");
    _solidColorShader.Load("solid_color.vert", "solid_color.frag");
    _shadowMapShader.Load("shadowmap.vert", "shadowmap.frag", "shadowmap.geom");

    _floorBoardsTexture = Texture("res/textures/floorboards.png");
    _wallpaperTexture = Texture("res/textures/wallpaper.png");
    
    _cubeMesh = MeshUtil::CreateCube(1.0f, 1.0f, true);
    _cubeMeshZFront = MeshUtil::CreateCubeFaceZFront(1.0f);
    _cubeMeshZBack = MeshUtil::CreateCubeFaceZBack(1.0f);
    _cubeMeshXFront = MeshUtil::CreateCubeFaceXFront(1.0f);
    _cubeMeshXBack = MeshUtil::CreateCubeFaceXBack(1.0f);
    _cubeMeshYTop = MeshUtil::CreateCubeFaceYTop(1.0f);
    _cubeMeshYBottom = MeshUtil::CreateCubeFaceYBottom(1.0f);

    _gBuffer.Configure(_renderWidth, _renderHeight);
    _indirectLightingTexture = Texture3D(VoxelWorld::GetPropogationGridWidth(), VoxelWorld::GetPropogationGridHeight(), VoxelWorld::GetPropogationGridDepth());
    //_indirectLightingTexture = Texture3D(3, 3, 2);

    //VoxelWorld::FillIndirectLightingTexture(Renderer::GetIndirectLightingTexture());

    _shadowMaps.push_back(ShadowMap());
    _shadowMaps.push_back(ShadowMap());
    _shadowMaps.push_back(ShadowMap());
    _shadowMaps.push_back(ShadowMap());

    for (ShadowMap& shadowMap : _shadowMaps) {
        shadowMap.Init();
    }
}

void Renderer::RenderFrame() {

    _lines.clear();
    _points.clear();

    _gBuffer.Bind();
    _gBuffer.EnableAllDrawBuffers();










    glViewport(0, 0, _renderWidth, _renderHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)GL::GetWindowWidth() / (float)GL::GetWindowHeight(), 0.1f, 100.0f);

    _testShader.Use();
    _testShader.SetMat4("projection", projection);
    _testShader.SetMat4("view", Player::GetViewMatrix());
    _testShader.SetVec3("viewPos", Player::GetViewPos());

    _indirectLightingTexture.Bind(0);

    if (Input::KeyPressed(HELL_KEY_L)) {
        _drawLights = !_drawLights;
        Audio::PlayAudio("RE_Beep.wav", 0.25f);
    }
    if (Input::KeyPressed(HELL_KEY_B)) {
        _drawLines = !_drawLines;
        Audio::PlayAudio("RE_Beep.wav", 0.25f);
    }
    if (Input::KeyPressed(HELL_KEY_SPACE)) {
        _drawProbes = !_drawProbes;
        Audio::PlayAudio("RE_Beep.wav", 0.25f);
    }

    // Bind shadow maps
    auto lights2 = VoxelWorld::GetLights();
    for (int i = 0; i < lights2.size(); i++) {
        glActiveTexture(GL_TEXTURE1 + i);
        glBindTexture(GL_TEXTURE_CUBE_MAP, _shadowMaps[i]._depthTexture);

        glm::vec3 position = glm::vec3(lights2[i].x, lights2[i].y, lights2[i].z) * VoxelWorld::GetVoxelSize();

        if (i == 0) {
            _testShader.SetVec3("lightPosition[0]", position);
            _testShader.SetVec3("lightColor[0]", lights2[i].color);
            _testShader.SetFloat("lightRadius[0]", lights2[i].radius);
            _testShader.SetFloat("lightStrength[0]", lights2[i].strength);
        }
        if (i == 1) {
            _testShader.SetVec3("lightPosition[1]", position);
            _testShader.SetVec3("lightColor[1]", lights2[i].color);
            _testShader.SetFloat("lightRadius[1]", lights2[i].radius);
            _testShader.SetFloat("lightStrength[1]", lights2[i].strength);
        }
        if (i == 2) {
            _testShader.SetVec3("lightPosition[2]", position);
            _testShader.SetVec3("lightColor[2]", lights2[i].color);
            _testShader.SetFloat("lightRadius[2]", lights2[i].radius);
            _testShader.SetFloat("lightStrength[2]", lights2[i].strength);
        }
        if (i == 3) {
            _testShader.SetVec3("lightPosition[3]", position);
            _testShader.SetVec3("lightColor[3]", lights2[i].color);
            _testShader.SetFloat("lightRadius[3]", lights2[i].radius);
            _testShader.SetFloat("lightStrength[3]", lights2[i].strength);
        }
    }


    // Draw Z Front
    for (VoxelFace& voxel : VoxelWorld::GetZFrontFacingVoxels()) {
        _testShader.SetMat4("model", Util::GetVoxelModelMatrix(voxel, VoxelWorld::GetVoxelSize()));
        _testShader.SetVec3("lightingColor", voxel.accumulatedDirectLighting);
        _testShader.SetVec3("indirectLightingColor", voxel.indirectLighting);
        _testShader.SetVec3("baseColor", voxel.baseColor);
       // _cubeMeshZFront.Draw();
    }
    // Draw Z Back
    for (VoxelFace& voxel : VoxelWorld::GetZBackFacingVoxels()) {
        _testShader.SetMat4("model", Util::GetVoxelModelMatrix(voxel, VoxelWorld::GetVoxelSize()));
        _testShader.SetVec3("lightingColor", voxel.accumulatedDirectLighting);
        _testShader.SetVec3("indirectLightingColor", voxel.indirectLighting);
        _testShader.SetVec3("baseColor", voxel.baseColor);
       // _cubeMeshZBack.Draw();
    }
    // Draw X Front
    for (VoxelFace& voxel : VoxelWorld::GetXFrontFacingVoxels()) {
        _testShader.SetMat4("model", Util::GetVoxelModelMatrix(voxel, VoxelWorld::GetVoxelSize()));
        _testShader.SetVec3("lightingColor", voxel.accumulatedDirectLighting);
        _testShader.SetVec3("indirectLightingColor", voxel.indirectLighting);
        _testShader.SetVec3("baseColor", voxel.baseColor);
        //_cubeMeshXFront.Draw();
    }
    // Draw X Back
    for (VoxelFace& voxel : VoxelWorld::GetXBackFacingVoxels()) {
        _testShader.SetMat4("model", Util::GetVoxelModelMatrix(voxel, VoxelWorld::GetVoxelSize()));
        _testShader.SetVec3("lightingColor", voxel.accumulatedDirectLighting);
        _testShader.SetVec3("indirectLightingColor", voxel.indirectLighting);
        _testShader.SetVec3("baseColor", voxel.baseColor);
        //_cubeMeshXBack.Draw();
    }
    // Draw Y Top
    for (VoxelFace& voxel : VoxelWorld::GetYTopVoxels()) {
        _testShader.SetMat4("model", Util::GetVoxelModelMatrix(voxel, VoxelWorld::GetVoxelSize()));
        _testShader.SetVec3("lightingColor", voxel.accumulatedDirectLighting);
        _testShader.SetVec3("indirectLightingColor", voxel.indirectLighting);
        _testShader.SetVec3("baseColor", voxel.baseColor);
        //_cubeMeshYTop.Draw();
    }
    // Draw Y Bottom
    for (VoxelFace& voxel : VoxelWorld::GetYBottomVoxels()) {
        _testShader.SetMat4("model", Util::GetVoxelModelMatrix(voxel, VoxelWorld::GetVoxelSize()));
        _testShader.SetVec3("lightingColor", voxel.accumulatedDirectLighting);
        _testShader.SetVec3("indirectLightingColor", voxel.indirectLighting);
        _testShader.SetVec3("baseColor", voxel.baseColor);
        _cubeMeshYBottom.Draw();
    }




    // Draw lines
    _solidTrianglePoints.clear();
    for (Triangle& tri : VoxelWorld::GetTriangleOcculdersYUp()) {
        _solidTrianglePoints.push_back(Point(tri.p1, WHITE));
        _solidTrianglePoints.push_back(Point(tri.p2, WHITE));
        _solidTrianglePoints.push_back(Point(tri.p3, WHITE));
    }
    glBindVertexArray(_pointLineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, _pointLineVBO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)offsetof(Point, color));
    glBufferData(GL_ARRAY_BUFFER, _solidTrianglePoints.size() * sizeof(Point), _solidTrianglePoints.data(), GL_STATIC_DRAW);
    _testShader.SetInt("tex_flag", 1);
    _testShader.SetMat4("model", glm::mat4(1));

    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, _floorBoardsTexture.GetID());
    glDrawArrays(GL_TRIANGLES, 0, _solidTrianglePoints.size());


    _solidTrianglePoints.clear();
    for (Triangle& tri : VoxelWorld::GetTriangleOcculdersXFacing()) {
        _solidTrianglePoints.push_back(Point(tri.p1, WHITE));
        _solidTrianglePoints.push_back(Point(tri.p2, WHITE));
        _solidTrianglePoints.push_back(Point(tri.p3, WHITE));
    }
    glBindVertexArray(_pointLineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, _pointLineVBO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)offsetof(Point, color));
    glBufferData(GL_ARRAY_BUFFER, _solidTrianglePoints.size() * sizeof(Point), _solidTrianglePoints.data(), GL_STATIC_DRAW);
    _testShader.SetInt("tex_flag", 2);
    _testShader.SetMat4("model", glm::mat4(1));

    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, _wallpaperTexture.GetID());
    glDrawArrays(GL_TRIANGLES, 0, _solidTrianglePoints.size());

    _solidTrianglePoints.clear();
    for (Triangle& tri : VoxelWorld::GetTriangleOcculdersZFacing()) {
        _solidTrianglePoints.push_back(Point(tri.p1, WHITE));
        _solidTrianglePoints.push_back(Point(tri.p2, WHITE));
        _solidTrianglePoints.push_back(Point(tri.p3, WHITE));
    }
    glBindVertexArray(_pointLineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, _pointLineVBO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)offsetof(Point, color));
    glBufferData(GL_ARRAY_BUFFER, _solidTrianglePoints.size() * sizeof(Point), _solidTrianglePoints.data(), GL_STATIC_DRAW);
    _testShader.SetInt("tex_flag", 3);
    _testShader.SetMat4("model", glm::mat4(1));

    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, _wallpaperTexture.GetID());
    glDrawArrays(GL_TRIANGLES, 0, _solidTrianglePoints.size());


    _solidTrianglePoints.clear();
    _testShader.SetInt("tex_flag", 0);


    _testShader.SetVec3("color", WHITE);

    _solidColorShader.Use();
    _solidColorShader.SetMat4("projection", projection);
    _solidColorShader.SetMat4("view", Player::GetViewMatrix());
    _solidColorShader.SetVec3("viewPos", Player::GetViewPos());
    _solidColorShader.SetVec3("color", glm::vec3(1, 1, 0));
    _solidColorShader.SetMat4("model", glm::mat4(1));

    if (_drawLines) {
        for (Triangle& tri : VoxelWorld::GetAllTriangleOcculders()) {
            QueueTriangleForLineRendering(tri);
        }
    }

    //_drawLines = true;
    for (Line& line : VoxelWorld::GetTestRays()) {
        QueueLineForDrawing(line);
    }


    // glm::vec3 worldPosTest = glm::vec3(3.1f, 1, 2.1f);
    // QueuePointForDrawing(Point(worldPosTest, YELLOW));

     // Draw lights
    if (_drawLights) {
        for (Light& light : VoxelWorld::GetLights()) {
            glm::vec3 lightCenter = glm::vec3(light.x, light.y, light.z) * VoxelWorld::GetVoxelSize();
            Transform lightTransform;
            lightTransform.scale = glm::vec3(VoxelWorld::GetVoxelSize());
            lightTransform.position = lightCenter;
            _solidColorShader.SetMat4("model", lightTransform.to_mat4());
            _solidColorShader.SetVec3("color", WHITE);
            _solidColorShader.SetBool("uniformColor", true);
            _cubeMesh.Draw();
        }
    }

    if (_drawProbes) {
        // Draw propogated light values
        for (int x = 0; x < VoxelWorld::GetPropogationGridWidth(); x++) {
            for (int y = 0; y < VoxelWorld::GetPropogationGridHeight(); y++) {
                for (int z = 0; z < VoxelWorld::GetPropogationGridDepth(); z++) {

                    GridProbe& probe = VoxelWorld::GetProbeByGridIndex(x, y, z);
                    if (probe.color == BLACK || probe.ignore)
                        continue;

                    // draw
                    Transform t;
                    t.scale = glm::vec3(0.05f);
                    t.position = probe.worldPositon;
                    _solidColorShader.SetMat4("model", t.to_mat4());
                    _solidColorShader.SetVec3("color", probe.color / (float)probe.samplesRecieved);
                    //_solidColorShader.SetVec3("color", probe.color);
                    _solidColorShader.SetBool("uniformColor", true);
                    _cubeMesh.Draw();

                }
            }
        }
    }

    // Prep for points/lines
    _solidColorShader.SetMat4("model", glm::mat4(1));
    _solidColorShader.SetBool("uniformColor", false);
    glDisable(GL_DEPTH_TEST);

    // Draw lines
    glBindVertexArray(_pointLineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, _pointLineVBO);
    glBufferData(GL_ARRAY_BUFFER, _lines.size() * sizeof(Line), _lines.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)offsetof(Point, color));
    glBindVertexArray(_pointLineVAO);
    if (_drawLines)
        glDrawArrays(GL_LINES, 0, 2 * _lines.size());
    // Draw points
    glBufferData(GL_ARRAY_BUFFER, _points.size() * sizeof(Point), _points.data(), GL_STATIC_DRAW);
    glBindVertexArray(_pointLineVAO);
    glDrawArrays(GL_POINTS, 0, _points.size());
    // Draw triangles
    glBufferData(GL_ARRAY_BUFFER, _solidTrianglePoints.size() * sizeof(Point), _solidTrianglePoints.data(), GL_STATIC_DRAW);
    glBindVertexArray(_pointLineVAO);
    glDrawArrays(GL_TRIANGLES, 0, _solidTrianglePoints.size());



    // Render shadowmaps for next frame

    _solidTrianglePoints.clear();
    for (Triangle& tri : VoxelWorld::GetAllTriangleOcculders()) {
        _solidTrianglePoints.push_back(Point(tri.p1, YELLOW));
        _solidTrianglePoints.push_back(Point(tri.p2, YELLOW));
        _solidTrianglePoints.push_back(Point(tri.p3, YELLOW));
    }
    glBufferData(GL_ARRAY_BUFFER, _solidTrianglePoints.size() * sizeof(Point), _solidTrianglePoints.data(), GL_STATIC_DRAW);
    glBindVertexArray(_pointLineVAO);

    _shadowMapShader.Use();
    _shadowMapShader.SetFloat("far_plane", SHADOW_FAR_PLANE);
    _shadowMapShader.SetMat4("model", glm::mat4(1));

    glDepthMask(true);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    // glEnable(GL_CULL_FACE);
    // glCullFace(GL_FRONT);

    auto lights = VoxelWorld::GetLights();

    for (int i = 0; i < lights.size(); i++) {
        glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
        glBindFramebuffer(GL_FRAMEBUFFER, _shadowMaps[i]._ID);
        glEnable(GL_DEPTH_TEST);
        glClear(GL_DEPTH_BUFFER_BIT);
        std::vector<glm::mat4> projectionTransforms;
        glm::vec3 position = glm::vec3(lights[i].x, lights[i].y, lights[i].z) * VoxelWorld::GetVoxelSize();
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
        glDrawArrays(GL_TRIANGLES, 0, _solidTrianglePoints.size());
    }

    _solidTrianglePoints.clear();






    // Blit image back to frame buffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, _gBuffer.GetID());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, _gBuffer.GetWidth(), _gBuffer.GetHeight(), 0, 0, GL::GetWindowWidth(), GL::GetWindowHeight(), GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

void Renderer::HotloadShaders() {
    std::cout << "Hotloaded shaders\n";
    _testShader.Load("test.vert", "test.frag");
    _solidColorShader.Load("solid_color.vert", "solid_color.frag");
}

Texture3D& Renderer::GetIndirectLightingTexture() {
    return _indirectLightingTexture;
}
