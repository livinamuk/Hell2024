#include "GL_renderer.h"
#include "GL_backEnd.h"
#include "Types/GL_gBuffer.h"
#include "Types/GL_shader.h"
#include "Types/GL_shadowMap.h"
#include "Types/GL_frameBuffer.hpp"
#include "../../BackEnd/BackEnd.h"
#include "../../Core/Scene.h"
#include "../../Core/AssetManager.h"
#include "../../Core/Game.h"
#include "../../Renderer/GlobalIllumination.h"
#include "../../Renderer/RendererCommon.h"
#include "../../Renderer/TextBlitter.h"
#include "../../Renderer/RendererStorage.h"
#include "../../Renderer/RendererUtil.hpp"

namespace OpenGLRenderer {

    struct FrameBuffers {
        GLFrameBuffer loadingScreen;
        GLFrameBuffer debugMenu;
        GLFrameBuffer present;
        GLFrameBuffer gBuffer;
        GLFrameBuffer lighting;
        GLFrameBuffer finalFullSizeImage;
    } _frameBuffers;

    struct BlurFrameBuffers {
        std::vector<GLFrameBuffer> p1;
        std::vector<GLFrameBuffer> p2;
        std::vector<GLFrameBuffer> p3;
        std::vector<GLFrameBuffer> p4;
    } g_blurBuffers;

    struct Shaders {
        Shader geometry;
        Shader geometrySkinned;
        Shader lighting;
        Shader UI;
        Shader shadowMap;
        Shader debugSolidColor;
        Shader debugPointCloud;
        Shader flipBook;
        Shader decals;
        Shader glass;
        Shader horizontalBlur;
        Shader verticalBlur;
        Shader bloodDecals;
        Shader vatBlood;
        Shader skyBox;
        Shader debugProbes;
        ComputeShader postProcessing;
        ComputeShader glassComposite;
        ComputeShader emissiveComposite;
        ComputeShader pointCloudDirectLigthing;
        ComputeShader sandbox;
        ComputeShader bloom;

        Shader newDebugShader;
        ComputeShader computeSkinning;

    } _shaders;

    struct SSBOs {
        GLuint samplers = 0;
        GLuint renderItems2D = 0;
        GLuint animatedRenderItems3D = 0;
        GLuint animatedTransforms = 0;
        GLuint lights = 0;

        GLuint geometryRenderItems = 0;
        GLuint glassRenderItems = 0;
        GLuint bulletHoleDecalRenderItems = 0;
        GLuint bloodDecalRenderItems = 0;
        GLuint shadowMapGeometryRenderItems = 0;
        GLuint bloodVATRenderItems = 0;

        GLuint skinningTransforms = 0;
        GLuint baseAnimatedTransformIndices = 0;
        GLuint cameraData = 0;
        GLuint skinnedMeshInstanceData = 0;
        GLuint muzzleFlashData = 0;

    } _ssbos;

    GLuint _indirectBuffer = 0;
    std::vector<ShadowMap> _shadowMaps;
}

void DrawRenderItem(RenderItem3D& renderItem);
void MultiDrawIndirect(std::vector<DrawIndexedIndirectCommand>& commands, GLuint vertexArray);
void MultiDrawIndirect(std::vector<RenderItem3D>& renderItems, GLuint vertexArray);
void BlitFrameBuffer(GLFrameBuffer* src, GLFrameBuffer* dst, const char* srcName, const char* dstName, GLbitfield mask, GLenum filter);
void BlitFrameBuffer(GLFrameBuffer* src, GLFrameBuffer* dst, const char* srcName, const char* dstName, ViewportInfo srcRegion, ViewportInfo dstRegion, GLbitfield mask, GLenum filter);
void BlitPlayerPresentTargetToDefaultFrameBuffer(GLFrameBuffer* src, GLFrameBuffer* dst, const char* srcName, const char* dstName, GLbitfield mask, GLenum filter, BlitDstCoords& blitDstCoords);

void ClearRenderTargets();
void LightingPass(RenderData& renderData);
void DebugPassProbePass(RenderData& renderData);
void SkyBoxPass(RenderData& renderData);
void PostProcessingPass(RenderData& renderData);
void GeometryPass(RenderData& renderData);
void DrawVATBlood(RenderData& renderData);
void DrawBloodDecals(RenderData& renderData);
void DrawBulletDecals(RenderData& renderData);
void RenderUI(std::vector<RenderItem2D>& renderItems, GLFrameBuffer& frameBuffer, bool clearScreen);
void RenderShadowMapss(RenderData& renderData);
void UploadSSBOsGPU(RenderData& renderData);
void DebugPass(RenderData& renderData);
void MuzzleFlashPass(RenderData& renderData);
void GlassPass(RenderData& renderData);
void DownScaleGBuffer();
void ComputeSkin(RenderData& renderData);
void EmissivePass(RenderData& renderData);

void OpenGLRenderer::HotloadShaders() {

    std::cout << "Hotloading shaders...\n";

    _shaders.UI.Load("GL_ui.vert", "GL_ui.frag");
    _shaders.geometry.Load("GL_gbuffer.vert", "GL_gbuffer.frag");
    _shaders.geometrySkinned.Load("GL_gbufferSkinned.vert", "GL_gbufferSkinned.frag");
    _shaders.lighting.Load("GL_lighting.vert", "GL_lighting.frag");
    _shaders.shadowMap.Load("GL_shadowMap.vert", "GL_shadowMap.frag", "GL_shadowMap.geom");
    _shaders.flipBook.Load("GL_flipBook.vert", "GL_flipBook.frag");
    _shaders.glass.Load("GL_glass.vert", "GL_glass.frag");
    _shaders.decals.Load("GL_decals.vert", "GL_decals.frag");
    _shaders.horizontalBlur.Load("GL_blurHorizontal.vert", "GL_blur.frag");
    _shaders.verticalBlur.Load("GL_blurVertical.vert", "GL_blur.frag");
    _shaders.bloodDecals.Load("GL_bloodDecals.vert", "GL_bloodDecals.frag");
    _shaders.vatBlood.Load("GL_bloodVAT.vert", "GL_bloodVAT.frag");
    _shaders.skyBox.Load("GL_skybox.vert", "GL_skybox.frag");
    _shaders.debugSolidColor.Load("GL_debug_solidColor.vert", "GL_debug_solidColor.frag");
    _shaders.debugPointCloud.Load("GL_debug_pointCloud.vert", "GL_debug_pointCloud.frag");
    _shaders.debugProbes.Load("GL_debug_probes.vert", "GL_debug_probes.frag");
    //_shaders.glassComposite.Load("GL_glassComposite.vert", "GL_glassComposite.frag");

    _shaders.newDebugShader.Load("GL_new_debug_shader.vert", "GL_new_debug_shader.frag");

    _shaders.emissiveComposite.LoadOLD("res/shaders/OpenGL/GL_emissiveComposite.comp");
    _shaders.postProcessing.LoadOLD("res/shaders/OpenGL/GL_postProcessing.comp");
    _shaders.glassComposite.LoadOLD("res/shaders/OpenGL/GL_glassComposite.comp");
    _shaders.pointCloudDirectLigthing.LoadOLD("res/shaders/OpenGL/GL_pointCloudDirectLighting.comp");
    _shaders.computeSkinning.LoadOLD("res/shaders/OpenGL/GL_computeSkinning.comp");
    _shaders.sandbox.LoadOLD("res/shaders/OpenGL/GL_sandbox.comp");
    _shaders.bloom.LoadOLD("res/shaders/OpenGL/GL_bloom.comp");

}

void OpenGLRenderer::CreatePlayerRenderTargets(int presentWidth, int presentHeight) {

    if (_frameBuffers.present.GetHandle() != 0) {
        _frameBuffers.present.CleanUp();
    }
    if (_frameBuffers.gBuffer.GetHandle() != 0) {
        _frameBuffers.gBuffer.CleanUp();
    }

    _frameBuffers.present.Create("Present", presentWidth, presentHeight);
    _frameBuffers.present.CreateAttachment("Color", GL_RGBA8);

    _frameBuffers.gBuffer.Create("GBuffer", presentWidth * 2, presentHeight * 2);
    _frameBuffers.gBuffer.CreateAttachment("BaseColor", GL_RGBA8);
    _frameBuffers.gBuffer.CreateAttachment("Normal", GL_RGBA16F);
    _frameBuffers.gBuffer.CreateAttachment("RMA", GL_RGBA8);
    _frameBuffers.gBuffer.CreateAttachment("FinalLighting", GL_RGBA8);
    _frameBuffers.gBuffer.CreateAttachment("Glass", GL_RGBA8);
    _frameBuffers.gBuffer.CreateAttachment("EmissiveMask", GL_RGBA8);
    _frameBuffers.gBuffer.CreateAttachment("BloomPrePass", GL_RGBA8);
    _frameBuffers.gBuffer.CreateDepthAttachment(GL_DEPTH32F_STENCIL8);
}

