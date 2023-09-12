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
#include "../Core/VoxelWorld.h"
#include "../Core/AssetManager.h"
#include "../Core/TextBlitter.h"
#include "../Core/Editor.h"

#include <vector>
#include <cstdlib>
#include <format>

Shader _testShader;
Shader _solidColorShader;
Shader _shadowMapShader;
Shader _UIShader;
Shader _editorSolidColorShader;
Mesh _cubeMesh;
Mesh _cubeMeshZFront;
Mesh _cubeMeshZBack;
Mesh _cubeMeshXFront;
Mesh _cubeMeshXBack;
Mesh _cubeMeshYTop;
Mesh _cubeMeshYBottom;
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
    bool renderAsVoxelDirectLighting = true;
} _toggles;

enum RenderMode { FINAL = 0, VOXEL_WORLD, MODE_COUNT} _mode;

void QueueLineForDrawing(Line line);
void QueuePointForDrawing(Point point);
void QueueTriangleForLineRendering(Triangle& triangle);
void QueueTriangleForSolidRendering(Triangle& triangle);

void Renderer::Init() {

    glGenVertexArrays(1, &_pointLineVAO);
    glGenBuffers(1, &_pointLineVBO);
    glPointSize(4);

    _testShader.Load("test.vert", "test.frag");
    _solidColorShader.Load("solid_color.vert", "solid_color.frag");
    _shadowMapShader.Load("shadowmap.vert", "shadowmap.frag", "shadowmap.geom");    
    _UIShader.Load("ui.vert", "ui.frag");
    _editorSolidColorShader.Load("editor_solid_color.vert", "editor_solid_color.frag");

    _floorBoardsTexture = Texture("res/textures/floorboards.png");
    _wallpaperTexture = Texture("res/textures/wallpaper.png");
    _plasterTexture = Texture("res/textures/plaster.png");
    
    _cubeMesh = MeshUtil::CreateCube(1.0f, 1.0f, true);
    _cubeMeshZFront = MeshUtil::CreateCubeFaceZFront(1.0f);
    _cubeMeshZBack = MeshUtil::CreateCubeFaceZBack(1.0f);
    _cubeMeshXFront = MeshUtil::CreateCubeFaceXFront(1.0f);
    _cubeMeshXBack = MeshUtil::CreateCubeFaceXBack(1.0f);
    _cubeMeshYTop = MeshUtil::CreateCubeFaceYTop(1.0f);
    _cubeMeshYBottom = MeshUtil::CreateCubeFaceYBottom(1.0f);

    _gBuffer.Configure(_renderWidth, _renderHeight);
    _gBufferFinalSize.Configure(_renderWidth / 2, _renderHeight / 2);
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

void RenderImmediate() {

    // Draw lines
    glBindVertexArray(_pointLineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, _pointLineVBO);
    glBufferData(GL_ARRAY_BUFFER, _lines.size() * sizeof(Line), _lines.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)offsetof(Point, color));
    glBindVertexArray(_pointLineVAO);
    glDrawArrays(GL_LINES, 0, 2 * _lines.size());

    // Draw points
    glBufferData(GL_ARRAY_BUFFER, _points.size() * sizeof(Point), _points.data(), GL_STATIC_DRAW);
    glBindVertexArray(_pointLineVAO);
    glDrawArrays(GL_POINTS, 0, _points.size());

    // Draw triangles
    glBufferData(GL_ARRAY_BUFFER, _solidTrianglePoints.size() * sizeof(Point), _solidTrianglePoints.data(), GL_STATIC_DRAW);
    glBindVertexArray(_pointLineVAO);
    glDrawArrays(GL_TRIANGLES, 0, _solidTrianglePoints.size());

    // Cleanup
    _lines.clear();
    _points.clear();
    _solidTrianglePoints.clear();
}

