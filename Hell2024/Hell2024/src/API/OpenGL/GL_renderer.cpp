#include "GL_renderer.h"
#include "GL_assetManager.h"
#include "GL_backEnd.h"
#include "Types/GL_gBuffer.h"
#include "Types/GL_shader.h"
#include "Types/GL_shadowMap.h"
#include "Types/GL_frameBuffer.hpp"
#include "../../BackEnd/BackEnd.h"
#include "../../Renderer/RendererCommon.h"
#include "../../Renderer/TextBlitter.h"
#include "../../Core/Scene.h"
#include "../../Core/AssetManager.h"
#include "../../Core/Game.h"

namespace OpenGLRenderer {

    struct FrameBuffers {
        GLFrameBuffer loadingScreen;
        GLFrameBuffer debugMenu;
        GLFrameBuffer present;
        GLFrameBuffer gBuffer;
        GLFrameBuffer lighting;
        std::vector<GLFrameBuffer> blurBuffers;
    } _frameBuffers;

    struct Shaders {
        Shader geometry;
        Shader geometrySkinned;
        Shader lighting;
        Shader UI;
        Shader shadowMap;
        Shader debug;
        Shader flipBook;
        Shader decals;
        Shader glass;
        Shader horizontalBlur;
        Shader verticalBlur;
        Shader bloodDecals;
        Shader vatBlood;
        ComputeShader postProcessing;
        ComputeShader glassComposite;
        ComputeShader emissiveComposite;
    } _shaders;

    struct SSBOs {
        GLuint samplers = 0;
        GLuint renderItems2D = 0;
        GLuint animatedRenderItems3D = 0;
        GLuint animatedTransforms = 0;
        GLuint lights = 0;

        GLuint cameraData = 0;

        GLuint geometryRenderItems = 0;
        GLuint glassRenderItems = 0;
        GLuint bulletHoleDecalRenderItems = 0;
        GLuint bloodDecalRenderItems = 0;
        GLuint shadowMapGeometryRenderItems = 0;
        GLuint bloodVATRenderItems = 0;

    } _ssbos;

    GLuint _indirectBuffer = 0;
    std::vector<ShadowMap> _shadowMaps;
}

void DrawRenderItem(RenderItem3D& renderItem);


void MultiDrawIndirect(std::vector<DrawIndexedIndirectCommand>& commands, GLuint vertexArray);

void MultiDrawIndirect(std::vector<RenderItem3D>& renderItems, GLuint vertexArray);
void BlitFrameBuffer(GLFrameBuffer* src, GLFrameBuffer* dst, const char* srcName, const char* dstName, GLbitfield mask, GLenum filter);


void BlitPlayerPresentTargetToDefaultFrameBuffer(GLFrameBuffer* src, GLFrameBuffer* dst, const char* srcName, const char* dstName, GLbitfield mask, GLenum filter, BlitDstCoords& blitDstCoords);

void ClearRenderTargets();
void LightingPass(RenderData& renderData);
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
void EmissivePass(RenderData& renderData);

void OpenGLRenderer::HotloadShaders() {

    std::cout << "Hotloading shaders...\n";

    _shaders.UI.Load("GL_ui.vert", "GL_ui.frag");
    _shaders.geometry.Load("GL_gbuffer.vert", "GL_gbuffer.frag");
    _shaders.geometrySkinned.Load("GL_gbufferSkinned.vert", "GL_gbufferSkinned.frag");
    _shaders.lighting.Load("GL_lighting.vert", "GL_lighting.frag");
    _shaders.debug.Load("GL_debug.vert", "GL_debug.frag");
    _shaders.shadowMap.Load("GL_shadowMap.vert", "GL_shadowMap.frag", "GL_shadowMap.geom");
    _shaders.flipBook.Load("GL_flipBook.vert", "GL_flipBook.frag");
    _shaders.glass.Load("GL_glass.vert", "GL_glass.frag");
    _shaders.decals.Load("GL_decals.vert", "GL_decals.frag");
    _shaders.postProcessing.LoadOLD("res/shaders/OpenGL/GL_postProcessing.comp");
    _shaders.glassComposite.LoadOLD("res/shaders/OpenGL/GL_glassComposite.comp");
    _shaders.horizontalBlur.Load("GL_blurHorizontal.vert", "GL_blur.frag");
    _shaders.verticalBlur.Load("GL_blurVertical.vert", "GL_blur.frag");
    _shaders.bloodDecals.Load("GL_bloodDecals.vert", "GL_bloodDecals.frag");
    _shaders.vatBlood.Load("GL_bloodVAT.vert", "GL_bloodVAT.frag");
    _shaders.emissiveComposite.LoadOLD("res/shaders/OpenGL/GL_emissiveComposite.comp");

}