void OpenGLRenderer::RecreateBlurBuffers() {

    // Destroy any existing textures
    for (auto& fbo : g_blurBuffers.p1) {
        fbo.CleanUp();
    }
    for (auto& fbo : g_blurBuffers.p2) {
        fbo.CleanUp();
    }
    for (auto& fbo : g_blurBuffers.p3) {
        fbo.CleanUp();
    }
    for (auto& fbo : g_blurBuffers.p4) {
        fbo.CleanUp();
    }
    g_blurBuffers.p1.clear();
    g_blurBuffers.p2.clear();
    g_blurBuffers.p3.clear();
    g_blurBuffers.p4.clear();

    // Create new framebuffers
    ViewportInfo viewportInfo = RendererUtil::CreateViewportInfo(0, Game::GetSplitscreenMode(), PRESENT_WIDTH * 2, PRESENT_HEIGHT * 2);
    int bufferWidth = viewportInfo.width;
    int bufferHeight = viewportInfo.height;
    GLFrameBuffer* frameBuffer = nullptr;
    std::string frameBufferName;

    for (int i = 0; i < 4; i++) {

        frameBufferName = "BlurBuffer_P1" + std::to_string(i);
        frameBuffer = &g_blurBuffers.p1.emplace_back();
        frameBuffer->Create(frameBufferName.c_str(), bufferWidth, bufferHeight);
        frameBuffer->CreateAttachment("ColorA", GL_RGBA8);
        frameBuffer->CreateAttachment("ColorB", GL_RGBA8);

        if (Game::GetSplitscreenMode() == SplitscreenMode::TWO_PLAYER || Game::GetSplitscreenMode() == SplitscreenMode::FOUR_PLAYER) {

            frameBufferName = "BlurBuffer_P2" + std::to_string(i);
            frameBuffer = &g_blurBuffers.p2.emplace_back();
            frameBuffer->Create(frameBufferName.c_str(), bufferWidth, bufferHeight);
            frameBuffer->CreateAttachment("ColorA", GL_RGBA8);
            frameBuffer->CreateAttachment("ColorB", GL_RGBA8);
        }
        if (Game::GetSplitscreenMode() == SplitscreenMode::FOUR_PLAYER) {

            frameBufferName = "BlurBuffer_P3" + std::to_string(i);
            frameBuffer = &g_blurBuffers.p3.emplace_back();
            frameBuffer->Create(frameBufferName.c_str(), bufferWidth, bufferHeight);
            frameBuffer->CreateAttachment("ColorA", GL_RGBA8);
            frameBuffer->CreateAttachment("ColorB", GL_RGBA8);

            frameBufferName = "BlurBuffer_P4" + std::to_string(i);
            frameBuffer = &g_blurBuffers.p4.emplace_back();
            frameBuffer->Create(frameBufferName.c_str(), bufferWidth, bufferHeight);
            frameBuffer->CreateAttachment("ColorA", GL_RGBA8);
            frameBuffer->CreateAttachment("ColorB", GL_RGBA8);
        }
        bufferWidth *= 0.5f;
        bufferHeight *= 0.5f;
    }
}

void OpenGLRenderer::InitMinimum() {

    HotloadShaders();

    int desiredTotalLines = 40;
    float linesPerPresentHeight = (float)PRESENT_HEIGHT / (float)TextBlitter::GetLineHeight(BitmapFontType::STANDARD);
    float scaleRatio = (float)desiredTotalLines / (float)linesPerPresentHeight;

    _frameBuffers.debugMenu.Create("DebugMenu", PRESENT_WIDTH, PRESENT_HEIGHT);
    _frameBuffers.debugMenu.CreateAttachment("Color", GL_RGBA8);

    _frameBuffers.loadingScreen.Create("LoadingsCreen", PRESENT_WIDTH * scaleRatio, PRESENT_HEIGHT * scaleRatio);
    _frameBuffers.loadingScreen.CreateAttachment("Color", GL_RGBA8);

    _frameBuffers.finalFullSizeImage.Create("FinalFullSizeImage", PRESENT_WIDTH, PRESENT_HEIGHT);
    _frameBuffers.finalFullSizeImage.CreateAttachment("Color", GL_RGBA8);

    CreatePlayerRenderTargets(PRESENT_WIDTH, PRESENT_HEIGHT);
    RecreateBlurBuffers();

    // Shader storage buffer objects
    glGenBuffers(1, &_indirectBuffer);

    glCreateBuffers(1, &_ssbos.geometryRenderItems);
    glNamedBufferStorage(_ssbos.geometryRenderItems, MAX_RENDER_OBJECTS_3D * sizeof(RenderItem3D), NULL, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &_ssbos.animatedRenderItems3D);
    glNamedBufferStorage(_ssbos.animatedRenderItems3D, MAX_RENDER_OBJECTS_3D * sizeof(RenderItem3D), NULL, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &_ssbos.renderItems2D);
    glNamedBufferStorage(_ssbos.renderItems2D, MAX_RENDER_OBJECTS_2D * sizeof(RenderItem2D), NULL, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &_ssbos.lights);
    glNamedBufferStorage(_ssbos.lights, MAX_LIGHTS * sizeof(GPULight), NULL, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &_ssbos.animatedTransforms);
    glNamedBufferStorage(_ssbos.animatedTransforms, MAX_ANIMATED_TRANSFORMS * sizeof(glm::mat4), NULL, GL_DYNAMIC_STORAGE_BIT);

    //glCreateBuffers(1, &_ssbos.decalMatrices);
    //glNamedBufferStorage(_ssbos.decalMatrices, MAX_INSTANCES * sizeof(glm::mat4), NULL, GL_DYNAMIC_STORAGE_BIT);

    //glCreateBuffers(1, &_ssbos.decalRenderItems);
    //glNamedBufferStorage(_ssbos.decalRenderItems, MAX_RENDER_OBJECTS_3D * sizeof(RenderItem3DInstanced), NULL, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &_ssbos.glassRenderItems);
    glNamedBufferStorage(_ssbos.glassRenderItems, MAX_GLASS_MESH_COUNT * sizeof(RenderItem3D), NULL, GL_DYNAMIC_STORAGE_BIT);

   // glCreateBuffers(1, &_ssbos.decalInstanceData);
   // glNamedBufferStorage(_ssbos.decalInstanceData, MAX_DECAL_COUNT * sizeof(InstanceData), NULL, GL_DYNAMIC_STORAGE_BIT);

    //glCreateBuffers(1, &_ssbos.geometryInstanceData);
   // glNamedBufferStorage(_ssbos.geometryInstanceData, MAX_RENDER_OBJECTS_3D * sizeof(InstanceData), NULL, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &_ssbos.shadowMapGeometryRenderItems);
    glNamedBufferStorage(_ssbos.shadowMapGeometryRenderItems, MAX_RENDER_OBJECTS_3D * sizeof(RenderItem3D), NULL, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &_ssbos.bulletHoleDecalRenderItems);
    glNamedBufferStorage(_ssbos.bulletHoleDecalRenderItems, MAX_DECAL_COUNT * sizeof(RenderItem3D), NULL, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &_ssbos.bloodDecalRenderItems);
    glNamedBufferStorage(_ssbos.bloodDecalRenderItems, MAX_BLOOD_DECAL_COUNT * sizeof(RenderItem3D), NULL, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &_ssbos.bloodVATRenderItems);
    glNamedBufferStorage(_ssbos.bloodVATRenderItems, MAX_VAT_INSTANCE_COUNT * sizeof(RenderItem3D), NULL, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &_ssbos.skinningTransforms);
    glNamedBufferStorage(_ssbos.skinningTransforms, MAX_ANIMATED_TRANSFORMS * sizeof(glm::mat4), NULL, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &_ssbos.baseAnimatedTransformIndices);
    glNamedBufferStorage(_ssbos.baseAnimatedTransformIndices, MAX_ANIMATED_TRANSFORMS * sizeof(uint32_t), NULL, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &_ssbos.cameraData);
    glNamedBufferStorage(_ssbos.cameraData, sizeof(CameraData) * 4, NULL, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &_ssbos.skinnedMeshInstanceData);
    glNamedBufferStorage(_ssbos.skinnedMeshInstanceData, sizeof(SkinnedRenderItem) * 4 * MAX_SKINNED_MESH, NULL, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &_ssbos.muzzleFlashData);
    glNamedBufferStorage(_ssbos.muzzleFlashData, sizeof(MuzzleFlashData) * 4, NULL, GL_DYNAMIC_STORAGE_BIT);


    for (int i = 0; i < 16; i++) {
        ShadowMap& shadowMap = _shadowMaps.emplace_back();
        shadowMap.Init();
    }
}

