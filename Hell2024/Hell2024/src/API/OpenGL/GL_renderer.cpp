#include "GL_renderer.h"
#include "GL_assetManager.h"
#include "GL_backEnd.h"
#include "Types/GL_gBuffer.h"
#include "Types/GL_shader.h"
#include "Types/GL_types.h"
#include "../../BackEnd/BackEnd.h"
#include "../../Renderer/RendererCommon.h"
#include "../../Renderer/TextBlitter.h"
#include "../../Core/Scene.h"
#include "../../Core/AssetManager.h"

namespace OpenGLRenderer {

    struct RenderTargets {
        OpenGL::RenderTarget loadingScreen;
        OpenGL::RenderTarget debugMenu;
        GBuffer gBuffer;

    } _renderTargets;

    struct Shaders {
        Shader geometry;
        Shader UI;
    } _shaders;

    OpenGLMesh _quadMesh;
    std::vector<UIRenderInfo> _UIRenderInfos;
}

void DrawRenderItem(RenderItem3D& renderItem);
void RenderUI(const std::vector<RenderItem2D>& renderItems);

void OpenGLRenderer::HotloadShaders() {
    _shaders.UI.Load("ui.vert", "ui.frag");
    _shaders.geometry.Load("geometry.vert", "geometry.frag");
}

void OpenGLRenderer::InitMinimum() {

    HotloadShaders();
        
    int desiredTotalLines = 40;
    float linesPerPresentHeight = (float)PRESENT_HEIGHT / (float)TextBlitter::GetLineHeight();
    float scaleRatio = (float)desiredTotalLines / (float)linesPerPresentHeight;

    _renderTargets.debugMenu.Configure(PRESENT_WIDTH, PRESENT_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, _renderTargets.debugMenu.fbo);
    glGenTextures(1, &_renderTargets.debugMenu.texture);
    glBindTexture(GL_TEXTURE_2D, _renderTargets.debugMenu.texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _renderTargets.debugMenu.width, _renderTargets.debugMenu.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _renderTargets.debugMenu.texture, 0);

    _renderTargets.loadingScreen.Configure(PRESENT_WIDTH * scaleRatio, PRESENT_HEIGHT * scaleRatio);
    glBindFramebuffer(GL_FRAMEBUFFER, _renderTargets.loadingScreen.fbo);
    glGenTextures(1, &_renderTargets.loadingScreen.texture);
    glBindTexture(GL_TEXTURE_2D, _renderTargets.loadingScreen.texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _renderTargets.loadingScreen.width, _renderTargets.loadingScreen.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _renderTargets.loadingScreen.texture, 0);
    
    // Move this to some later Init() function
    _renderTargets.gBuffer.Configure(PRESENT_WIDTH * 2, PRESENT_WIDTH * 2);
}