void OpenGLRenderer::CreatePlayerRenderTargets(int presentWidth, int presentHeight) {

    if (_frameBuffers.present.GetHandle() != 0) {
        _frameBuffers.present.CleanUp();
    }
    if (_frameBuffers.gBuffer.GetHandle() != 0) {
        _frameBuffers.gBuffer.CleanUp();
    }
    //if (_frameBuffers.lighting.GetHandle() != 0) {
       // _frameBuffers.lighting.CleanUp();
    //}

    _frameBuffers.present.Create("Present", presentWidth, presentHeight);
    _frameBuffers.present.CreateAttachment("Color", GL_RGBA8);

    _frameBuffers.gBuffer.Create("GBuffer", presentWidth * 2, presentHeight * 2);
    _frameBuffers.gBuffer.CreateAttachment("BaseColor", GL_RGBA8);
    _frameBuffers.gBuffer.CreateAttachment("Normal", GL_RGBA16F);
    _frameBuffers.gBuffer.CreateAttachment("RMA", GL_RGBA8);
    _frameBuffers.gBuffer.CreateAttachment("FinalLighting", GL_RGBA8);
    _frameBuffers.gBuffer.CreateAttachment("Glass", GL_RGBA8);
    _frameBuffers.gBuffer.CreateAttachment("EmissiveMask", GL_RGBA8);
    _frameBuffers.gBuffer.CreateDepthAttachment(GL_DEPTH32F_STENCIL8);


    // Create blur buffer frame buffers

    int blurBufferWidth = presentWidth * 2;
    int blurBufferHeight = presentHeight * 2;

    for (int i = 0; i < 4; i++) {
        GLFrameBuffer& frameBuffer = _frameBuffers.blurBuffers.emplace_back();
        std::string frameBufferName = "BlurBuffer" + std::to_string(i);
        frameBuffer.Create(frameBufferName.c_str(), blurBufferWidth, blurBufferHeight);
        frameBuffer.CreateAttachment("ColorA", GL_RGBA8);
        frameBuffer.CreateAttachment("ColorB", GL_RGBA8);
        blurBufferWidth *= 0.5f;
        blurBufferHeight *= 0.5f;
    }

    

    //_frameBuffers.lighting.Create("Lighting", presentWidth * 2, presentHeight * 2);
    //_frameBuffers.lighting.CreateAttachment("Color", GL_RGBA8);

    std::cout << "Resizing render targets: " << presentWidth << " x " << presentHeight << "\n";
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

    CreatePlayerRenderTargets(PRESENT_WIDTH, PRESENT_HEIGHT);

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

    glCreateBuffers(1, &_ssbos.cameraData);
    glNamedBufferStorage(_ssbos.cameraData, sizeof(CameraData), NULL, GL_DYNAMIC_STORAGE_BIT);

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
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, _ssbos.samplers);
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
            commands[i].baseVertex = mesh->baseVertex;
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

void DrawQuad() {
    Mesh* mesh = AssetManager::GetQuadMesh();
    glDrawElementsBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), mesh->baseVertex);
}