void OpenGLRenderer::BindBindlessTextures() {

    // Create the samplers SSBO if needed
    if (_ssbos.samplers == 0) {
        glCreateBuffers(1, &_ssbos.samplers);
        glNamedBufferStorage(_ssbos.samplers, TEXTURE_ARRAY_SIZE * sizeof(glm::uvec2), NULL, GL_DYNAMIC_STORAGE_BIT);
    }
    // Get the handles and stash em in a vector
    std::vector<GLuint64> samplers;
    samplers.reserve(AssetManager::GetTextureCount());
    for (int i = 0; i < AssetManager::GetTextureCount(); i++) {
        samplers.push_back(AssetManager::GetTextureByIndex(i)->GetGLTexture().GetBindlessID());
    }
    // Send to GPU
    glNamedBufferSubData(_ssbos.samplers, 0, samplers.size() * sizeof(glm::uvec2), &samplers[0]);
}

void DrawRenderItem(RenderItem3D& renderItem) {
    Mesh* mesh = AssetManager::GetMeshByIndex(renderItem.meshIndex);
    glDrawElementsBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), mesh->baseVertex);
}

void MultiDrawIndirect(std::vector<DrawIndexedIndirectCommand>& commands, GLuint vertexArray) {
    if (commands.size()) {
        // Feed the draw command data to the gpu
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, OpenGLRenderer::_indirectBuffer);
        glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawIndexedIndirectCommand) * commands.size(), commands.data(), GL_DYNAMIC_DRAW);

        // Fire of the commands
        glBindVertexArray(vertexArray);
        glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (GLvoid*)0, commands.size(), 0);
    }
}

void MultiDrawIndirect(std::vector<RenderItem3D>& renderItems, GLuint vertexArray) {

    if (renderItems.empty()) {
        return;
    }
    std::vector<DrawIndexedIndirectCommand> commands(renderItems.size());

    if (vertexArray == OpenGLBackEnd::GetVertexDataVAO()) {
        for (int i = 0; i < renderItems.size(); i++) {
            RenderItem3D& renderItem = renderItems[i];
            DrawIndexedIndirectCommand& command = commands[i];
            Mesh* mesh = AssetManager::GetMeshByIndex(renderItem.meshIndex);
            commands[i].indexCount = mesh->indexCount;
            commands[i].instanceCount = 1;
            commands[i].firstIndex = mesh->baseIndex;
            commands[i].baseVertex = mesh->baseVertex;
            commands[i].baseInstance = 0;
        }
    }
    else if (vertexArray == OpenGLBackEnd::GetWeightedVertexDataVAO()) {
        for (int i = 0; i < renderItems.size(); i++) {
            RenderItem3D& renderItem = renderItems[i];
            DrawIndexedIndirectCommand& command = commands[i];
            SkinnedMesh* mesh = AssetManager::GetSkinnedMeshByIndex(renderItem.meshIndex);
            commands[i].indexCount = mesh->indexCount;
            commands[i].instanceCount = 1;
            commands[i].firstIndex = mesh->baseIndex;
            commands[i].baseVertex = mesh->baseVertexGlobal;
            commands[i].baseInstance = 0;
        }
    }
    else {
        return;
    }

    // Feed the draw command data to the gpu
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, OpenGLRenderer::_indirectBuffer);
    glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawIndexedIndirectCommand) * commands.size(), commands.data(), GL_DYNAMIC_DRAW);

    // Fire of the commands
    glBindVertexArray(vertexArray);
    glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (GLvoid*)0, commands.size(), 0);
}

void BlitPlayerPresentTargetToDefaultFrameBuffer(GLFrameBuffer* src, GLFrameBuffer* dst, const char* srcName, const char* dstName, GLbitfield mask, GLenum filter, BlitDstCoords& blitDstCoords) {

    GLint srcHandle = 0;
    GLint dstHandle = 0;
    GLint srcWidth = BackEnd::GetCurrentWindowWidth();
    GLint srcHeight = BackEnd::GetCurrentWindowHeight();
    GLint dstWidth = BackEnd::GetCurrentWindowWidth();
    GLint dstHeight = BackEnd::GetCurrentWindowHeight();
    GLenum srcSlot = GL_BACK;
    GLenum dstSlot = GL_BACK;

    if (src) {
        srcHandle = src->GetHandle();
        srcWidth = src->GetWidth();
        srcHeight = src->GetHeight();
        srcSlot = src->GetColorAttachmentSlotByName(srcName);
    }

    GLint srcX0 = 0;
    GLint srcY0 = 0;
    GLint srcX1 = srcWidth;
    GLint srcY1 = srcHeight;
    GLint dstX0 = blitDstCoords.dstX0;
    GLint dstY0 = blitDstCoords.dstY0;
    GLint dstX1 = blitDstCoords.dstX1;
    GLint dstY1 = blitDstCoords.dstY1;



    glBindFramebuffer(GL_READ_FRAMEBUFFER, srcHandle);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dstHandle);
    glReadBuffer(srcSlot);
    glDrawBuffer(dstSlot);;
    glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);

}



void BlitFrameBuffer(GLFrameBuffer* src, GLFrameBuffer* dst, const char* srcName, const char* dstName, GLbitfield mask, GLenum filter) {

    GLint srcHandle = 0;
    GLint dstHandle = 0;
    GLint srcWidth = BackEnd::GetCurrentWindowWidth();
    GLint srcHeight = BackEnd::GetCurrentWindowHeight();
    GLint dstWidth = BackEnd::GetCurrentWindowWidth();
    GLint dstHeight = BackEnd::GetCurrentWindowHeight();
    GLenum srcSlot = GL_BACK;
    GLenum dstSlot = GL_BACK;

    if (src) {
        srcHandle = src->GetHandle();
        srcWidth = src->GetWidth();
        srcHeight = src->GetHeight();
        srcSlot = src->GetColorAttachmentSlotByName(srcName);
    }
    if (dst) {
        dstHandle = dst->GetHandle();
        dstWidth = dst->GetWidth();
        dstHeight = dst->GetHeight();
        dstSlot = dst->GetColorAttachmentSlotByName(dstName);
    }

    glBindFramebuffer(GL_READ_FRAMEBUFFER, srcHandle);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dstHandle);
    glReadBuffer(srcSlot);
    glDrawBuffer(dstSlot);;
    glBlitFramebuffer(0, 0, srcWidth, srcHeight, 0, 0, dstWidth, dstHeight, mask, filter);
}


void BlitFrameBuffer(GLFrameBuffer* src, GLFrameBuffer* dst, const char* srcName, const char* dstName, ViewportInfo srcRegion, ViewportInfo dstRegion, GLbitfield mask, GLenum filter) {

    GLint srcHandle = 0;
    GLint dstHandle = 0;

    GLint srcX0 = srcRegion.xOffset;
    GLint srcX1 = srcRegion.xOffset + srcRegion.width;
    GLint srcY0 = srcRegion.yOffset;
    GLint srcY1 = srcRegion.yOffset + srcRegion.height;
    GLint dstX0 = dstRegion.xOffset;
    GLint dstX1 = dstRegion.xOffset + dstRegion.width;
    GLint dstY0 = dstRegion.yOffset;
    GLint dstY1 = dstRegion.yOffset + dstRegion.height;

    GLenum srcSlot = GL_BACK;
    GLenum dstSlot = GL_BACK;

    if (src) {
        srcHandle = src->GetHandle();
        srcSlot = src->GetColorAttachmentSlotByName(srcName);
    }
    if (dst) {
        dstHandle = dst->GetHandle();
        dstSlot = dst->GetColorAttachmentSlotByName(dstName);
    }

    glBindFramebuffer(GL_READ_FRAMEBUFFER, srcHandle);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dstHandle);
    glReadBuffer(srcSlot);
    glDrawBuffer(dstSlot);;
    glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
}