void Renderer::RenderFrame() {

    _gBuffer.Bind();
    _gBuffer.EnableAllDrawBuffers();

    glViewport(0, 0, _renderWidth, _renderHeight);
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

    _indirectLightingTexture.Bind(0);

    // Bind shadow maps
    for (int i = 0; i < 4; i++) {
        _testShader.SetVec3("lightColor[" + std::to_string(i) + "]", glm::vec3(0));
    }

    auto lights2 = VoxelWorld::GetLights();
    for (int i = 0; i < lights2.size(); i++) {
        glActiveTexture(GL_TEXTURE1 + i);
        glBindTexture(GL_TEXTURE_CUBE_MAP, _shadowMaps[i]._depthTexture);

        glm::vec3 position = glm::vec3(lights2[i].x, lights2[i].y, lights2[i].z) * VoxelWorld::GetVoxelSize();
        _testShader.SetVec3("lightPosition[" + std::to_string(i) + "]", position);
        _testShader.SetVec3("lightColor[" + std::to_string(i) + "]", lights2[i].color);
        _testShader.SetFloat("lightRadius[" + std::to_string(i) + "]", lights2[i].radius);
        _testShader.SetFloat("lightStrength[" + std::to_string(i) + "]", lights2[i].strength);
    }




    // Draw Z Front
    for (VoxelFace& voxel : VoxelWorld::GetZFrontFacingVoxels()) {
        _testShader.SetMat4("model", Util::GetVoxelModelMatrix(voxel, VoxelWorld::GetVoxelSize()));
        _testShader.SetVec3("lightingColor", voxel.accumulatedDirectLighting);
        _testShader.SetVec3("indirectLightingColor", voxel.indirectLighting);
        _testShader.SetVec3("baseColor", voxel.baseColor);
     //   if (_toggles.renderAsVoxelDirectLighting)
     //       _cubeMeshZFront.Draw();
    }
    // Draw Z Back
    for (VoxelFace& voxel : VoxelWorld::GetZBackFacingVoxels()) {
        _testShader.SetMat4("model", Util::GetVoxelModelMatrix(voxel, VoxelWorld::GetVoxelSize()));
        _testShader.SetVec3("lightingColor", voxel.accumulatedDirectLighting);
        _testShader.SetVec3("indirectLightingColor", voxel.indirectLighting);
        _testShader.SetVec3("baseColor", voxel.baseColor);
      //  if (_toggles.renderAsVoxelDirectLighting)
     //       _cubeMeshZBack.Draw();
    }
    // Draw X Front
    for (VoxelFace& voxel : VoxelWorld::GetXFrontFacingVoxels()) {
        _testShader.SetMat4("model", Util::GetVoxelModelMatrix(voxel, VoxelWorld::GetVoxelSize()));
        _testShader.SetVec3("lightingColor", voxel.accumulatedDirectLighting);
        _testShader.SetVec3("indirectLightingColor", voxel.indirectLighting);
        _testShader.SetVec3("baseColor", voxel.baseColor);
        //if (_toggles.renderAsVoxelDirectLighting)
        //    _cubeMeshXFront.Draw();
    }
    // Draw X Back
    for (VoxelFace& voxel : VoxelWorld::GetXBackFacingVoxels()) {
        _testShader.SetMat4("model", Util::GetVoxelModelMatrix(voxel, VoxelWorld::GetVoxelSize()));
        _testShader.SetVec3("lightingColor", voxel.accumulatedDirectLighting);
        _testShader.SetVec3("indirectLightingColor", voxel.indirectLighting);
        _testShader.SetVec3("baseColor", voxel.baseColor);
       // if (_toggles.renderAsVoxelDirectLighting)
     //       _cubeMeshXBack.Draw();
    }
    // Draw Y Top
    for (VoxelFace& voxel : VoxelWorld::GetYTopVoxels()) {
        _testShader.SetMat4("model", Util::GetVoxelModelMatrix(voxel, VoxelWorld::GetVoxelSize()));
        _testShader.SetVec3("lightingColor", voxel.accumulatedDirectLighting);
        _testShader.SetVec3("indirectLightingColor", voxel.indirectLighting);
        _testShader.SetVec3("baseColor", voxel.baseColor);
      //  if (_toggles.renderAsVoxelDirectLighting)
       //     _cubeMeshYTop.Draw();
    }
    // Draw Y Bottom
    for (VoxelFace& voxel : VoxelWorld::GetYBottomVoxels()) {
        _testShader.SetMat4("model", Util::GetVoxelModelMatrix(voxel, VoxelWorld::GetVoxelSize()));
        _testShader.SetVec3("lightingColor", voxel.accumulatedDirectLighting);
        _testShader.SetVec3("indirectLightingColor", voxel.indirectLighting);
        _testShader.SetVec3("baseColor", voxel.baseColor);
      //  _cubeMeshYBottom.Draw();
    }




    for (VoxelFace& voxel : VoxelWorld::GetXFrontFacingVoxels()) {
        _testShader.SetMat4("model", Util::GetVoxelModelMatrix(voxel, VoxelWorld::GetVoxelSize()));
        _testShader.SetVec3("lightingColor", voxel.accumulatedDirectLighting);
        _testShader.SetVec3("indirectLightingColor", voxel.indirectLighting);
        _testShader.SetVec3("baseColor", voxel.baseColor);
        //if (_toggles.renderAsVoxelDirectLighting)
        //    _cubeMeshXFront.Draw();
    }

    _testShader.SetVec3("camForward", Player::GetCameraFront());

    for (int x = 0; x < MAP_WIDTH; x++) {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int z = 0; z < MAP_DEPTH; z++) {

                _testShader.SetInt("tex_flag", 0);

                if (VoxelWorld::CellIsSolid(x, y, z) && VoxelWorld::CellIsEmpty(x + 1, y, z)) {
                    _testShader.SetMat4("model", Util::GetVoxelModelMatrix(x, y, z, VoxelWorld::GetVoxelSize()));
                    _testShader.SetVec3("lightingColor", VoxelWorld::GetVoxel(x, y, z).forwardFaceX.accumulatedDirectLighting);
                    _testShader.SetInt("tex_flag", 2);
                    glActiveTexture(GL_TEXTURE5);
                    glBindTexture(GL_TEXTURE_2D, _wallpaperTexture.GetID());
                    _cubeMeshXFront.Draw();
                }
                if (VoxelWorld::CellIsSolid(x, y, z) && VoxelWorld::CellIsEmpty(x - 1, y, z)) {
                    _testShader.SetMat4("model", Util::GetVoxelModelMatrix(x, y, z, VoxelWorld::GetVoxelSize()));
                    _testShader.SetVec3("lightingColor", VoxelWorld::GetVoxel(x, y, z).backFaceX.accumulatedDirectLighting);
                    _testShader.SetInt("tex_flag", 2);
                    glActiveTexture(GL_TEXTURE5);
                    glBindTexture(GL_TEXTURE_2D, _wallpaperTexture.GetID());  
                    _cubeMeshXBack.Draw();
                }
                if (VoxelWorld::CellIsSolid(x, y, z) && VoxelWorld::CellIsEmpty(x , y, z + 1)) {
                    _testShader.SetMat4("model", Util::GetVoxelModelMatrix(x, y, z, VoxelWorld::GetVoxelSize()));
                    _testShader.SetVec3("lightingColor", VoxelWorld::GetVoxel(x, y, z).forwardFaceZ.accumulatedDirectLighting);
                    _testShader.SetInt("tex_flag", 3);
                    glActiveTexture(GL_TEXTURE5);
                    glBindTexture(GL_TEXTURE_2D, _wallpaperTexture.GetID());
                    _cubeMeshZFront.Draw();
                }
                if (VoxelWorld::CellIsSolid(x, y, z) && VoxelWorld::CellIsEmpty(x , y, z - 1)) {
                    _testShader.SetMat4("model", Util::GetVoxelModelMatrix(x, y, z, VoxelWorld::GetVoxelSize()));
                    _testShader.SetVec3("lightingColor", VoxelWorld::GetVoxel(x, y, z).backFaceZ.accumulatedDirectLighting);
                    _testShader.SetInt("tex_flag", 3);
                    glActiveTexture(GL_TEXTURE5);
                    glBindTexture(GL_TEXTURE_2D, _wallpaperTexture.GetID());
                    _cubeMeshZBack.Draw();
                }

                if (VoxelWorld::CellIsSolid(x, y, z) && VoxelWorld::CellIsEmpty(x, y + 1, z )) {
                    _testShader.SetMat4("model", Util::GetVoxelModelMatrix(x, y, z, VoxelWorld::GetVoxelSize()));
                    _testShader.SetVec3("lightingColor", VoxelWorld::GetVoxel(x, y, z).YUpFace.accumulatedDirectLighting);
                   
                    _testShader.SetInt("tex_flag", 1);
                    glActiveTexture(GL_TEXTURE5);

                    if (y == 0) {
                        glBindTexture(GL_TEXTURE_2D, _floorBoardsTexture.GetID());
                    } else
                        glBindTexture(GL_TEXTURE_2D, _wallpaperTexture.GetID());

                    _cubeMeshYTop.Draw();
                }

                if (VoxelWorld::CellIsSolid(x, y, z) && VoxelWorld::CellIsEmpty(x, y - 1, z )) {
                    _testShader.SetMat4("model", Util::GetVoxelModelMatrix(x, y, z, VoxelWorld::GetVoxelSize()));
                    _testShader.SetVec3("lightingColor", VoxelWorld::GetVoxel(x, y, z).YDownFace.accumulatedDirectLighting);

                    _testShader.SetInt("tex_flag", 1);
                    glActiveTexture(GL_TEXTURE5);
                    glBindTexture(GL_TEXTURE_2D, _plasterTexture.GetID());

                    _cubeMeshYBottom.Draw();
                }
            }
        }
    }




    if (!_toggles.renderAsVoxelDirectLighting) {
        _solidTrianglePoints.clear();
        for (Triangle& tri : VoxelWorld::GetTriangleOcculdersYUp()) {
            _solidTrianglePoints.push_back(Point(tri.p1, tri.color));
            _solidTrianglePoints.push_back(Point(tri.p2, tri.color));
            _solidTrianglePoints.push_back(Point(tri.p3, tri.color));
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
            _solidTrianglePoints.push_back(Point(tri.p1, tri.color));
            _solidTrianglePoints.push_back(Point(tri.p2, tri.color));
            _solidTrianglePoints.push_back(Point(tri.p3, tri.color));
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
            _solidTrianglePoints.push_back(Point(tri.p1, tri.color));
            _solidTrianglePoints.push_back(Point(tri.p2, tri.color));
            _solidTrianglePoints.push_back(Point(tri.p3, tri.color));
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
        _testShader.SetVec3("camForward", Player::GetCameraFront());

        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, _wallpaperTexture.GetID());
        glDrawArrays(GL_TRIANGLES, 0, _solidTrianglePoints.size());

        _solidTrianglePoints.clear();
        _testShader.SetInt("tex_flag", 0);
    }

    
    
    glDisable(GL_DEPTH_TEST);

    _solidColorShader.Use();
    _solidColorShader.SetMat4("projection", projection);
    _solidColorShader.SetMat4("view", Player::GetViewMatrix());
    _solidColorShader.SetVec3("viewPos", Player::GetViewPos());
    _solidColorShader.SetVec3("color", glm::vec3(1, 1, 0));
    _solidColorShader.SetMat4("model", glm::mat4(1));

    if (_toggles.drawLines) {
        for (Triangle& tri : VoxelWorld::GetAllTriangleOcculders()) {
            Triangle tri2 = tri;
            tri.color = YELLOW;
                 QueueTriangleForLineRendering(tri2);
        }
    }

    //_drawLines = true;
    for (Line& line : VoxelWorld::GetTestRays()) {
        //     QueueLineForDrawing(line);
    }


    // glm::vec3 worldPosTest = glm::vec3(3.1f, 1, 2.1f);
    // QueuePointForDrawing(Point(worldPosTest, YELLOW));

     // Draw lights
    if (_toggles.drawLights) {
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

    if (_toggles.drawProbes) {
        // Draw propogated light values
        for (int x = 0; x < VoxelWorld::GetPropogationGridWidth(); x++) {
            for (int y = 0; y < VoxelWorld::GetPropogationGridHeight(); y++) {
                for (int z = 0; z < VoxelWorld::GetPropogationGridDepth(); z++) {

                    GridProbe& probe = VoxelWorld::GetProbeByGridIndex(x, y, z);
                    if (probe.color == BLACK)// || probe.ignore)
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


    // _toggles.drawLines = true;

     // DDA


    static int i = 0;
    if (Input::KeyPressed(HELL_KEY_N)) {
        Audio::PlayAudio("RE_Beep.wav", 0.25f);
        i--;
    }
    if (Input::KeyPressed(HELL_KEY_M)) {
        Audio::PlayAudio("RE_Beep.wav", 0.25f);
        i++;
    }
    //VoxelFace& face = VoxelWorld::GetXFrontFacingVoxels()[i];

//    VoxelCell& voxel = VoxelWorld::GetVoxel(i, 2, 1);


   // if (CellIsEmpty(x, y, z + 1)) {
        glm::vec3 origin = glm::vec3(2, 2, i) + NRM_Z_FORWARD;
   //     if (!ClosestHit(origin, destination).hitFound) {


        //    voxel.forwardFaceZ.accumulatedDirectLighting += Util::GetDirectLightAtAPoint(light, (origin - NRM_Z_FORWARD) * _voxelSize, NRM_Z_FORWARD, _voxelSize);
    //  std::cout << "hi\n";
    //glm::vec3 origin = VoxelWorld::GetVoxelXForwardFaceCenterInGridSpace(2, 2, i) + NRM_Z_FORWARD + (NRM_Z_FORWARD * 0.55f);
   //glm::vec3 origin = glm::vec3(i, 2, 2);

    glm::vec3 lightPos = { VoxelWorld::GetLightByIndex(0).x, VoxelWorld::GetLightByIndex(0).y, VoxelWorld::GetLightByIndex(0).z };
  //  TextBlitter::_debugTextToBilt = "Voxel: " + Util::Vec3ToString(origin) + "\n";
  //  TextBlitter::_debugTextToBilt += "Light: " + Util::Vec3ToString(lightPos) + "\n";
        
    if (Input::KeyPressed(HELL_KEY_LEFT)) {
        Audio::PlayAudio("RE_Beep.wav", 0.25f);
        origin.x--;
    }
    if (Input::KeyPressed(HELL_KEY_RIGHT)) {
        Audio::PlayAudio("RE_Beep.wav", 0.25f);
        origin.x++;
    }
    if (Input::KeyPressed(HELL_KEY_DOWN)) {
        Audio::PlayAudio("RE_Beep.wav", 0.25f);
        origin.z--;
    }
    if (Input::KeyPressed(HELL_KEY_UP)) {
        Audio::PlayAudio("RE_Beep.wav", 0.25f);
        origin.z++;
    }

    glEnable(GL_DEPTH_TEST);

   // std::cout << "\n";
    for (int j = 0; j < 4; j++) {

        glm::vec3 color = GREEN;
        Light& light = VoxelWorld::GetLightByIndex(j);
        glm::vec3 dest = glm::vec3(light.x, light.y, light.z) ;
   //     QueuePointForDrawing(Point(glm::vec3(dest.x, dest.y, dest.z) * VoxelWorld::GetVoxelSize(), YELLOW));
        auto hitData = VoxelWorld::ClosestHit(origin, dest);

        if (hitData.hitFound) {
            color = RED;
            float x = hitData.hitPos.x;
            float y = hitData.hitPos.y;
            float z = hitData.hitPos.z;
   //         QueuePointForDrawing(Point(glm::vec3(x, y, z) * VoxelWorld::GetVoxelSize(), BLUE));
          //  std::cout << Util::Vec3ToString(origin) << "  hit  " << Util::Vec3ToString(dest) << "  at  " << x << ", " << y << ", " << z << "\n";
        }

    //    QueueLineForDrawing(Line(origin * VoxelWorld::GetVoxelSize(), dest * VoxelWorld::GetVoxelSize(), color));
    }

    _solidColorShader.SetMat4("model", glm::mat4(1));
    _solidColorShader.SetBool("uniformColor", false);
   // glEnable(GL_DEPTH_TEST);
    RenderImmediate();

   

   // QueuePointForDrawing(Point(glm::vec3(origin.x, origin.y, origin.z ), YELLOW));

    glDisable(GL_DEPTH_TEST);
    auto allVoxels = VoxelWorld::GetAllSolidPositions();
   // for (auto& v : allVoxels)
   //     QueuePointForDrawing(Point(v, BLUE));
    RenderImmediate();











  //  QueuePointForDrawing(p);
  //  QueuePointForDrawing(p2);


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
    
    TextBlitter::_debugTextToBilt = "";
    //_toggles.drawLines = true;
 //   TextBlitter::_debugTextToBilt += "View Matrix\n";
 //   TextBlitter::_debugTextToBilt += Util::Mat4ToString(Player::GetViewMatrix()) + "\n" + "\n";
    //TextBlitter::_debugTextToBilt += "Inverse View Matrix\n";
    //TextBlitter::_debugTextToBilt += Util::Mat4ToString(Player::GetInverseViewMatrix()) + "\n" + "\n";
    TextBlitter::_debugTextToBilt += "View pos: " + Util::Vec3ToString(Player::GetViewPos()) + "\n";
 //   TextBlitter::_debugTextToBilt += "Forward: " + Util::Vec3ToString(Player::GetCameraFront()) + "\n";
  //  TextBlitter::_debugTextToBilt += "Up: " + Util::Vec3ToString(Player::GetCameraUp()) + "\n";
 //   TextBlitter::_debugTextToBilt += "Right: " + Util::Vec3ToString(Player::GetCameraRight()) + "\n" + "\n";
  //  TextBlitter::_debugTextToBilt += "Total faces: " + std::to_string(::VoxelWorld::GetTotalVoxelFaceCount()) + "\n" + "\n";

    
   // TextBlitter::_debugTextToBilt += "Front2: " + Util::Vec3ToString(Player::GetCameraFront() * glm::vec3(-1)) + "\n" + "\n";

 //  



    // Render shadowmaps for next frame


    _solidTrianglePoints.clear();
    for (Triangle& tri : VoxelWorld::GetAllTriangleOcculders()) {
        _solidTrianglePoints.push_back(Point(tri.p1, YELLOW));
        _solidTrianglePoints.push_back(Point(tri.p2, YELLOW));
        _solidTrianglePoints.push_back(Point(tri.p3, YELLOW));
    }
    glBufferData(GL_ARRAY_BUFFER, _solidTrianglePoints.size() * sizeof(Point), _solidTrianglePoints.data(), GL_STATIC_DRAW);
    glBindVertexArray(_pointLineVAO);

   // Triangle& tri = VoxelWorld::GetAllTriangleOcculders()[0];
   // std::cout << Util::Vec3ToString(tri.p1) << ", " << Util::Vec3ToString(tri.p2) <<", " << Util::Vec3ToString(tri.p3) << "\n";

    _shadowMapShader.Use();
    _shadowMapShader.SetFloat("far_plane", SHADOW_FAR_PLANE);
    _shadowMapShader.SetMat4("model", glm::mat4(1));


    glDepthMask(true);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    // glEnable(GL_CULL_FACE);
    // glCullFace(GL_FRONT);

    auto lights = VoxelWorld::GetLights();
//
   // WipeShadowMaps();
    //glDisable(GL_CULL_FACE);
    for (int i = 0; i < lights.size(); i++) {

        glBindVertexArray(_pointLineVAO);
        glBindBuffer(GL_ARRAY_BUFFER, _pointLineVBO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)offsetof(Point, color));
        glBufferData(GL_ARRAY_BUFFER, _solidTrianglePoints.size() * sizeof(Point), _solidTrianglePoints.data(), GL_STATIC_DRAW);


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
  //  std::cout << "there were  " << _solidTrianglePoints.size() << "tris\n";
   // return;

    _solidTrianglePoints.clear();






    // Blit image back to frame buffer
   // glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, _gBuffer.GetID());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _gBufferFinalSize.GetID());
    glBlitFramebuffer(0, 0, _gBuffer.GetWidth(), _gBuffer.GetHeight(), 0, 0, _gBufferFinalSize.GetWidth(), _gBufferFinalSize.GetHeight(), GL_COLOR_BUFFER_BIT, GL_LINEAR);


    QueueUIForRendering("CrosshairDot", _renderWidth / 2, _renderHeight / 2, true);

    glBindFramebuffer(GL_FRAMEBUFFER, _gBufferFinalSize.GetID());
    glViewport(0, 0, _gBufferFinalSize.GetWidth(), _gBufferFinalSize.GetHeight());
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    Renderer::RenderUI();


    glBindFramebuffer(GL_READ_FRAMEBUFFER, _gBufferFinalSize.GetID());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, _gBufferFinalSize.GetWidth(), _gBufferFinalSize.GetHeight(), 0, 0, GL::GetWindowWidth(), GL::GetWindowHeight(), GL_COLOR_BUFFER_BIT, GL_NEAREST);




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

void Renderer::RenderEditorFrame() {

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











   //DDA
  // _voxelGrid[20][4][18] = { true, WHITE };
  // _voxelGrid[21][4][5] = { true, WHITE };

















   // QueuePointForDrawing(Point(GetEditorWorldPosFromCoord(0, 0), RED));
  //  QueuePointForDrawing(Point(GetEditorWorldPosFromCoord(0, 1), GREEN));
    //QueuePointForDrawing(Point(GetEditorWorldPosFromCoord(2, 2), BLUE));

    _editorSolidColorShader.Use();
    _editorSolidColorShader.SetMat4("projection", projection);
    _editorSolidColorShader.SetBool("uniformColor", false);
    _editorSolidColorShader.SetFloat("viewportWidth", _renderWidth);
    _editorSolidColorShader.SetFloat("viewportHeight", _renderHeight);
    _editorSolidColorShader.SetFloat("cameraX", Editor::GetCameraGridX());
    _editorSolidColorShader.SetFloat("cameraZ", Editor::GetCameraGridZ());
    glDisable(GL_DEPTH_TEST);
    RenderImmediate();


    QueueUIForRendering("CrosshairDot", Editor::GetMouseScreenX(), Editor::GetMouseScreenZ(), true);
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
void Renderer::ToggleRenderingAsVoxelDirectLighting() {
    _toggles.renderAsVoxelDirectLighting = !_toggles.renderAsVoxelDirectLighting;
}

static Mesh _quadMesh;

inline void DrawQuad(const std::string& textureName, int xPosition, int yPosition, bool centered = false, int xSize = -1, int ySize = -1, int xClipMin = -1, int xClipMax = -1, int yClipMin = -1, int yClipMax = -1, float scale = 0.5f) {

    float quadWidth = xSize;
    float quadHeight = ySize;
    if (xSize == -1) {
        quadWidth = AssetManager::GetTexture(textureName).GetWidth();
    }
    if (ySize == -1) {
        quadHeight = AssetManager::GetTexture(textureName).GetHeight();
    }
    if (centered) {
        xPosition -= quadWidth / 2;
        yPosition -= quadHeight / 2;
    }
    float renderTargetWidth = _renderWidth * scale;
    float renderTargetHeight = _renderHeight * scale;

    float width = (1.0f / renderTargetWidth) * quadWidth;
    float height = (1.0f / renderTargetHeight) * quadHeight;
    float ndcX = ((xPosition + (quadWidth / 2.0f)) / renderTargetWidth) * 2 - 1;
    float ndcY = ((yPosition + (quadHeight / 2.0f)) / renderTargetHeight) * 2 - 1;
    Transform transform;
    transform.position.x = ndcX;
    transform.position.y = ndcY * -1;
    transform.scale = glm::vec3(width, height * -1, 1);

   // glDisable(GL_CULL_FACE);

    _UIShader.SetMat4("model", transform.to_mat4());
    //int meshIndex = AssetManager::GetModel("blitter_quad")->_meshIndices[0];
    //int textureIndex = AssetManager::GetTextureIndex(textureName);
    //SubmitUI(meshIndex, textureIndex, 0, transform.to_mat4(), destination, xClipMin, xClipMax, yClipMin, yClipMax);

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
        _quadMesh = Mesh(vertices, indices);
    }

    //QueueUIForRendering("CrosshairDot", _renderWidth / 2, _renderHeight / 2, true);

    for (UIRenderInfo& uiRenderInfo : _UIRenderInfos) {
        AssetManager::GetTexture(uiRenderInfo.textureName).Bind(0);
        DrawQuad(uiRenderInfo.textureName, uiRenderInfo.screenX, uiRenderInfo.screenY, uiRenderInfo.centered);
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