void DownScaleGBuffer() {
    GLFrameBuffer& gBuffer = OpenGLRenderer::_frameBuffers.gBuffer;
    GLFrameBuffer& presentBuffer = OpenGLRenderer::_frameBuffers.present;
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


void OpenGLRenderer::RenderLoadingScreen(std::vector<RenderItem2D>& renderItems) {

    RenderUI(renderItems, _frameBuffers.loadingScreen, true);
    BlitFrameBuffer(&_frameBuffers.loadingScreen, 0, "Color", "", GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

void UploadSSBOsGPU(RenderData& renderData) {

    glNamedBufferSubData(OpenGLRenderer::_ssbos.geometryRenderItems, 0, renderData.geometryDrawInfo.renderItems.size() * sizeof(RenderItem3D), &renderData.geometryDrawInfo.renderItems[0]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, OpenGLRenderer::_ssbos.geometryRenderItems);

    glNamedBufferSubData(OpenGLRenderer::_ssbos.lights, 0, renderData.lights.size() * sizeof(GPULight), &renderData.lights[0]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, OpenGLRenderer::_ssbos.lights);

    glNamedBufferSubData(OpenGLRenderer::_ssbos.cameraData, 0, sizeof(CameraData), &renderData.cameraData);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, OpenGLRenderer::_ssbos.cameraData);

    glNamedBufferSubData(OpenGLRenderer::_ssbos.animatedTransforms, 0, (*renderData.animatedTransforms).size() * sizeof(glm::mat4), &(*renderData.animatedTransforms)[0]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, OpenGLRenderer::_ssbos.animatedTransforms);

    glNamedBufferSubData(OpenGLRenderer::_ssbos.animatedRenderItems3D, 0, renderData.animatedRenderItems3D.size() * sizeof(RenderItem3D), &renderData.animatedRenderItems3D[0]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, OpenGLRenderer::_ssbos.animatedRenderItems3D);

    glNamedBufferSubData(OpenGLRenderer::_ssbos.glassRenderItems, 0, renderData.glassDrawInfo.renderItems.size() * sizeof(RenderItem3D), &renderData.glassDrawInfo.renderItems[0]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, OpenGLRenderer::_ssbos.glassRenderItems);

    glNamedBufferSubData(OpenGLRenderer::_ssbos.shadowMapGeometryRenderItems, 0, renderData.shadowMapGeometryDrawInfo.renderItems.size() * sizeof(RenderItem3D), &renderData.shadowMapGeometryDrawInfo.renderItems[0]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 12, OpenGLRenderer::_ssbos.shadowMapGeometryRenderItems);

    glNamedBufferSubData(OpenGLRenderer::_ssbos.bulletHoleDecalRenderItems, 0, renderData.bulletHoleDecalDrawInfo.renderItems.size() * sizeof(RenderItem3D), &renderData.bulletHoleDecalDrawInfo.renderItems[0]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 13, OpenGLRenderer::_ssbos.bulletHoleDecalRenderItems);

    glNamedBufferSubData(OpenGLRenderer::_ssbos.bloodDecalRenderItems, 0, renderData.bloodDecalDrawInfo.renderItems.size() * sizeof(RenderItem3D), &renderData.bloodDecalDrawInfo.renderItems[0]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 14, OpenGLRenderer::_ssbos.bloodDecalRenderItems);

    glNamedBufferSubData(OpenGLRenderer::_ssbos.bloodVATRenderItems, 0, renderData.bloodVATDrawInfo.renderItems.size() * sizeof(RenderItem3D), &renderData.bloodVATDrawInfo.renderItems[0]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 15, OpenGLRenderer::_ssbos.bloodVATRenderItems);
}

void OpenGLRenderer::RenderGame(RenderData& renderData) {

    GLFrameBuffer& gBuffer = _frameBuffers.gBuffer;
    GLFrameBuffer& present = _frameBuffers.present;

    UploadSSBOsGPU(renderData);
    ClearRenderTargets();

    RenderShadowMapss(renderData);
    GeometryPass(renderData);
    DrawVATBlood(renderData);
    DrawBloodDecals(renderData);
    DrawBulletDecals(renderData);
    LightingPass(renderData);
    GlassPass(renderData);
    EmissivePass(renderData);
    MuzzleFlashPass(renderData);
    PostProcessingPass(renderData);
    RenderUI(renderData.renderItems2DHiRes, _frameBuffers.gBuffer, false);
    DownScaleGBuffer();
    DebugPass(renderData);
    RenderUI(renderData.renderItems2D, _frameBuffers.present, false);

    BlitPlayerPresentTargetToDefaultFrameBuffer(&_frameBuffers.present, 0, "Color", "", GL_COLOR_BUFFER_BIT, GL_NEAREST, renderData.blitDstCoords);
}


void ClearRenderTargets() {

    // Clear blur buffers
    for (GLFrameBuffer& blurBuffer : OpenGLRenderer::_frameBuffers.blurBuffers) {
        blurBuffer.Bind();
        blurBuffer.SetViewport();
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    }

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


void EmissivePass(RenderData& renderData) {

    GLFrameBuffer& gBuffer = OpenGLRenderer::_frameBuffers.gBuffer;
    std::vector<GLFrameBuffer>& blurBuffers = OpenGLRenderer::_frameBuffers.blurBuffers;
    Shader& horizontalBlurShader = OpenGLRenderer::_shaders.horizontalBlur;
    Shader& verticalBlurShader = OpenGLRenderer::_shaders.verticalBlur;

    blurBuffers[0].Bind();
    blurBuffers[0].SetViewport();

    // First round blur (horizontal)
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gBuffer.GetColorAttachmentHandleByName("EmissiveMask")); 
    horizontalBlurShader.Use();
    horizontalBlurShader.SetFloat("targetWidth", blurBuffers[0].GetWidth());
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
    DrawQuad();

    // First round blur (vertical)    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, blurBuffers[0].GetColorAttachmentHandleByName("ColorA"));
    glDrawBuffer(GL_COLOR_ATTACHMENT1);
    verticalBlurShader.Use();
    verticalBlurShader.SetFloat("targetHeight", blurBuffers[0].GetHeight());
    DrawQuad();    


    // Second round blur (horizontal)
    blurBuffers[1].Bind();
    blurBuffers[1].SetViewport();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, blurBuffers[0].GetColorAttachmentHandleByName("ColorB"));
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    horizontalBlurShader.Use();
    horizontalBlurShader.SetFloat("targetWidth", blurBuffers[1].GetWidth());
    DrawQuad();

    // Second round blur (vertical)
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, blurBuffers[0].GetColorAttachmentHandleByName("ColorA"));
    glDrawBuffer(GL_COLOR_ATTACHMENT1);
    verticalBlurShader.Use();
    verticalBlurShader.SetFloat("targetHeight", blurBuffers[1].GetHeight());
    DrawQuad();

    // Third round blur (horizontal)
    blurBuffers[2].Bind();
    blurBuffers[2].SetViewport();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, blurBuffers[1].GetColorAttachmentHandleByName("ColorB"));
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    horizontalBlurShader.Use();
    horizontalBlurShader.SetFloat("targetWidth", blurBuffers[2].GetWidth());
    DrawQuad();

    // Third round blur (vertical)
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, blurBuffers[2].GetColorAttachmentHandleByName("ColorA"));
    glDrawBuffer(GL_COLOR_ATTACHMENT1);
    verticalBlurShader.Use();
    verticalBlurShader.SetFloat("targetHeight", blurBuffers[2].GetHeight());
    DrawQuad();

    // Forth round blur (horizontal)
    blurBuffers[3].Bind();
    blurBuffers[3].SetViewport();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, blurBuffers[2].GetColorAttachmentHandleByName("ColorB"));
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    horizontalBlurShader.Use();
    horizontalBlurShader.SetFloat("targetWidth", blurBuffers[3].GetWidth());
    DrawQuad();

    // Forth round blur (vertical)
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, blurBuffers[3].GetColorAttachmentHandleByName("ColorA"));
    glDrawBuffer(GL_COLOR_ATTACHMENT1);
    verticalBlurShader.Use();
    verticalBlurShader.SetFloat("targetHeight", blurBuffers[3].GetHeight());
    DrawQuad();

    // Composite those blurred textures into the main lighting image
    ComputeShader& shader = OpenGLRenderer::_shaders.emissiveComposite;
    shader.Use();
    shader.SetFloat("screenWidth", renderData.cameraData.viewportWidth);
    shader.SetFloat("screenHeight", renderData.cameraData.viewportHeight);
    glBindImageTexture(0, gBuffer.GetColorAttachmentHandleByName("FinalLighting"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, blurBuffers[0].GetColorAttachmentHandleByName("ColorB"));
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, blurBuffers[1].GetColorAttachmentHandleByName("ColorB"));
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, blurBuffers[2].GetColorAttachmentHandleByName("ColorB"));
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, blurBuffers[3].GetColorAttachmentHandleByName("ColorB"));
    glDispatchCompute(gBuffer.GetWidth() / 16, gBuffer.GetHeight() / 4, 1);
}