void DrawQuad() {
    Mesh* mesh = AssetManager::GetQuadMesh();
    glDrawElementsBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), mesh->baseVertex);
}

void DownScaleGBuffer() {
    GLFrameBuffer& gBuffer = OpenGLRenderer::_frameBuffers.gBuffer;
    GLFrameBuffer& presentBuffer = OpenGLRenderer::_frameBuffers.present;
  //  ViewportInfo gBufferViewportInfo = RendererUtil::CreateViewportInfo(playerIndex, Game::GetSplitscreenMode(), gBuffer.GetWidth(), gBuffer.GetHeight());
  //  ViewportInfo presentViewportInfo = RendererUtil::CreateViewportInfo(playerIndex, Game::GetSplitscreenMode(), presentBuffer.GetWidth(), presentBuffer.GetHeight());

  //  void BlitFrameBuffer(GLFrameBuffer * src, GLFrameBuffer * dst, const char* srcName, const char* dstName, GLbitfield mask, GLenum filter);

  //  BlitFrameBuffer(&gBuffer, &presentBuffer, "FinalLighting", "Color", gBufferViewportInfo, presentViewportInfo, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    BlitFrameBuffer(&gBuffer, &presentBuffer, "FinalLighting", "Color", GL_COLOR_BUFFER_BIT, GL_LINEAR);
}

/*
   ▄████████    ▄████████ ███▄▄▄▄   ████████▄     ▄████████    ▄████████         ▄███████▄    ▄████████    ▄████████    ▄████████    ▄████████    ▄████████
  ███    ███   ███    ███ ███▀▀▀██▄ ███   ▀███   ███    ███   ███    ███        ███    ███   ███    ███   ███    ███   ███    ███   ███    ███   ███    ███
  ███    ███   ███    █▀  ███   ███ ███    ███   ███    █▀    ███    ███        ███    ███   ███    ███   ███    █▀    ███    █▀    ███    █▀    ███    █▀
 ▄███▄▄▄▄██▀  ▄███▄▄▄     ███   ███ ███    ███  ▄███▄▄▄      ▄███▄▄▄▄██▀        ███    ███   ███    ███   ███          ███         ▄███▄▄▄       ███
▀▀███▀▀▀▀▀   ▀▀███▀▀▀     ███   ███ ███    ███ ▀▀███▀▀▀     ▀▀███▀▀▀▀▀        ▀█████████▀  ▀███████████ ▀███████████ ▀███████████ ▀▀███▀▀▀     ▀███████████
▀███████████   ███    █▄  ███   ███ ███    ███   ███    █▄  ▀███████████        ███          ███    ███          ███          ███   ███    █▄           ███
  ███    ███   ███    ███ ███   ███ ███   ▄███   ███    ███   ███    ███        ███          ███    ███    ▄█    ███    ▄█    ███   ███    ███    ▄█    ███
  ███    ███   ██████████  ▀█   █▀  ████████▀    ██████████   ███    ███       ▄████▀        ███    █▀   ▄████████▀   ▄████████▀    ██████████  ▄████████▀
  ███    ███                                                  ███    ███                                                                                     */


void BindFrameBuffer(GLFrameBuffer& frameBuffer) {
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer.GetHandle());
}

void SetViewport(ViewportInfo viewportInfo) {
    glViewport(viewportInfo.xOffset, viewportInfo.yOffset, viewportInfo.width, viewportInfo.height);
}

void OpenGLRenderer::RenderLoadingScreen(std::vector<RenderItem2D>& renderItems) {

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, _ssbos.samplers);

    RenderUI(renderItems, _frameBuffers.loadingScreen, true);
    BlitFrameBuffer(&_frameBuffers.loadingScreen, 0, "Color", "", GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

void UploadSSBOsGPU(RenderData& renderData) {

    glNamedBufferSubData(OpenGLRenderer::_ssbos.geometryRenderItems, 0, renderData.geometryDrawInfo.renderItems.size() * sizeof(RenderItem3D), &renderData.geometryDrawInfo.renderItems[0]);
    glNamedBufferSubData(OpenGLRenderer::_ssbos.lights, 0, renderData.lights.size() * sizeof(GPULight), &renderData.lights[0]);
    //glNamedBufferSubData(OpenGLRenderer::_ssbos.cameraData, 0, sizeof(CameraData), &renderData.cameraData);
    glNamedBufferSubData(OpenGLRenderer::_ssbos.animatedTransforms, 0, (*renderData.animatedTransforms).size() * sizeof(glm::mat4), &(*renderData.animatedTransforms)[0]);
    glNamedBufferSubData(OpenGLRenderer::_ssbos.animatedRenderItems3D, 0, renderData.animatedRenderItems3D.size() * sizeof(RenderItem3D), &renderData.animatedRenderItems3D[0]);
    glNamedBufferSubData(OpenGLRenderer::_ssbos.glassRenderItems, 0, renderData.glassDrawInfo.renderItems.size() * sizeof(RenderItem3D), &renderData.glassDrawInfo.renderItems[0]);
    glNamedBufferSubData(OpenGLRenderer::_ssbos.shadowMapGeometryRenderItems, 0, renderData.shadowMapGeometryDrawInfo.renderItems.size() * sizeof(RenderItem3D), &renderData.shadowMapGeometryDrawInfo.renderItems[0]);
    glNamedBufferSubData(OpenGLRenderer::_ssbos.bulletHoleDecalRenderItems, 0, renderData.bulletHoleDecalDrawInfo.renderItems.size() * sizeof(RenderItem3D), &renderData.bulletHoleDecalDrawInfo.renderItems[0]);
    glNamedBufferSubData(OpenGLRenderer::_ssbos.bloodDecalRenderItems, 0, renderData.bloodDecalDrawInfo.renderItems.size() * sizeof(RenderItem3D), &renderData.bloodDecalDrawInfo.renderItems[0]);
    glNamedBufferSubData(OpenGLRenderer::_ssbos.bloodVATRenderItems, 0, renderData.bloodVATDrawInfo.renderItems.size() * sizeof(RenderItem3D), &renderData.bloodVATDrawInfo.renderItems[0]);

    glNamedBufferSubData(OpenGLRenderer::_ssbos.skinningTransforms, 0, renderData.skinningTransforms.size() * sizeof(glm::mat4), &renderData.skinningTransforms[0]);
    glNamedBufferSubData(OpenGLRenderer::_ssbos.baseAnimatedTransformIndices, 0, renderData.baseAnimatedTransformIndices.size() * sizeof(uint32_t), &renderData.baseAnimatedTransformIndices[0]);

    glNamedBufferSubData(OpenGLRenderer::_ssbos.cameraData, 0, 4 * sizeof(CameraData), &renderData.cameraData[0]);


    glNamedBufferSubData(OpenGLRenderer::_ssbos.skinnedMeshInstanceData, 0, sizeof(SkinnedRenderItem) * renderData.allSkinnedRenderItems.size(), &renderData.allSkinnedRenderItems[0]);

    glNamedBufferSubData(OpenGLRenderer::_ssbos.muzzleFlashData, 0, sizeof(MuzzleFlashData) * 4, &renderData.muzzleFlashData[0]);


  //  std::cout << "renderData.skinningTransforms.size(): " << renderData.skinningTransforms.size() << "\n";
}

void OpenGLRenderer::RenderGame(RenderData& renderData) {

    GLFrameBuffer& gBuffer = _frameBuffers.gBuffer;
    GLFrameBuffer& present = _frameBuffers.present;

    UploadSSBOsGPU(renderData);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, _ssbos.samplers);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, OpenGLRenderer::_ssbos.geometryRenderItems);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, OpenGLRenderer::_ssbos.lights);
   // glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, OpenGLRenderer::_ssbos.cameraData);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, OpenGLRenderer::_ssbos.animatedTransforms);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, OpenGLRenderer::_ssbos.animatedRenderItems3D);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, OpenGLRenderer::_ssbos.glassRenderItems);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 12, OpenGLRenderer::_ssbos.shadowMapGeometryRenderItems);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 13, OpenGLRenderer::_ssbos.bulletHoleDecalRenderItems);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 14, OpenGLRenderer::_ssbos.bloodDecalRenderItems);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 15, OpenGLRenderer::_ssbos.bloodVATRenderItems);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 16, OpenGLRenderer::_ssbos.cameraData);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 17, OpenGLRenderer::_ssbos.skinnedMeshInstanceData);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 18, OpenGLRenderer::_ssbos.muzzleFlashData);

    ClearRenderTargets();
    RenderShadowMapss(renderData);
    ComputeSkin(renderData);
    GeometryPass(renderData);
    DrawVATBlood(renderData);
    DrawBloodDecals(renderData);
    DrawBulletDecals(renderData);
    LightingPass(renderData);

   // DebugPassProbePass(renderData);
    SkyBoxPass(renderData);
    GlassPass(renderData);
    EmissivePass(renderData);
    MuzzleFlashPass(renderData);
    PostProcessingPass(renderData);
    RenderUI(renderData.renderItems2DHiRes, _frameBuffers.gBuffer, false);
    DownScaleGBuffer();
    DebugPass(renderData);
    RenderUI(renderData.renderItems2D, _frameBuffers.present, false);
}