void OpenGLRenderer::RenderLoadingScreen() {

    glBindFramebuffer(GL_FRAMEBUFFER, _renderTargets.loadingScreen.fbo);
    glViewport(0, 0, _renderTargets.loadingScreen.width, _renderTargets.loadingScreen.height);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    _shaders.UI.Use();
    _shaders.UI.SetVec3("color", WHITE);
    _shaders.UI.SetVec3("overrideColor", WHITE);
    _shaders.UI.SetMat4("model", glm::mat4(1));

    std::string text = "";
    int maxLinesDisplayed = 40;
    int endIndex = OpenGLAssetManager::_loadLog.size();
    int beginIndex = std::max(0, endIndex - maxLinesDisplayed);
    for (int i = beginIndex; i < endIndex; i++) {
        text += OpenGLAssetManager::_loadLog[i] + "\n";
    }
    TextBlitter::_debugTextToBilt = text;
    TextBlitter::CreateRenderItems(_renderTargets.loadingScreen.width, _renderTargets.loadingScreen.height);
    RenderUI(TextBlitter::GetRenderItems());

    glViewport(0, 0, BackEnd::GetCurrentWindowWidth(), BackEnd::GetCurrentWindowHeight());
    glBindFramebuffer(GL_READ_FRAMEBUFFER, _renderTargets.loadingScreen.fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glBlitFramebuffer(0, 0, _renderTargets.loadingScreen.width, _renderTargets.loadingScreen.height, 0, 0, BackEnd::GetCurrentWindowWidth(), BackEnd::GetCurrentWindowHeight(), GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OpenGLRenderer::RenderWorld(std::vector<RenderItem3D>& renderItems) {

    Player* player = &Scene::_players[0];
    glm::mat4 projection = player->GetProjectionMatrix();
    glm::mat4 view = player->GetViewMatrix();

    // Render target
    GBuffer& gBuffer = _renderTargets.gBuffer;
    gBuffer.Bind();
    glViewport(0, 0, gBuffer.GetWidth(), gBuffer.GetHeight());
    unsigned int attachments[5] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT6, GL_COLOR_ATTACHMENT7 };
    glDrawBuffers(5, attachments);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // GL state
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());

    // Shader
    _shaders.geometry.Use();
    _shaders.geometry.SetMat4("projection", projection);
    _shaders.geometry.SetMat4("view", view);
    _shaders.geometry.SetMat4("model", glm::mat4(1));
    _shaders.geometry.SetVec3("viewPos", player->GetViewPos());
    _shaders.geometry.SetVec3("camForward", player->GetCameraForward());
    _shaders.geometry.SetBool("isAnimated", false);

    int materialIndex = AssetManager::GetMaterialIndex("Shell");
    AssetManager::BindMaterialByIndex(materialIndex);

    // Draw
    for (RenderItem3D& renderItem : renderItems) {
        DrawRenderItem(renderItem);
    }

    // Composite
    glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer.GetID());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glBlitFramebuffer(0, 0, gBuffer.GetWidth(), gBuffer.GetHeight(), 0, 0, BackEnd::GetCurrentWindowWidth(), BackEnd::GetCurrentWindowHeight(), GL_COLOR_BUFFER_BIT, GL_NEAREST);

}

void OpenGLRenderer::RenderUI(std::vector<RenderItem2D>& renderItems) {

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);

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
        _quadMesh = OpenGLMesh(vertices, indices, "QuadMesh");
    }

    _shaders.UI.Use();

    for (const RenderItem2D& renderItem : renderItems) {
        AssetManager::GetTextureByIndex(renderItem.textureIndex)->glTexture.Bind(0);
        _shaders.UI.SetMat4("model", renderItem.modelMatrix);
        _shaders.UI.SetVec3("colorTint", { renderItem.colorTintR, renderItem.colorTintG, renderItem.colorTintB });
        _quadMesh.Draw();

        //AssetManager::GetTexture(renderItem.textureName)->Bind(0);
        //Texture* texture = AssetManager::GetTexture(renderItem.textureName);
        //_shaders.UI.SetVec3("color", renderItem.color);
        //DrawQuad(PRESENT_WIDTH, PRESENT_HEIGHT, texture->GetWidth(), texture->GetHeight(), renderItem.screenX, renderItem.screenY, renderItem.centered);
    }

    _UIRenderInfos.clear();
    glDisable(GL_BLEND);
}


void DrawQuad2(int viewportWidth, int viewPortHeight, int textureWidth, int textureHeight, int xPos, int yPos, bool centered, float scale = 1.0f, int xSize = -1, int ySize = -1) {

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
    OpenGLRenderer::_shaders.UI.SetMat4("model", transform.to_mat4());
    OpenGLRenderer::_quadMesh.Draw();
}

void DrawRenderItem(RenderItem3D& renderItem) {
    Mesh* mesh = AssetManager::GetMeshByIndex(renderItem.meshIndex);
    glDrawElementsBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), mesh->baseVertex);
}