void RenderShadowMapss(RenderData& renderData) {

    if (renderData.playerIndex != 0) {
        return;
    }

    Shader& shader = OpenGLRenderer::_shaders.shadowMap;

    shader.Use();
    shader.SetFloat("far_plane", SHADOW_FAR_PLANE);
    shader.SetMat4("model", glm::mat4(1));
    shader.SetMat4("model", glm::mat4(1));
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

    // Render Target
    frameBuffer.Bind();
    frameBuffer.SetViewport();

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
    
   // BlitFrameBuffer(&frameBuffer, 0, "Color", "", GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

/*

    █▀▀▄ █▀▀ █▀▀█ █  █ █▀▀▀ 　 █▀▀█ █▀▀█ █▀▀ █▀▀
    █  █ █▀▀ █▀▀▄ █  █ █ ▀█ 　 █▄▄█ █▄▄█ ▀▀█ ▀▀█
    █▄▄▀ ▀▀▀ ▀▀▀▀ ▀▀▀▀ ▀▀▀▀ 　 ▀    ▀  ▀ ▀▀▀ ▀▀▀  */

void DebugPass(RenderData& renderData) {

    OpenGLDetachedMesh& linesMesh = renderData.debugLinesMesh->GetGLMesh();
    OpenGLDetachedMesh& pointsMesh = renderData.debugPointsMesh->GetGLMesh();

    GLFrameBuffer& presentBuffer = OpenGLRenderer::_frameBuffers.present;
    presentBuffer.Bind();
    presentBuffer.SetViewport();

    // Draw mesh
    Shader& debugShader = OpenGLRenderer::_shaders.debug;
    debugShader.Use();
    debugShader.SetMat4("projection", renderData.cameraData.projection);
    debugShader.SetMat4("view", renderData.cameraData.view);

    glBindVertexArray(linesMesh.GetVAO());
    glDrawElements(GL_LINES, linesMesh.GetIndexCount(), GL_UNSIGNED_INT, 0);

    glPointSize(4);
    glBindVertexArray(pointsMesh.GetVAO());
    glDrawElements(GL_POINTS, pointsMesh.GetIndexCount(), GL_UNSIGNED_INT, 0);
}

/*

 █▀▀█ █▀▀ █▀▀█ █▀▄▀█ █▀▀ ▀▀█▀▀ █▀▀█ █  █ 　 █▀▀█ █▀▀█ █▀▀ █▀▀
 █ ▄▄ █▀▀ █  █ █ ▀ █ █▀▀   █   █▄▄▀ ▀▀▀█ 　 █▄▄█ █▄▄█ ▀▀█ ▀▀█
 █▄▄█ ▀▀▀ ▀▀▀▀ ▀   ▀ ▀▀▀   ▀   ▀ ▀▀ ▀▀▀▀ 　 ▀    ▀  ▀ ▀▀▀ ▀▀▀  */

void GeometryPass(RenderData& renderData) {

    GLFrameBuffer& gBuffer = OpenGLRenderer::_frameBuffers.gBuffer;
    GLFrameBuffer& present = OpenGLRenderer::_frameBuffers.present;

    // Render target
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

    // GL state
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // Draw mesh
    Shader& gBufferShader = OpenGLRenderer::_shaders.geometry;
    gBufferShader.Use();
    gBufferShader.SetMat4("projection", renderData.cameraData.projection);
    gBufferShader.SetMat4("view", renderData.cameraData.view);
    MultiDrawIndirect(renderData.geometryDrawInfo.commands, OpenGLBackEnd::GetVertexDataVAO());

    // Draw skinned mesh
    Shader& gBufferSkinnedShader = OpenGLRenderer::_shaders.geometrySkinned;
    gBufferSkinnedShader.Use();
    gBufferSkinnedShader.SetMat4("projection", renderData.cameraData.projection);
    gBufferSkinnedShader.SetMat4("view", renderData.cameraData.view);
    MultiDrawIndirect(renderData.animatedRenderItems3D, OpenGLBackEnd::GetWeightedVertexDataVAO());
}

void DrawVATBlood(RenderData& renderData) {

    // GL State
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glCullFace(GL_BACK);

    // Draw
    Shader& shader = OpenGLRenderer::_shaders.vatBlood;
    shader.Use();
    shader.SetMat4("projection", renderData.cameraData.projection);
    shader.SetMat4("view", renderData.cameraData.view);
    MultiDrawIndirect(renderData.bloodVATDrawInfo.commands, OpenGLBackEnd::GetVertexDataVAO());
}

void DrawBloodDecals(RenderData& renderData) {

    // Render target
    GLFrameBuffer& gBuffer = OpenGLRenderer::_frameBuffers.gBuffer;
    gBuffer.Bind();
    gBuffer.SetViewport();

    // GL state
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    // Draw
    Shader& shader = OpenGLRenderer::_shaders.bloodDecals;
    shader.Use();
    shader.SetMat4("projection", renderData.cameraData.projection);
    shader.SetMat4("view", renderData.cameraData.view);
    MultiDrawIndirect(renderData.bloodDecalDrawInfo.commands, OpenGLBackEnd::GetVertexDataVAO());

    // Cleanup
    glDepthMask(GL_TRUE);
}

void DrawBulletDecals(RenderData& renderData) {

    GLFrameBuffer& gBuffer = OpenGLRenderer::_frameBuffers.gBuffer;
    GLFrameBuffer& present = OpenGLRenderer::_frameBuffers.present;

    // Render target
    gBuffer.Bind();
    gBuffer.SetViewport();
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
    shader.SetMat4("projection", renderData.cameraData.projection);
    shader.SetMat4("view", renderData.cameraData.view);

    MultiDrawIndirect(renderData.bulletHoleDecalDrawInfo.commands, OpenGLBackEnd::GetVertexDataVAO());
}

/*

█    █ █▀▀▀ █  █ ▀▀█▀▀ █ █▀▀█ █▀▀▀ 　 █▀▀█ █▀▀█ █▀▀ █▀▀
█    █ █ ▀█ █▀▀█   █   █ █  █ █ ▀█ 　 █▄▄█ █▄▄█ ▀▀█ ▀▀█
█▄▄█ ▀ ▀▀▀▀ ▀  ▀   ▀   ▀ ▀  ▀ ▀▀▀▀ 　 ▀    ▀  ▀ ▀▀▀ ▀▀▀  */

void LightingPass(RenderData& renderData) {

    //GLFrameBuffer& lightingBuffer = OpenGLRenderer::_frameBuffers.lighting;
    GLFrameBuffer& gBuffer = OpenGLRenderer::_frameBuffers.gBuffer;
    GLFrameBuffer& presentBuffer = OpenGLRenderer::_frameBuffers.present;
    gBuffer.Bind();
    gBuffer.SetViewport();
    glDrawBuffer(gBuffer.GetColorAttachmentSlotByName("FinalLighting"));

    Shader& shader = OpenGLRenderer::_shaders.lighting;
    shader.Use();

    shader.SetMat4("inverseProjection", renderData.cameraData.projectionInverse);
    shader.SetMat4("inverseView", renderData.cameraData.viewInverse);

   // glClearColor(0, 0, 1, 0);
   // glClear(GL_COLOR_BUFFER_BIT);

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
    DrawQuad();
    
}

void GlassPass(RenderData& renderData) {

    // Render glass
    GLFrameBuffer& gBuffer = OpenGLRenderer::_frameBuffers.gBuffer;
    gBuffer.Bind();
    gBuffer.SetViewport();
    glDrawBuffer(gBuffer.GetColorAttachmentSlotByName("Glass"));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gBuffer.GetColorAttachmentHandleByName("FinalLighting"));
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    Shader& shader = OpenGLRenderer::_shaders.glass;
    shader.Use();
    shader.SetMat4("projection", renderData.cameraData.projection);
    shader.SetMat4("view", renderData.cameraData.view);
    shader.SetFloat("screenWidth", renderData.cameraData.viewportWidth);
    shader.SetFloat("screenHeight", renderData.cameraData.viewportHeight);
    shader.SetVec3("viewPos", renderData.cameraData.viewInverse[3]);
    MultiDrawIndirect(renderData.glassDrawInfo.commands, OpenGLBackEnd::GetVertexDataVAO());

    // Composite that render back into the lighting texture
    ComputeShader& computeShader = OpenGLRenderer::_shaders.glassComposite;
    computeShader.Use();
    glBindImageTexture(0, gBuffer.GetColorAttachmentHandleByName("FinalLighting"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
    glBindImageTexture(1, gBuffer.GetColorAttachmentHandleByName("Glass"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
    /*
    std::vector<GLFrameBuffer>& blurBuffers = OpenGLRenderer::_frameBuffers.blurBuffers;
    glBindImageTexture(2, blurBuffers[0].GetColorAttachmentHandleByName("ColorB"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
    glBindImageTexture(3, blurBuffers[1].GetColorAttachmentHandleByName("ColorB"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
    glBindImageTexture(4, blurBuffers[2].GetColorAttachmentHandleByName("ColorB"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
    glBindImageTexture(5, blurBuffers[1].GetColorAttachmentHandleByName("ColorB"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
    */
    glDispatchCompute(gBuffer.GetWidth() / 16, gBuffer.GetHeight() / 4, 1);
}


void MuzzleFlashPass(RenderData& renderData) {


    GLFrameBuffer& gBuffer = OpenGLRenderer::_frameBuffers.gBuffer;
    GLFrameBuffer& presentBuffer = OpenGLRenderer::_frameBuffers.present;
    gBuffer.Bind();
    gBuffer.SetViewport();
    glDrawBuffer(gBuffer.GetColorAttachmentSlotByName("FinalLighting"));

    glEnable(GL_DEPTH_TEST);

    Shader& animatedQuadShader = OpenGLRenderer::_shaders.flipBook;
    animatedQuadShader.Use();
    animatedQuadShader.SetMat4("u_MatrixProjection", renderData.cameraData.projection);
    animatedQuadShader.SetMat4("u_MatrixView", renderData.cameraData.view);

   

    animatedQuadShader.SetMat4("u_MatrixWorld", renderData.muzzleFlashData.modelMatrix);
    animatedQuadShader.SetInt("u_FrameIndex", renderData.muzzleFlashData.frameIndex);
    animatedQuadShader.SetInt("u_CountRaw", renderData.muzzleFlashData.rows);
    animatedQuadShader.SetInt("u_CountColumn", renderData.muzzleFlashData.columns);
    animatedQuadShader.SetFloat("u_TimeLerp", renderData.muzzleFlashData.interpolate);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glActiveTexture(GL_TEXTURE0);
    AssetManager::GetTextureByName("MuzzleFlash_ALB")->GetGLTexture().Bind(0);

    Mesh* mesh = AssetManager::GetQuadMesh();
    glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
    glDrawElementsInstancedBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), 1, mesh->baseVertex);

    glDisable(GL_BLEND);
}


void PostProcessingPass(RenderData& renderData) {

    OpenGLRenderer::_shaders.postProcessing.Use();
    OpenGLRenderer::_shaders.postProcessing.SetVec3("finalImageColorTint", renderData.finalImageColorTint);
    OpenGLRenderer::_shaders.postProcessing.SetFloat("finalImageColorContrast", renderData.finalImageColorContrast);

    GLFrameBuffer& gBuffer = OpenGLRenderer::_frameBuffers.gBuffer;   
    glBindImageTexture(0, gBuffer.GetColorAttachmentHandleByName("FinalLighting"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
    glDispatchCompute(gBuffer.GetWidth() / 16, gBuffer.GetHeight() / 4, 1);
}