void ClearRenderTargets() {

    // Clear GBuffer color attachments
    GLFrameBuffer& gBuffer = OpenGLRenderer::_frameBuffers.gBuffer;
    GLFrameBuffer& present = OpenGLRenderer::_frameBuffers.present;

    gBuffer.Bind();
    gBuffer.SetViewport();
    unsigned int attachments[7] = {
        GL_COLOR_ATTACHMENT0,
        GL_COLOR_ATTACHMENT1,
        GL_COLOR_ATTACHMENT2,
        GL_COLOR_ATTACHMENT6,
        GL_COLOR_ATTACHMENT7,
        gBuffer.GetColorAttachmentSlotByName("Glass"),
        gBuffer.GetColorAttachmentSlotByName("EmissiveMask") };
    glDrawBuffers(7, attachments);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

}

void SkyBoxPass(RenderData& renderData) {

    static CubemapTexture* cubemapTexture = AssetManager::GetCubemapTextureByIndex(AssetManager::GetCubemapTextureIndexByName("NightSky"));
    static Mesh* mesh = AssetManager::GetMeshByIndex(AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Cube"))->GetMeshIndices()[0]);

    GLFrameBuffer& gBuffer = OpenGLRenderer::_frameBuffers.gBuffer;
    Player* player = Game::GetPlayerByIndex(0);

    Transform skyBoxTransform;
    skyBoxTransform.position = player->GetViewPos();
    skyBoxTransform.scale = glm::vec3(50.0f);

    // Render target
    glEnable(GL_DEPTH_TEST);
    //glDrawBuffer(gBuffer.GetColorAttachmentSlotByName("FinalLighting"));

    unsigned int attachments[2] = {
        gBuffer.GetColorAttachmentSlotByName("FinalLighting"),
        gBuffer.GetColorAttachmentSlotByName("Normal") };
    glDrawBuffers(2, attachments);

    for (int i = 0; i < renderData.playerCount; i++) {
        ViewportInfo viewportInfo = RendererUtil::CreateViewportInfo(i, Game::GetSplitscreenMode(), gBuffer.GetWidth(), gBuffer.GetHeight());
        SetViewport(viewportInfo);
        // Draw
        Shader& shader = OpenGLRenderer::_shaders.skyBox;
        shader.Use();
        shader.SetMat4("projection", renderData.cameraData[0].projection);
        shader.SetMat4("view", renderData.cameraData[0].view);
        shader.SetMat4("model", skyBoxTransform.to_mat4());
        shader.SetInt("playerIndex", i);
        shader.SetVec3("skyboxTint", Game::GameSettings().skyBoxTint);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture->GetGLTexture().GetID());
        glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
        glDrawElementsBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), mesh->baseVertex);
    }
}

void EmissivePass(RenderData& renderData) {

    GLFrameBuffer& gBuffer = OpenGLRenderer::_frameBuffers.gBuffer;
    Shader& horizontalBlurShader = OpenGLRenderer::_shaders.horizontalBlur;
    Shader& verticalBlurShader = OpenGLRenderer::_shaders.verticalBlur;

    std::vector<GLFrameBuffer>* blurBuffers = nullptr;

    for (int i = 0; i < renderData.playerCount; i++) {

        if (i == 0) {
            blurBuffers = &OpenGLRenderer::g_blurBuffers.p1;
        }
        if (i == 1) {
            blurBuffers = &OpenGLRenderer::g_blurBuffers.p2;
        }
        if (i == 2) {
            blurBuffers = &OpenGLRenderer::g_blurBuffers.p3;
        }
        if (i == 3) {
            blurBuffers = &OpenGLRenderer::g_blurBuffers.p4;
        }

        (*blurBuffers)[0].Bind();
        (*blurBuffers)[0].SetViewport();

        ViewportInfo gBufferViewportInfo = RendererUtil::CreateViewportInfo(i, Game::GetSplitscreenMode(), gBuffer.GetWidth(), gBuffer.GetHeight());
        ViewportInfo blurBufferViewportInfo;
        blurBufferViewportInfo.xOffset = 0;
        blurBufferViewportInfo.yOffset = 0;
        blurBufferViewportInfo.width = (*blurBuffers)[0].GetWidth();
        blurBufferViewportInfo.height = (*blurBuffers)[0].GetHeight();
        BlitFrameBuffer(&gBuffer, &(*blurBuffers)[0], "EmissiveMask", "ColorA", gBufferViewportInfo, blurBufferViewportInfo, GL_COLOR_BUFFER_BIT, GL_LINEAR);

        // First round blur (vertical)
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, (*blurBuffers)[0].GetColorAttachmentHandleByName("ColorA"));
        glDrawBuffer(GL_COLOR_ATTACHMENT1);
        verticalBlurShader.Use();
        verticalBlurShader.SetFloat("targetHeight", (*blurBuffers)[0].GetHeight());
        DrawQuad();

        for (int i = 1; i < 4; i++) {

            GLuint horizontalSourceHandle = (*blurBuffers)[i - 1].GetColorAttachmentHandleByName("ColorB");
            GLuint verticalSourceHandle = (*blurBuffers)[i].GetColorAttachmentHandleByName("ColorA");

            (*blurBuffers)[i].Bind();
            (*blurBuffers)[i].SetViewport();

            // Second round blur (horizontal)
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, horizontalSourceHandle);
            glDrawBuffer(GL_COLOR_ATTACHMENT0);
            horizontalBlurShader.Use();
            horizontalBlurShader.SetFloat("targetWidth", (*blurBuffers)[i].GetWidth());
            DrawQuad();

            // Second round blur (vertical)
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, verticalSourceHandle);
            glDrawBuffer(GL_COLOR_ATTACHMENT1);
            verticalBlurShader.Use();
            verticalBlurShader.SetFloat("targetHeight", (*blurBuffers)[i].GetHeight());
            DrawQuad();
        }

        // Composite those blurred textures into the main lighting image
        ViewportInfo viewportInfo = RendererUtil::CreateViewportInfo(i, Game::GetSplitscreenMode(), gBuffer.GetWidth(), gBuffer.GetHeight());
        ComputeShader& computeShader = OpenGLRenderer::_shaders.emissiveComposite;
        computeShader.Use();
        computeShader.SetFloat("screenWidth", renderData.cameraData[0].viewportWidth);
        computeShader.SetFloat("screenHeight", renderData.cameraData[0].viewportHeight);
        computeShader.SetInt("viewportOffsetX", viewportInfo.xOffset);
        computeShader.SetInt("viewportOffsetY", viewportInfo.yOffset);
        computeShader.SetInt("playerIndex", i);
        glBindImageTexture(0, gBuffer.GetColorAttachmentHandleByName("FinalLighting"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, (*blurBuffers)[0].GetColorAttachmentHandleByName("ColorB"));
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, (*blurBuffers)[1].GetColorAttachmentHandleByName("ColorB"));
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, (*blurBuffers)[2].GetColorAttachmentHandleByName("ColorB"));
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, (*blurBuffers)[3].GetColorAttachmentHandleByName("ColorB"));

        glDispatchCompute(viewportInfo.width / 16, viewportInfo.height / 4, 1);
    }
}

void RenderShadowMapss(RenderData& renderData) {

    if (0 != 0) {
        return;
    }

    Shader& shader = OpenGLRenderer::_shaders.shadowMap;

    shader.Use();
    shader.SetFloat("far_plane", SHADOW_FAR_PLANE);
    glDepthMask(true);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);

    for (int i = 0; i < Scene::_lights.size(); i++) {

        bool skip = false;

        if (!Scene::_lights[i].isDirty) {
            skip = true;
        }

        if (skip) {
            continue;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, OpenGLRenderer::_shadowMaps[i]._ID);
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
        shader.SetMat4("shadowMatrices[0]", projectionTransforms[0]);
        shader.SetMat4("shadowMatrices[1]", projectionTransforms[1]);
        shader.SetMat4("shadowMatrices[2]", projectionTransforms[2]);
        shader.SetMat4("shadowMatrices[3]", projectionTransforms[3]);
        shader.SetMat4("shadowMatrices[4]", projectionTransforms[4]);
        shader.SetMat4("shadowMatrices[5]", projectionTransforms[5]);
        shader.SetVec3("lightPosition", position);
        shader.SetMat4("model", glm::mat4(1));
        //DrawShadowMapScene(_shaders.shadowMap);
        MultiDrawIndirect(renderData.shadowMapGeometryDrawInfo.commands, OpenGLBackEnd::GetVertexDataVAO());
    }
}

/*

 █  █ █▀▀ █▀▀ █▀▀█ 　 ▀█▀ █▀▀█ ▀▀█▀▀ █▀▀ █▀▀█ █▀▀ █▀▀█ █▀▀ █▀▀
 █  █ ▀▀█ █▀▀ █▄▄▀ 　  █  █  █   █   █▀▀ █▄▄▀ █▀▀ █▄▄█ █   █▀▀
 █▄▄█ ▀▀▀ ▀▀▀ ▀─▀▀ 　 ▀▀▀ ▀  ▀   ▀   ▀▀▀ ▀ ▀▀ ▀   ▀  ▀ ▀▀▀ ▀▀▀  */

void RenderUI(std::vector<RenderItem2D>& renderItems, GLFrameBuffer& frameBuffer, bool clearScreen) {

    //GLFrameBuffer& presentBuffer = OpenGLRenderer::_frameBuffers.gBuffer;
    //ViewportInfo viewportInfo = RendererUtil::CreateViewportInfo(playerIndex, Game::GetSplitscreenMode(), frameBuffer.GetWidth(), frameBuffer.GetHeight());
    //presentBuffer.Bind();
    frameBuffer.Bind();
    frameBuffer.SetViewport();
    //SetViewport(viewportInfo);

    // GL State
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    if (clearScreen) {
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    // Feed instance data to GPU
    OpenGLRenderer::_shaders.UI.Use();
    glNamedBufferSubData(OpenGLRenderer::_ssbos.renderItems2D, 0, renderItems.size() * sizeof(RenderItem2D), &renderItems[0]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, OpenGLRenderer::_ssbos.renderItems2D);

    // Draw instanced
    Mesh* mesh = AssetManager::GetQuadMesh();
    glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
    glDrawElementsInstancedBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), renderItems.size(), mesh->baseVertex);
}

/*

    █▀▀▄ █▀▀ █▀▀█ █  █ █▀▀▀ 　 █▀▀█ █▀▀█ █▀▀ █▀▀
    █  █ █▀▀ █▀▀▄ █  █ █ ▀█ 　 █▄▄█ █▄▄█ ▀▀█ ▀▀█
    █▄▄▀ ▀▀▀ ▀▀▀▀ ▀▀▀▀ ▀▀▀▀ 　 ▀    ▀  ▀ ▀▀▀ ▀▀▀  */

void DebugPass(RenderData& renderData) {

    OpenGLDetachedMesh& linesMesh = renderData.debugLinesMesh->GetGLMesh();
    OpenGLDetachedMesh& pointsMesh = renderData.debugPointsMesh->GetGLMesh();

    // Render target
    GLFrameBuffer& presentBuffer = OpenGLRenderer::_frameBuffers.present;
    ViewportInfo viewportInfo = RendererUtil::CreateViewportInfo(0, Game::GetSplitscreenMode(), presentBuffer.GetWidth(), presentBuffer.GetHeight());
    BindFrameBuffer(presentBuffer);
    SetViewport(viewportInfo);

    Shader& shader = OpenGLRenderer::_shaders.debugSolidColor;
    shader.Use();
    shader.SetMat4("projection", renderData.cameraData[0].projection);
    shader.SetMat4("view", renderData.cameraData[0].view);

    glDisable(GL_DEPTH_TEST);

    // Draw lines
    glBindVertexArray(linesMesh.GetVAO());
    glDrawElements(GL_LINES, linesMesh.GetIndexCount(), GL_UNSIGNED_INT, 0);

    // Draw points
    glPointSize(4);
    glBindVertexArray(pointsMesh.GetVAO());
    glDrawElements(GL_POINTS, pointsMesh.GetIndexCount(), GL_UNSIGNED_INT, 0);

    // Point cloud
    if (renderData.renderMode == RenderMode::POINT_CLOUD_PROPAGATION_GRID ||
        renderData.renderMode == RenderMode::POINT_CLOUD) {
        Shader& pointCloudDebugShader = OpenGLRenderer::_shaders.debugPointCloud;
        pointCloudDebugShader.Use();
        pointCloudDebugShader.SetMat4("projection", renderData.cameraData[0].projection);
        pointCloudDebugShader.SetMat4("view", renderData.cameraData[0].view);
        glBindVertexArray(OpenGLBackEnd::GetPointCloudVAO());
        glDrawArrays(GL_POINTS, 0, GlobalIllumination::GetPointCloud().size());
    }
}

void DebugPassProbePass(RenderData& renderData) {

    static int cubeMeshIndex = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Cube"))->GetMeshIndices()[0];

    // Render target
    GLFrameBuffer& gBuffer = OpenGLRenderer::_frameBuffers.gBuffer;
    ViewportInfo viewportInfo = RendererUtil::CreateViewportInfo(0, Game::GetSplitscreenMode(), gBuffer.GetWidth(), gBuffer.GetHeight());
    BindFrameBuffer(gBuffer);
    SetViewport(viewportInfo);

    Shader& shader = OpenGLRenderer::_shaders.debugProbes;
    shader.Use();
    shader.SetMat4("projection", renderData.cameraData[0].projection);
    shader.SetMat4("view", renderData.cameraData[0].view);
    shader.SetFloat("probeSpacing", PROBE_SPACING);

    Mesh* mesh = AssetManager::GetMeshByIndex(cubeMeshIndex);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDrawBuffer(gBuffer.GetColorAttachmentSlotByName("FinalLighting"));
    glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());

    // Draw prorogation grid
    if (renderData.renderMode == RenderMode::POINT_CLOUD_PROPAGATION_GRID ||
        renderData.renderMode == RenderMode::PROPAGATION_GRID) {
        LightVolume* lightVolume = GlobalIllumination::GetLightVolumeByIndex(0);
        if (lightVolume) {
            shader.SetInt("probeSpaceWidth", lightVolume->GetProbeSpaceWidth());
            shader.SetInt("probeSpaceHeight", lightVolume->GetProbeSpaceHeight());
            shader.SetInt("probeSpaceDepth", lightVolume->GetProbeSpaceDepth());
            glDrawElementsInstancedBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), lightVolume->GetProbeCount(), mesh->baseVertex);
        }
    }
    glDepthMask(GL_TRUE);
}

void ComputeSkin(RenderData& renderData) {

    ComputeShader& computeShader = OpenGLRenderer::_shaders.computeSkinning;
    computeShader.Use();

    // Calculate total amount of vertices to skin and allocate space
    int totalVertexCount = 0;
    for (AnimatedGameObject* animatedGameObject : renderData.animatedGameObjectsToSkin) {
        SkinnedModel* skinnedModel = animatedGameObject->_skinnedModel;
        for (int i = 0; i < skinnedModel->GetMeshCount(); i++) {
            int meshindex = skinnedModel->GetMeshIndices()[i];
            SkinnedMesh* mesh = AssetManager::GetSkinnedMeshByIndex(meshindex);
            totalVertexCount += mesh->vertexCount;
        }
    }
    OpenGLBackEnd::AllocateSkinnedVertexBufferSpace(totalVertexCount);

    // Skin
    int j = 0;
    int baseOutputVertex = 0;
    for (AnimatedGameObject* animatedGameObject : renderData.animatedGameObjectsToSkin) {

        SkinnedModel* skinnedModel = animatedGameObject->_skinnedModel;

        for (int i = 0; i < skinnedModel->GetMeshCount(); i++) {
            int meshindex = skinnedModel->GetMeshIndices()[i];
            SkinnedMesh* mesh = AssetManager::GetSkinnedMeshByIndex(meshindex);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, OpenGLBackEnd::GetSkinnedVertexDataVBO());
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, OpenGLBackEnd::GetWeightedVertexDataVBO());
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, OpenGLRenderer::_ssbos.skinningTransforms);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, OpenGLRenderer::_ssbos.baseAnimatedTransformIndices);
            computeShader.SetInt("vertexCount", mesh->vertexCount);
            computeShader.SetInt("baseInputVertex", mesh->baseVertexGlobal);
            computeShader.SetInt("baseOutputVertex", baseOutputVertex);
            computeShader.SetInt("animatedGameObjectIndex", j);
            glDispatchCompute(mesh->vertexCount, 1, 1);
            baseOutputVertex += mesh->vertexCount;
        }
        j++;
    }
}

/*

 █▀▀█ █▀▀ █▀▀█ █▀▄▀█ █▀▀ ▀▀█▀▀ █▀▀█ █  █ 　 █▀▀█ █▀▀█ █▀▀ █▀▀
 █ ▄▄ █▀▀ █  █ █ ▀ █ █▀▀   █   █▄▄▀ ▀▀▀█ 　 █▄▄█ █▄▄█ ▀▀█ ▀▀█
 █▄▄█ ▀▀▀ ▀▀▀▀ ▀   ▀ ▀▀▀   ▀   ▀ ▀▀ ▀▀▀▀ 　 ▀    ▀  ▀ ▀▀▀ ▀▀▀  */

void GeometryPass(RenderData& renderData) {

    // Render target
    GLFrameBuffer& gBuffer = OpenGLRenderer::_frameBuffers.gBuffer;
    BindFrameBuffer(gBuffer);

    unsigned int attachments[7] = {
        GL_COLOR_ATTACHMENT0,
        GL_COLOR_ATTACHMENT1,
        GL_COLOR_ATTACHMENT2,
        GL_COLOR_ATTACHMENT6,
        GL_COLOR_ATTACHMENT7,
        gBuffer.GetColorAttachmentSlotByName("Glass"),
        gBuffer.GetColorAttachmentSlotByName("EmissiveMask") };
    glDrawBuffers(7, attachments);

    // GL state
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // Draw mesh
    Shader& gBufferShader = OpenGLRenderer::_shaders.geometry;
    gBufferShader.Use();
    for (int i = 0; i < renderData.playerCount; i++) {

        ViewportInfo viewportInfo = RendererUtil::CreateViewportInfo(i, Game::GetSplitscreenMode(), gBuffer.GetWidth(), gBuffer.GetHeight());
        SetViewport(viewportInfo);
        glViewport(viewportInfo.xOffset, viewportInfo.yOffset, viewportInfo.width, viewportInfo.height);
        gBufferShader.SetMat4("projection", renderData.cameraData[i].projection);
        gBufferShader.SetMat4("view", renderData.cameraData[i].view);
        gBufferShader.SetInt("playerIndex", i);
        MultiDrawIndirect(renderData.geometryIndirectDrawInfo.playerDrawCommands[i], OpenGLBackEnd::GetVertexDataVAO());
    }


    // Skinned mesh
    glBindVertexArray(OpenGLBackEnd::GetSkinnedVertexDataVAO());
    glBindBuffer(GL_ARRAY_BUFFER, OpenGLBackEnd::GetSkinnedVertexDataVBO());
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, OpenGLBackEnd::GetWeightedVertexDataEBO());

    Shader& newDebugShader = OpenGLRenderer::_shaders.newDebugShader;
    newDebugShader.Use();

    int k = 0;
    for (int i = 0; i < renderData.playerCount; i++) {

        ViewportInfo viewportInfo = RendererUtil::CreateViewportInfo(i, Game::GetSplitscreenMode(), gBuffer.GetWidth(), gBuffer.GetHeight());
        SetViewport(viewportInfo);

        newDebugShader.SetInt("playerIndex", i);
        newDebugShader.SetMat4("projection", renderData.cameraData[i].projection);
        newDebugShader.SetMat4("view", renderData.cameraData[i].view);

        for (SkinnedRenderItem& skinnedRenderItem : renderData.skinnedRenderItems[i]) {
            SkinnedMesh* mesh = AssetManager::GetSkinnedMeshByIndex(skinnedRenderItem.originalMeshIndex);
            newDebugShader.SetMat4("model", skinnedRenderItem.modelMatrix);
            newDebugShader.SetInt("renderItemIndex", k);
            glDrawElementsInstancedBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), 1, skinnedRenderItem.baseVertex);
            k++;
        }
    }
    glBindVertexArray(0);

}

void DrawVATBlood(RenderData& renderData) {

    // Render target
    GLFrameBuffer& gBuffer = OpenGLRenderer::_frameBuffers.gBuffer;
    BindFrameBuffer(gBuffer);

    // GL State
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glCullFace(GL_BACK);

    Shader& shader = OpenGLRenderer::_shaders.vatBlood;
    shader.Use();

    for (int i = 0; i < renderData.playerCount; i++) {

        shader.SetInt("playerIndex", i);
        ViewportInfo viewportInfo = RendererUtil::CreateViewportInfo(i, Game::GetSplitscreenMode(), gBuffer.GetWidth(), gBuffer.GetHeight());
        SetViewport(viewportInfo);
        MultiDrawIndirect(renderData.bloodVATDrawInfo.commands, OpenGLBackEnd::GetVertexDataVAO());
    }
}

void DrawBloodDecals(RenderData& renderData) {

    // Render target
    GLFrameBuffer& gBuffer = OpenGLRenderer::_frameBuffers.gBuffer;
    BindFrameBuffer(gBuffer);

    // GL state
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    // Draw
    Shader& shader = OpenGLRenderer::_shaders.bloodDecals;
    shader.Use();

    for (int i = 0; i < renderData.playerCount; i++) {

        shader.SetInt("playerIndex", i);
        ViewportInfo viewportInfo = RendererUtil::CreateViewportInfo(i, Game::GetSplitscreenMode(), gBuffer.GetWidth(), gBuffer.GetHeight());
        SetViewport(viewportInfo);
        MultiDrawIndirect(renderData.bloodDecalDrawInfo.commands, OpenGLBackEnd::GetVertexDataVAO());
    }

    // Cleanup
    glDepthMask(GL_TRUE);
}

void DrawBulletDecals(RenderData& renderData) {

    // Render target
    GLFrameBuffer& gBuffer = OpenGLRenderer::_frameBuffers.gBuffer;
    BindFrameBuffer(gBuffer);

    unsigned int attachments[5] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT6, GL_COLOR_ATTACHMENT7 };
    glDrawBuffers(5, attachments);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // GL state
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // Draw mesh
    Shader& shader = OpenGLRenderer::_shaders.decals;
    shader.Use();

    for (int i = 0; i < renderData.playerCount; i++) {

        shader.SetInt("playerIndex", i);
        ViewportInfo viewportInfo = RendererUtil::CreateViewportInfo(i, Game::GetSplitscreenMode(), gBuffer.GetWidth(), gBuffer.GetHeight());
        SetViewport(viewportInfo);
        MultiDrawIndirect(renderData.bulletHoleDecalDrawInfo.commands, OpenGLBackEnd::GetVertexDataVAO());
    }
}


/*

█    █ █▀▀▀ █  █ ▀▀█▀▀ █ █▀▀█ █▀▀▀ 　 █▀▀█ █▀▀█ █▀▀ █▀▀
█    █ █ ▀█ █▀▀█   █   █ █  █ █ ▀█ 　 █▄▄█ █▄▄█ ▀▀█ ▀▀█
█▄▄█ ▀ ▀▀▀▀ ▀  ▀   ▀   ▀ ▀  ▀ ▀▀▀▀ 　 ▀    ▀  ▀ ▀▀▀ ▀▀▀  */

void LightingPass(RenderData& renderData) {

    GLFrameBuffer& gBuffer = OpenGLRenderer::_frameBuffers.gBuffer;
    gBuffer.Bind();
    gBuffer.SetViewport();
    glDrawBuffer(gBuffer.GetColorAttachmentSlotByName("FinalLighting"));

    Shader& shader = OpenGLRenderer::_shaders.lighting;
    shader.Use();

    shader.SetMat4("inverseProjection", renderData.cameraData[0].projectionInverse);
    shader.SetMat4("inverseView", renderData.cameraData[0].viewInverse);
    shader.SetFloat("clipSpaceXMin", renderData.cameraData[0].clipSpaceXMin);
    shader.SetFloat("clipSpaceXMax", renderData.cameraData[0].clipSpaceXMax);
    shader.SetFloat("clipSpaceYMin", renderData.cameraData[0].clipSpaceYMin);
    shader.SetFloat("clipSpaceYMax", renderData.cameraData[0].clipSpaceYMax);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gBuffer.GetColorAttachmentHandleByName("BaseColor"));
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gBuffer.GetColorAttachmentHandleByName("Normal"));
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gBuffer.GetColorAttachmentHandleByName("RMA"));
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, gBuffer.GetDepthAttachmentHandle());

    for (int i = 0; i < renderData.lights.size(); i++) {
        glActiveTexture(GL_TEXTURE5 + i);
        glBindTexture(GL_TEXTURE_CUBE_MAP, OpenGLRenderer::_shadowMaps[i]._depthTexture);
    }

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());



    int meshIndex = AssetManager::GetQuadMeshIndex();
   /* if (Game::GetSplitscreenMode() == SplitscreenMode::TWO_PLAYER) {
        if (0 == 0) {
            meshIndex = AssetManager::GetQuadMeshIndexSplitscreenTop();
        }
        if (0 == 1) {
            meshIndex = AssetManager::GetQuadMeshIndexSplitscreenBottom();
        }
    }
    if (Game::GetSplitscreenMode() == SplitscreenMode::FOUR_PLAYER) {
        if (0 == 0) {
            meshIndex = AssetManager::GetQuadMeshIndexQuadscreenTopLeft();
        }
        if (0 == 1) {
            meshIndex = AssetManager::GetQuadMeshIndexQuadscreenTopRight();
        }
        if (0 == 2) {
            meshIndex = AssetManager::GetQuadMeshIndexQuadscreenBottomLeft();
        }
        if (0 == 3) {
            meshIndex = AssetManager::GetQuadMeshIndexQuadscreenBottomRight();
        }
    }*/
    Mesh* mesh = AssetManager::GetMeshByIndex(meshIndex);

    glDrawElementsBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), mesh->baseVertex);
}

void GlassPass(RenderData& renderData) {

    // Render target
    GLFrameBuffer& gBuffer = OpenGLRenderer::_frameBuffers.gBuffer;
    BindFrameBuffer(gBuffer);

    glDrawBuffer(gBuffer.GetColorAttachmentSlotByName("Glass"));
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    Shader& shader = OpenGLRenderer::_shaders.glass;
    shader.Use();



    for (int i = 0; i < renderData.playerCount; i++) {

        shader.SetInt("playerIndex", i);
        ViewportInfo viewportInfo = RendererUtil::CreateViewportInfo(i, Game::GetSplitscreenMode(), gBuffer.GetWidth(), gBuffer.GetHeight());
        SetViewport(viewportInfo);

       // shader.SetMat4("projection", renderData.cameraData[0].projection);
       // shader.SetMat4("view", renderData.cameraData[0].view);
       // shader.SetVec3("viewPos", renderData.cameraData[0].viewInverse[3]);

        MultiDrawIndirect(renderData.glassDrawInfo.commands, OpenGLBackEnd::GetVertexDataVAO());

    }


    // Composite that render back into the lighting texture
    gBuffer.SetViewport();
    ComputeShader& computeShader = OpenGLRenderer::_shaders.glassComposite;
    computeShader.Use();
    glBindImageTexture(0, gBuffer.GetColorAttachmentHandleByName("FinalLighting"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
    glBindImageTexture(1, gBuffer.GetColorAttachmentHandleByName("Glass"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
    glDispatchCompute(gBuffer.GetWidth() / 16, gBuffer.GetHeight() / 4, 1);
}


void MuzzleFlashPass(RenderData& renderData) {

    // Render target
    GLFrameBuffer& gBuffer = OpenGLRenderer::_frameBuffers.gBuffer;
  //  ViewportInfo viewportInfo = RendererUtil::CreateViewportInfo(0, Game::GetSplitscreenMode(), gBuffer.GetWidth(), gBuffer.GetHeight());
    BindFrameBuffer(gBuffer);
    //SetViewport(viewportInfo);

    glDrawBuffer(gBuffer.GetColorAttachmentSlotByName("FinalLighting"));

    glEnable(GL_DEPTH_TEST);

    Shader& animatedQuadShader = OpenGLRenderer::_shaders.flipBook;
    animatedQuadShader.Use();
    //animatedQuadShader.SetMat4("u_MatrixProjection", renderData.cameraData[0].projection);
    //animatedQuadShader.SetMat4("u_MatrixView", renderData.cameraData[0].view);
    //animatedQuadShader.SetMat4("u_MatrixWorld", renderData.muzzleFlashData.modelMatrix);


    for (int i = 0; i < renderData.playerCount; i++) {

        animatedQuadShader.SetInt("playerIndex", i);
        ViewportInfo viewportInfo = RendererUtil::CreateViewportInfo(i, Game::GetSplitscreenMode(), gBuffer.GetWidth(), gBuffer.GetHeight());
        SetViewport(viewportInfo);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glActiveTexture(GL_TEXTURE0);
        AssetManager::GetTextureByName("MuzzleFlash_ALB")->GetGLTexture().Bind(0);

        Mesh* mesh = AssetManager::GetQuadMesh();
        glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
        glDrawElementsInstancedBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), 1, mesh->baseVertex);

    }


    glDisable(GL_BLEND);
}


void PostProcessingPass(RenderData& renderData) {

    GLFrameBuffer& gBuffer = OpenGLRenderer::_frameBuffers.gBuffer;
    ComputeShader& computeShader = OpenGLRenderer::_shaders.postProcessing;
    computeShader.Use();
    glBindImageTexture(0, gBuffer.GetColorAttachmentHandleByName("FinalLighting"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
    glBindImageTexture(1, gBuffer.GetColorAttachmentHandleByName("Normal"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
    glDispatchCompute(gBuffer.GetWidth() / 16, gBuffer.GetHeight() / 4, 1);
}


void OpenGLRenderer::UpdatePointCloud() {

    int pointCount = GlobalIllumination::GetPointCloud().size();
    int invocationCount = std::ceil(pointCount / 64.0f);

    ComputeShader& shader = OpenGLRenderer::_shaders.pointCloudDirectLigthing;
    shader.Use();
    shader.SetInt("pointCount", pointCount);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, OpenGLRenderer::_ssbos.lights);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, OpenGLBackEnd::GetPointCloudVBO());

    glDispatchCompute(invocationCount, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void OpenGLRenderer::PresentFinalImage() {
    BlitDstCoords blitDstCoords;
    blitDstCoords.dstX0 = 0;
    blitDstCoords.dstY0 = 0;
    blitDstCoords.dstX1 = BackEnd::GetCurrentWindowWidth();
    blitDstCoords.dstY1 = BackEnd::GetCurrentWindowHeight();
    BlitPlayerPresentTargetToDefaultFrameBuffer(&_frameBuffers.present, 0, "Color", "", GL_COLOR_BUFFER_BIT, GL_NEAREST, blitDstCoords);
   // BlitPlayerPresentTargetToDefaultFrameBuffer(&_frameBuffers.gBuffer, 0, "FinalLighting", "", GL_COLOR_BUFFER_BIT, GL_NEAREST, blitDstCoords);
    //BlitPlayerPresentTargetToDefaultFrameBuffer(&_frameBuffers.gBuffer, 0, "FinalLighting", "", GL_COLOR_BUFFER_BIT, GL_NEAREST, blitDstCoords);
}