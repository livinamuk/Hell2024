#include "GL_renderer.h"
#include "GL_backEnd.h"
#include "Types/GL_gBuffer.h"
#include "Types/GL_shader.h"
#include "Types/GL_shadowMap.h"
#include "Types/GL_ssbo.hpp"
#include "Types/GL_frameBuffer.hpp"
#include "../../BackEnd/BackEnd.h"
#include "../../Core/AssetManager.h"
#include "../../Game/Scene.h"
#include "../../Game/Game.h"
#include "../../Editor/CSG.h"
#include "../../Editor/Editor.h"
#include "../../Editor/Gizmo.hpp"
#include "../../Renderer/GlobalIllumination.h"
#include "../../Renderer/RendererCommon.h"
#include "../../Renderer/TextBlitter.h"
#include "../../Renderer/RendererStorage.h"
#include "../../Renderer/Renderer.h"
#include "../../Renderer/RendererUtil.hpp"
#include "../../Renderer/Raytracing/Raytracing.h"

namespace OpenGLRenderer {

    struct FrameBuffers {
        GLFrameBuffer loadingScreen;
        GLFrameBuffer debugMenu;
        GLFrameBuffer present;
        GLFrameBuffer gBuffer;
        GLFrameBuffer lighting;
        GLFrameBuffer finalFullSizeImage;
    } g_frameBuffers;

    struct BlurFrameBuffers {
        std::vector<GLFrameBuffer> p1;
        std::vector<GLFrameBuffer> p2;
        std::vector<GLFrameBuffer> p3;
        std::vector<GLFrameBuffer> p4;
    } g_blurBuffers;

    struct Shaders {
        Shader geometry;
        //Shader geometrySkinned;
        Shader lighting;
        Shader UI;
        Shader shadowMap;
        Shader shadowMapCSG;
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
        Shader csg;
        Shader outline;
        Shader gbufferSkinned;
        Shader csgSubtractive;
        Shader triangles2D;
        ComputeShader debugCircle;
        ComputeShader postProcessing;
        ComputeShader glassComposite;
        ComputeShader emissiveComposite;
        ComputeShader pointCloudDirectLigthing;
        //ComputeShader sandbox;
        //ComputeShader bloom;
        ComputeShader computeSkinning;
        ComputeShader raytracingTest;
        ComputeShader probeLighting;
    } g_shaders;

    struct SSBOs {
        SSBO bulletHoleDecalRenderItems;
        SSBO bloodDecalRenderItems;
        SSBO bloodVATRenderItems;
        SSBO tlasNodes;
        SSBO blasNodes;
        SSBO blasInstances;
        SSBO triangleIndices;
        SSBO materials;

        GLuint samplers = 0;
        GLuint renderItems2D = 0;
        GLuint animatedRenderItems3D = 0;
        GLuint animatedTransforms = 0;
        GLuint lights = 0;
        GLuint geometryRenderItems = 0;
        GLuint glassRenderItems = 0;
        GLuint shadowMapGeometryRenderItems = 0;
        GLuint skinningTransforms = 0;
        GLuint baseAnimatedTransformIndices = 0;
        GLuint cameraData = 0;
        GLuint skinnedMeshInstanceData = 0;
        GLuint muzzleFlashData = 0;
    } _ssbos;

    GLuint _indirectBuffer = 0;
    std::vector<ShadowMap> _shadowMaps;

    void RaytracingTestPass(RenderData& renderData);
    void Triangle2DPass();
    void ProbeGridDebugPass();
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
void OutlinePass(RenderData& renderData);
void CSGSubtractivePass();

void OpenGLRenderer::HotloadShaders() {

    std::cout << "Hotloading shaders...\n";

    g_shaders.triangles2D.Load("GL_triangles_2D.vert", "GL_triangles_2D.frag");
    g_shaders.csgSubtractive.Load("GL_csg_subtractive.vert", "GL_csg_subtractive.frag");
    g_shaders.csg.Load("GL_csg_test.vert", "GL_csg_test.frag");
    g_shaders.outline.Load("GL_outline.vert", "GL_outline.frag");
    g_shaders.UI.Load("GL_ui.vert", "GL_ui.frag");
    g_shaders.geometry.Load("GL_gbuffer.vert", "GL_gbuffer.frag");
    g_shaders.lighting.Load("GL_lighting.vert", "GL_lighting.frag");
    g_shaders.shadowMap.Load("GL_shadowMap.vert", "GL_shadowMap.frag");
    g_shaders.shadowMapCSG.Load("GL_shadowMap_csg.vert", "GL_shadowMap_csg.frag");
    g_shaders.flipBook.Load("GL_flipBook.vert", "GL_flipBook.frag");
    g_shaders.glass.Load("GL_glass.vert", "GL_glass.frag");
    g_shaders.decals.Load("GL_decals.vert", "GL_decals.frag");
    g_shaders.horizontalBlur.Load("GL_blurHorizontal.vert", "GL_blur.frag");
    g_shaders.verticalBlur.Load("GL_blurVertical.vert", "GL_blur.frag");
    g_shaders.bloodDecals.Load("GL_bloodDecals.vert", "GL_bloodDecals.frag");
    g_shaders.vatBlood.Load("GL_bloodVAT.vert", "GL_bloodVAT.frag");
    g_shaders.skyBox.Load("GL_skybox.vert", "GL_skybox.frag");
    g_shaders.debugSolidColor.Load("GL_debug_solidColor.vert", "GL_debug_solidColor.frag");
    g_shaders.debugPointCloud.Load("GL_debug_pointCloud.vert", "GL_debug_pointCloud.frag");
    g_shaders.debugProbes.Load("GL_debug_probes.vert", "GL_debug_probes.frag");
    g_shaders.gbufferSkinned.Load("GL_gbuffer_skinned.vert", "GL_gbuffer_skinned.frag");
    g_shaders.emissiveComposite.Load("res/shaders/OpenGL/GL_emissiveComposite.comp");
    g_shaders.postProcessing.Load("res/shaders/OpenGL/GL_postProcessing.comp");
    g_shaders.glassComposite.Load("res/shaders/OpenGL/GL_glassComposite.comp");
    g_shaders.pointCloudDirectLigthing.Load("res/shaders/OpenGL/GL_pointCloudDirectLighting.comp");
    g_shaders.computeSkinning.Load("res/shaders/OpenGL/GL_computeSkinning.comp");
    g_shaders.raytracingTest.Load("res/shaders/OpenGL/GL_raytracing_test.comp");
    g_shaders.debugCircle.Load("res/shaders/OpenGL/GL_debug_circle.comp");
    g_shaders.probeLighting.Load("res/shaders/OpenGL/GL_probe_lighting.comp");
}

void OpenGLRenderer::CreatePlayerRenderTargets(int presentWidth, int presentHeight) {

    if (g_frameBuffers.present.GetHandle() != 0) {
        g_frameBuffers.present.CleanUp();
    }
    if (g_frameBuffers.gBuffer.GetHandle() != 0) {
        g_frameBuffers.gBuffer.CleanUp();
    }

    g_frameBuffers.present.Create("Present", presentWidth, presentHeight);
    g_frameBuffers.present.CreateAttachment("Color", GL_RGBA8);
    g_frameBuffers.present.CreateDepthAttachment(GL_DEPTH32F_STENCIL8);

    g_frameBuffers.gBuffer.Create("GBuffer", presentWidth * 2, presentHeight * 2);
    g_frameBuffers.gBuffer.CreateAttachment("BaseColor", GL_RGBA8);
    g_frameBuffers.gBuffer.CreateAttachment("Normal", GL_RGBA16F);
    g_frameBuffers.gBuffer.CreateAttachment("RMA", GL_RGBA8);
    g_frameBuffers.gBuffer.CreateAttachment("FinalLighting", GL_RGBA8);
    g_frameBuffers.gBuffer.CreateAttachment("Glass", GL_RGBA8);
    g_frameBuffers.gBuffer.CreateAttachment("EmissiveMask", GL_RGBA8);
    g_frameBuffers.gBuffer.CreateAttachment("BloomPrePass", GL_RGBA8);
    g_frameBuffers.gBuffer.CreateDepthAttachment(GL_DEPTH32F_STENCIL8);
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

    g_frameBuffers.debugMenu.Create("DebugMenu", PRESENT_WIDTH, PRESENT_HEIGHT);
    g_frameBuffers.debugMenu.CreateAttachment("Color", GL_RGBA8);

    g_frameBuffers.loadingScreen.Create("LoadingsCreen", PRESENT_WIDTH * scaleRatio, PRESENT_HEIGHT * scaleRatio);
    g_frameBuffers.loadingScreen.CreateAttachment("Color", GL_RGBA8);

    g_frameBuffers.finalFullSizeImage.Create("FinalFullSizeImage", PRESENT_WIDTH, PRESENT_HEIGHT);
    g_frameBuffers.finalFullSizeImage.CreateAttachment("Color", GL_RGBA8);

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

    //glCreateBuffers(1, &_ssbos.bulletHoleDecalRenderItems);
    //glNamedBufferStorage(_ssbos.bulletHoleDecalRenderItems, MAX_DECAL_COUNT * sizeof(RenderItem3D), NULL, GL_DYNAMIC_STORAGE_BIT);


    _ssbos.bulletHoleDecalRenderItems.PreAllocate(MAX_DECAL_COUNT * sizeof(RenderItem3D));
    _ssbos.bloodDecalRenderItems.PreAllocate(MAX_BLOOD_DECAL_COUNT * sizeof(RenderItem3D));
    _ssbos.bloodVATRenderItems.PreAllocate(MAX_VAT_INSTANCE_COUNT * sizeof(RenderItem3D));
    //_ssbos.materials.PreAllocate(AssetManager::GetMaterialCount() * sizeof(GPUMaterial));



  /*  glCreateBuffers(1, &_ssbos.bloodDecalRenderItems);
    glNamedBufferStorage(_ssbos.bloodDecalRenderItems, MAX_BLOOD_DECAL_COUNT * sizeof(RenderItem3D), NULL, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &_ssbos.bloodVATRenderItems);
    glNamedBufferStorage(_ssbos.bloodVATRenderItems, MAX_VAT_INSTANCE_COUNT * sizeof(RenderItem3D), NULL, GL_DYNAMIC_STORAGE_BIT);*/

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

void BlitDepthBuffer(GLFrameBuffer* src, GLFrameBuffer* dst, const char* srcName, const char* dstName, GLbitfield mask, GLenum filter) {
    GLint srcHandle = 0;
    GLint dstHandle = 0;
    GLint srcWidth = BackEnd::GetCurrentWindowWidth();
    GLint srcHeight = BackEnd::GetCurrentWindowHeight();
    GLint dstWidth = BackEnd::GetCurrentWindowWidth();
    GLint dstHeight = BackEnd::GetCurrentWindowHeight();
    if (src) {
        srcHandle = src->GetHandle();
        srcWidth = src->GetWidth();
        srcHeight = src->GetHeight();
    }
    if (dst) {
        dstHandle = dst->GetHandle();
        dstWidth = dst->GetWidth();
        dstHeight = dst->GetHeight();
    }
    glBlitFramebuffer(0, 0, srcWidth, srcHeight, 0, 0, dstWidth, dstHeight, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
}

void DownScaleGBuffer() {
    GLFrameBuffer& gBuffer = OpenGLRenderer::g_frameBuffers.gBuffer;
    GLFrameBuffer& presentBuffer = OpenGLRenderer::g_frameBuffers.present;
    BlitFrameBuffer(&gBuffer, &presentBuffer, "FinalLighting", "Color", GL_COLOR_BUFFER_BIT, GL_LINEAR);
    BlitDepthBuffer(&gBuffer, &presentBuffer, "BaseColor", "Color", GL_COLOR_BUFFER_BIT, GL_LINEAR);
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

    RenderUI(renderItems, g_frameBuffers.loadingScreen, true);
    BlitFrameBuffer(&g_frameBuffers.loadingScreen, 0, "Color", "", GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

void UploadSSBOsGPU(RenderData& renderData) {

    glNamedBufferSubData(OpenGLRenderer::_ssbos.geometryRenderItems, 0, renderData.geometryDrawInfo.renderItems.size() * sizeof(RenderItem3D), &renderData.geometryDrawInfo.renderItems[0]);
    glNamedBufferSubData(OpenGLRenderer::_ssbos.lights, 0, renderData.lights.size() * sizeof(GPULight), &renderData.lights[0]);
    glNamedBufferSubData(OpenGLRenderer::_ssbos.glassRenderItems, 0, renderData.glassDrawInfo.renderItems.size() * sizeof(RenderItem3D), &renderData.glassDrawInfo.renderItems[0]);
    glNamedBufferSubData(OpenGLRenderer::_ssbos.shadowMapGeometryRenderItems, 0, renderData.shadowMapGeometryDrawInfo.renderItems.size() * sizeof(RenderItem3D), &renderData.shadowMapGeometryDrawInfo.renderItems[0]);
    glNamedBufferSubData(OpenGLRenderer::_ssbos.skinningTransforms, 0, renderData.skinningTransforms.size() * sizeof(glm::mat4), &renderData.skinningTransforms[0]);
    glNamedBufferSubData(OpenGLRenderer::_ssbos.baseAnimatedTransformIndices, 0, renderData.baseAnimatedTransformIndices.size() * sizeof(uint32_t), &renderData.baseAnimatedTransformIndices[0]);
    glNamedBufferSubData(OpenGLRenderer::_ssbos.cameraData, 0, 4 * sizeof(CameraData), &renderData.cameraData[0]);
    glNamedBufferSubData(OpenGLRenderer::_ssbos.skinnedMeshInstanceData, 0, sizeof(SkinnedRenderItem) * renderData.allSkinnedRenderItems.size(), &renderData.allSkinnedRenderItems[0]);
    glNamedBufferSubData(OpenGLRenderer::_ssbos.muzzleFlashData, 0, sizeof(MuzzleFlashData) * 4, &renderData.muzzleFlashData[0]);

    OpenGLRenderer::_ssbos.bulletHoleDecalRenderItems.Update(renderData.bulletHoleDecalDrawInfo.renderItems.size() * sizeof(RenderItem3D), renderData.bulletHoleDecalDrawInfo.renderItems.data());
    OpenGLRenderer::_ssbos.bloodDecalRenderItems.Update(renderData.bloodDecalDrawInfo.renderItems.size() * sizeof(RenderItem3D), renderData.bloodDecalDrawInfo.renderItems.data());
    OpenGLRenderer::_ssbos.bloodVATRenderItems.Update(renderData.bloodVATDrawInfo.renderItems.size() * sizeof(RenderItem3D), renderData.bloodVATDrawInfo.renderItems.data());
    OpenGLRenderer::_ssbos.materials.Update(AssetManager::GetGPUMaterials().size() * sizeof(GPUMaterial), &AssetManager::GetGPUMaterials()[0]);
}

void OpenGLRenderer::RenderFrame(RenderData& renderData) {

    GLFrameBuffer& gBuffer = g_frameBuffers.gBuffer;
    GLFrameBuffer& present = g_frameBuffers.present;

    UploadSSBOsGPU(renderData);
    UpdatePointCloud();

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, _ssbos.samplers);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, OpenGLRenderer::_ssbos.geometryRenderItems);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, OpenGLRenderer::_ssbos.lights);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, OpenGLRenderer::_ssbos.materials.GetHandle());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, OpenGLRenderer::_ssbos.animatedTransforms);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, OpenGLRenderer::_ssbos.animatedRenderItems3D);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, OpenGLRenderer::_ssbos.glassRenderItems);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 12, OpenGLRenderer::_ssbos.shadowMapGeometryRenderItems);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 13, OpenGLRenderer::_ssbos.bulletHoleDecalRenderItems.GetHandle());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 14, OpenGLRenderer::_ssbos.bloodDecalRenderItems.GetHandle());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 15, OpenGLRenderer::_ssbos.bloodVATRenderItems.GetHandle());

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 16, OpenGLRenderer::_ssbos.cameraData);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 17, OpenGLRenderer::_ssbos.skinnedMeshInstanceData);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 18, OpenGLRenderer::_ssbos.muzzleFlashData);

    //RaytracingTestPass(renderData);

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
    MuzzleFlashPass(renderData);
    GlassPass(renderData);
    EmissivePass(renderData);
    MuzzleFlashPass(renderData);
    PostProcessingPass(renderData);

    ProbeGridDebugPass();


    RenderUI(renderData.renderItems2DHiRes, g_frameBuffers.gBuffer, false);
    DownScaleGBuffer();
    DebugPass(renderData);
    CSGSubtractivePass();
    OutlinePass(renderData);
    if (Editor::ObjectIsSelected()) {
        Gizmo::Draw(renderData.cameraData[0].projection, renderData.cameraData[0].view, g_frameBuffers.present.GetWidth(), g_frameBuffers.present.GetHeight());
    }
    Triangle2DPass();
    RenderUI(renderData.renderItems2D, g_frameBuffers.present, false);
}


void ClearRenderTargets() {

    // Clear GBuffer color attachments
    GLFrameBuffer& gBuffer = OpenGLRenderer::g_frameBuffers.gBuffer;
    GLFrameBuffer& present = OpenGLRenderer::g_frameBuffers.present;

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

    GLFrameBuffer& gBuffer = OpenGLRenderer::g_frameBuffers.gBuffer;
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
        Shader& shader = OpenGLRenderer::g_shaders.skyBox;
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

    GLFrameBuffer& gBuffer = OpenGLRenderer::g_frameBuffers.gBuffer;
    Shader& horizontalBlurShader = OpenGLRenderer::g_shaders.horizontalBlur;
    Shader& verticalBlurShader = OpenGLRenderer::g_shaders.verticalBlur;

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
        ComputeShader& computeShader = OpenGLRenderer::g_shaders.emissiveComposite;
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


bool IsBoxInFrustum(const glm::vec3& minBounds, const glm::vec3& maxBounds, const glm::mat4& projectionViewMatrix) {
    std::vector<glm::vec3> corners = {
        glm::vec3(minBounds.x, minBounds.y, minBounds.z),
        glm::vec3(maxBounds.x, minBounds.y, minBounds.z),
        glm::vec3(minBounds.x, maxBounds.y, minBounds.z),
        glm::vec3(maxBounds.x, maxBounds.y, minBounds.z),
        glm::vec3(minBounds.x, minBounds.y, maxBounds.z),
        glm::vec3(maxBounds.x, minBounds.y, maxBounds.z),
        glm::vec3(minBounds.x, maxBounds.y, maxBounds.z),
        glm::vec3(maxBounds.x, maxBounds.y, maxBounds.z)
    };

    int outCount = 0;
    for (const auto& corner : corners) {
        glm::vec4 clipSpaceCorner = projectionViewMatrix * glm::vec4(corner, 1.0f);
        glm::vec3 ndcCorner = glm::vec3(clipSpaceCorner) / clipSpaceCorner.w;

        // Check if this point is outside of the frustum
        if (ndcCorner.x < -1 || ndcCorner.x > 1 ||
            ndcCorner.y < -1 || ndcCorner.y > 1 ||
            ndcCorner.z < -1 || ndcCorner.z > 1) {
            outCount++;
        }
    }

    // If all corners are outside of the frustum, the box is not visible
    return outCount != 8;
}


MultiDrawIndirectDrawInfo CreateMultiDrawIndirectDrawInfo2(std::vector<RenderItem3D>& renderItems) {

    MultiDrawIndirectDrawInfo drawInfo;
    drawInfo.renderItems = renderItems;
    std::sort(drawInfo.renderItems.begin(), drawInfo.renderItems.end());

    // Create indirect draw commands
    drawInfo.commands.clear();
    int baseInstance = 0;
    for (RenderItem3D& renderItem : drawInfo.renderItems) {
        Mesh* mesh = AssetManager::GetMeshByIndex(renderItem.meshIndex);
        bool found = false;
        // Does a draw command already exist for this mesh?
        for (auto& cmd : drawInfo.commands) {
            // If so then increment the instance count
            if (cmd.baseVertex == mesh->baseVertex) {
                cmd.instanceCount++;
                baseInstance++;
                found = true;
                break;
            }
        }
        // If not, then create the command
        if (!found) {
            auto& cmd = drawInfo.commands.emplace_back();
            cmd.indexCount = mesh->indexCount;
            cmd.firstIndex = mesh->baseIndex;
            cmd.baseVertex = mesh->baseVertex;
            cmd.baseInstance = baseInstance;
            cmd.instanceCount = 1;
            baseInstance++;
        }
    }
    return drawInfo;
}

void RenderShadowMapss(RenderData& renderData) {

    if (0 != 0) {
        return;
    }

    Shader& shader = OpenGLRenderer::g_shaders.shadowMap;

    shader.Use();
    shader.SetFloat("far_plane", SHADOW_FAR_PLANE);
    glDepthMask(true);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
    glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());


    /*
    std::vector<RenderItem3D> renderItems = renderData.shadowMapGeometryDrawInfo.renderItems;


    for (int i = 0; i < Scene::g_lights.size(); i++) {

        glBindFramebuffer(GL_FRAMEBUFFER, OpenGLRenderer::_shadowMaps[i]._ID);




        std::vector<glm::mat4> projectionTransforms;
        glm::vec3 position = Scene::g_lights[i].position;
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


        for (int face = 0; face < 6; ++face) {

            std::vector<RenderItem3D> culledRenderItems;
            for (RenderItem3D& renderItem : renderItems) {
                if (IsBoxInFrustum(renderItem.aabbMin, renderItem.aabbMax, projectionTransforms[i]) ||
                    renderItem.aabbMin == glm::vec3(0) && renderItem.aabbMax == glm::vec3(0)) {
                    culledRenderItems.push_back(renderItem);
                }
                else {
                    //std::cout << "culled " << Util::Vec3ToString(renderItem.aabbMin) << " " << Util::Vec3ToString(renderItem.aabbMin) << "\n";
                }
            }
            std::cout << "light " << i << "  " << face << " Culled: " << renderItems.size() - culledRenderItems.size() << "items\n";

            MultiDrawIndirectDrawInfo drawInfo = CreateMultiDrawIndirectDrawInfo2(culledRenderItems);

            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 12, OpenGLRenderer::_ssbos.shadowMapGeometryRenderItems);
            glNamedBufferSubData(OpenGLRenderer::_ssbos.shadowMapGeometryRenderItems, 0, culledRenderItems.size() * sizeof(RenderItem3D), &culledRenderItems[0]);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

            shader.SetInt("faceIndex", face);
            GLuint depthCubemap = OpenGLRenderer::_shadowMaps[i]._depthTexture;
            glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap, 0, face);
            glClear(GL_DEPTH_BUFFER_BIT);
            MultiDrawIndirect(drawInfo.commands, OpenGLBackEnd::GetVertexDataVAO());

        }



    }

    glNamedBufferSubData(OpenGLRenderer::_ssbos.shadowMapGeometryRenderItems, 0, renderData.shadowMapGeometryDrawInfo.renderItems.size() * sizeof(RenderItem3D), &renderData.shadowMapGeometryDrawInfo.renderItems[0]);

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    */


    for (int i = 0; i < Scene::g_lights.size(); i++) {

        bool skip = false;
        if (!Scene::g_lights[i].isDirty) {
            skip = true;
        }
        if (skip) {
            continue;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, OpenGLRenderer::_shadowMaps[i]._ID);

        std::vector<glm::mat4> projectionTransforms;
        glm::vec3 position = Scene::g_lights[i].position;
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

        for (int face = 0; face < 6; ++face) {
            shader.SetInt("faceIndex", face);
            GLuint depthCubemap = OpenGLRenderer::_shadowMaps[i]._depthTexture;
            glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap, 0, face);
            glClear(GL_DEPTH_BUFFER_BIT);
            MultiDrawIndirect(renderData.shadowMapGeometryDrawInfo.commands, OpenGLBackEnd::GetVertexDataVAO());
        }
    }


    // CSG Geometry
    if (CSG::GeometryExists()) {

        Shader& csgShadowMapShader = OpenGLRenderer::g_shaders.shadowMapCSG;
        csgShadowMapShader.Use();
        csgShadowMapShader.SetFloat("far_plane", SHADOW_FAR_PLANE);

        glBindVertexArray(OpenGLBackEnd::GetCSGVAO());

        for (int i = 0; i < Scene::g_lights.size(); i++) {

            bool skip = false;
            if (!Scene::g_lights[i].isDirty) {
                skip = true;
            }
            if (skip) {
                continue;
            }

            std::vector<glm::mat4> projectionTransforms;
            glm::vec3 position = Scene::g_lights[i].position;
            glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), (float)SHADOW_MAP_SIZE / (float)SHADOW_MAP_SIZE, SHADOW_NEAR_PLANE, SHADOW_FAR_PLANE);
            projectionTransforms.clear();
            projectionTransforms.push_back(shadowProj * glm::lookAt(position, position + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
            projectionTransforms.push_back(shadowProj * glm::lookAt(position, position + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
            projectionTransforms.push_back(shadowProj * glm::lookAt(position, position + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
            projectionTransforms.push_back(shadowProj * glm::lookAt(position, position + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)));
            projectionTransforms.push_back(shadowProj * glm::lookAt(position, position + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
            projectionTransforms.push_back(shadowProj * glm::lookAt(position, position + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));

            csgShadowMapShader.SetMat4("shadowMatrices[0]", projectionTransforms[0]);
            csgShadowMapShader.SetMat4("shadowMatrices[1]", projectionTransforms[1]);
            csgShadowMapShader.SetMat4("shadowMatrices[2]", projectionTransforms[2]);
            csgShadowMapShader.SetMat4("shadowMatrices[3]", projectionTransforms[3]);
            csgShadowMapShader.SetMat4("shadowMatrices[4]", projectionTransforms[4]);
            csgShadowMapShader.SetMat4("shadowMatrices[5]", projectionTransforms[5]);
            csgShadowMapShader.SetVec3("lightPosition", position);
            csgShadowMapShader.SetMat4("model", glm::mat4(1));

            glBindFramebuffer(GL_FRAMEBUFFER, OpenGLRenderer::_shadowMaps[i]._ID);

            // Render the scene six times, once for each face
            for (int face = 0; face < 6; ++face) {
                csgShadowMapShader.SetInt("faceIndex", face);
                GLuint depthCubemap = OpenGLRenderer::_shadowMaps[i]._depthTexture;
                glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap, 0, face);
                std::vector<CSGObject>& cubes = CSG::GetCSGObjects();
                for (int j = 0; j < cubes.size(); j++) {
                    if (cubes[j].m_disableRendering) {
                        continue;
                    }
                    glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLES, cubes[j].m_vertexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * cubes[j].m_baseIndex), 1, cubes[j].m_baseVertex, j);
                }
            }
        }
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
    OpenGLRenderer::g_shaders.UI.Use();
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
    OpenGLDetachedMesh& trianglesMesh = renderData.debugTrianglesMesh->GetGLMesh();

    // Render target
    GLFrameBuffer& presentBuffer = OpenGLRenderer::g_frameBuffers.present;
    ViewportInfo viewportInfo = RendererUtil::CreateViewportInfo(0, Game::GetSplitscreenMode(), presentBuffer.GetWidth(), presentBuffer.GetHeight());
    BindFrameBuffer(presentBuffer);
    SetViewport(viewportInfo);

    Shader& shader = OpenGLRenderer::g_shaders.debugSolidColor;
    shader.Use();
    shader.SetMat4("projection", renderData.cameraData[0].projection);
    shader.SetMat4("view", renderData.cameraData[0].view);
    shader.SetMat4("model", glm::mat4(1));

    glDisable(GL_DEPTH_TEST);

    // Draw lines
    if (linesMesh.GetIndexCount() > 0) {
        glBindVertexArray(linesMesh.GetVAO());
        glDrawElements(GL_LINES, linesMesh.GetIndexCount(), GL_UNSIGNED_INT, 0);
    }
    // Draw points
    if (pointsMesh.GetIndexCount() > 0) {
        glPointSize(4);
        glBindVertexArray(pointsMesh.GetVAO());
        glDrawElements(GL_POINTS, pointsMesh.GetIndexCount(), GL_UNSIGNED_INT, 0);
    }
    // Draw triangles
    if (trianglesMesh.GetIndexCount() > 0) {
        glPointSize(4);
        glBindVertexArray(trianglesMesh.GetVAO());
        glDrawElements(GL_TRIANGLES, trianglesMesh.GetIndexCount(), GL_UNSIGNED_INT, 0);
    }

    /*
    Player* player = Game::GetPlayerByIndex(0);
    glm::mat4 projectionMatrix = player->GetProjectionMatrix();
    glm::mat4 viewMatrix = player->GetViewMatrix();
    glm::vec3 cameraPos = player->GetViewPos();
    int mouseX = Input::GetMouseX();
    int mouseY = Input::GetMouseY();
    int windowWidth = BackEnd::GetCurrentWindowWidth();
    int windowHeight = BackEnd::GetCurrentWindowHeight();
    glm::vec3 rayWorld = Util::GetMouseRay(projectionMatrix, viewMatrix, windowWidth, windowHeight, mouseX, mouseY);
    if (!Editor::IsOpen()) {
        rayWorld = player->GetCameraForward();
    }
    glm::vec3 rayOrigin = cameraPos;
    glm::vec3 rayDirection = rayWorld;
    float t = -rayOrigin.y / rayDirection.y;
    glm::vec3 intersectionPoint = rayOrigin + t * rayDirection;
    float worldX = intersectionPoint.x;
    float worldZ = intersectionPoint.z;

    Transform transform;
   // transform.position = glm::vec3(worldX * Pathfinding::GetGridSpacing(), 0, worldZ * Pathfinding::GetGridSpacing());

    worldX = (int)(worldX / Pathfinding::GetGridSpacing()) * Pathfinding::GetGridSpacing();
    worldZ = (int)(worldZ / Pathfinding::GetGridSpacing()) * Pathfinding::GetGridSpacing();

    transform.position = glm::vec3(worldX, 0, worldZ);

    transform.position += glm::vec3(Pathfinding::GetGridSpacing() * 0.5f, 0, Pathfinding::GetGridSpacing() * 0.5f);
   // transform.position.x += Pathfinding::GetWorldSpaceOffsetX();
   // transform.position.z += Pathfinding::GetWorldSpaceOffsetZ();
    transform.scale = glm::vec3(Pathfinding::GetGridSpacing());
    shader.SetMat4("model", transform.to_mat4());
    shader.SetBool("useUniformColor", true);
    shader.SetVec3("uniformColor", BLUE);
    Mesh* mesh = AssetManager::GetMeshByIndex(AssetManager::GetUpFacingPlaneMeshIndex());
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
    glDrawElementsInstancedBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), 1, mesh->baseVertex);
    */




    shader.SetBool("useUniformColor", false);


    // Point cloud
    if (renderData.renderMode == RenderMode::COMPOSITE_PLUS_POINT_CLOUD ||
        renderData.renderMode == RenderMode::POINT_CLOUD) {
        Shader& pointCloudDebugShader = OpenGLRenderer::g_shaders.debugPointCloud;
        pointCloudDebugShader.Use();
        pointCloudDebugShader.SetMat4("projection", renderData.cameraData[0].projection);
        pointCloudDebugShader.SetMat4("view", renderData.cameraData[0].view);
        glBindVertexArray(OpenGLBackEnd::GetPointCloudVAO());
        glPointSize(4);
        glDrawArrays(GL_POINTS, 0, GlobalIllumination::GetPointCloud().size());
    }
}

void DebugPassProbePass(RenderData& renderData) {

    static int cubeMeshIndex = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Cube"))->GetMeshIndices()[0];

    // Render target
    GLFrameBuffer& gBuffer = OpenGLRenderer::g_frameBuffers.gBuffer;
    ViewportInfo viewportInfo = RendererUtil::CreateViewportInfo(0, Game::GetSplitscreenMode(), gBuffer.GetWidth(), gBuffer.GetHeight());
    BindFrameBuffer(gBuffer);
    SetViewport(viewportInfo);

    Shader& shader = OpenGLRenderer::g_shaders.debugProbes;
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
    if (Renderer::ProbesVisible()) {
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

    ComputeShader& computeShader = OpenGLRenderer::g_shaders.computeSkinning;
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
    GLFrameBuffer& gBuffer = OpenGLRenderer::g_frameBuffers.gBuffer;
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

    // Constructive Solid Geometry
    Shader& csgShader = OpenGLRenderer::g_shaders.csg;
    csgShader.Use();
    if (CSG::GeometryExists()) {
        csgShader.SetMat4("model", glm::mat4(1));
        csgShader.SetMat4("inverseModel", glm::inverse(glm::mat4(1)));
        for (int i = 0; i < renderData.playerCount; i++) {
            ViewportInfo viewportInfo = RendererUtil::CreateViewportInfo(i, Game::GetSplitscreenMode(), gBuffer.GetWidth(), gBuffer.GetHeight());
            SetViewport(viewportInfo);
            glViewport(viewportInfo.xOffset, viewportInfo.yOffset, viewportInfo.width, viewportInfo.height);
            csgShader.SetMat4("projection", renderData.cameraData[i].projection);
            csgShader.SetMat4("view", renderData.cameraData[i].view);
            csgShader.SetInt("playerIndex", i);
            glBindVertexArray(OpenGLBackEnd::GetCSGVAO());
            std::vector<CSGObject>& cubes = CSG::GetCSGObjects();
            for (int j = 0; j < cubes.size(); j++) {
                CSGObject& cube = cubes[j];
                if (cube.m_disableRendering) {
                    continue;
                }
                csgShader.SetInt("materialIndex", cube.m_materialIndex);
                glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLES, cube.m_vertexCount, GL_UNSIGNED_INT, (void*)(sizeof(uint32_t) * cube.m_baseIndex), 1, cube.m_baseVertex, j);
            }
            /*for (Door& door : Scene::GetDoors()) {
                csgShader.SetMat4("model", door.GetDoorModelMatrix());
                glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLES, 12, GL_UNSIGNED_INT, (void*)(sizeof(uint32_t) * 0), 1, 0, 0);
            }*/
        }
    }

    // Draw mesh
    static Material* goldMaterial = AssetManager::GetMaterialByIndex(AssetManager::GetMaterialIndex("Gold"));
    Shader& gBufferShader = OpenGLRenderer::g_shaders.geometry;
    gBufferShader.Use();
    gBufferShader.SetInt("goldBaseColorTextureIndex", goldMaterial->_basecolor);
    gBufferShader.SetInt("goldRMATextureIndex", goldMaterial->_rma);
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

    Shader& gbufferSkinnedShader = OpenGLRenderer::g_shaders.gbufferSkinned;
    gbufferSkinnedShader.Use();
    gbufferSkinnedShader.SetInt("goldBaseColorTextureIndex", goldMaterial->_basecolor);
    gbufferSkinnedShader.SetInt("goldRMATextureIndex", goldMaterial->_rma);

    int k = 0;
    for (int i = 0; i < renderData.playerCount; i++) {

        ViewportInfo viewportInfo = RendererUtil::CreateViewportInfo(i, Game::GetSplitscreenMode(), gBuffer.GetWidth(), gBuffer.GetHeight());
        SetViewport(viewportInfo);

        gbufferSkinnedShader.SetInt("playerIndex", i);
        gbufferSkinnedShader.SetMat4("projection", renderData.cameraData[i].projection);
        gbufferSkinnedShader.SetMat4("view", renderData.cameraData[i].view);

        for (SkinnedRenderItem& skinnedRenderItem : renderData.skinnedRenderItems[i]) {
            SkinnedMesh* mesh = AssetManager::GetSkinnedMeshByIndex(skinnedRenderItem.originalMeshIndex);
            gbufferSkinnedShader.SetMat4("model", skinnedRenderItem.modelMatrix);
            gbufferSkinnedShader.SetInt("renderItemIndex", k);
            glDrawElementsInstancedBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), 1, skinnedRenderItem.baseVertex);
            k++;
        }
    }
    glBindVertexArray(0);

}

void DrawVATBlood(RenderData& renderData) {

    // Render target
    GLFrameBuffer& gBuffer = OpenGLRenderer::g_frameBuffers.gBuffer;
    BindFrameBuffer(gBuffer);

    // GL State
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glCullFace(GL_BACK);

    Shader& shader = OpenGLRenderer::g_shaders.vatBlood;
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
    GLFrameBuffer& gBuffer = OpenGLRenderer::g_frameBuffers.gBuffer;
    BindFrameBuffer(gBuffer);

    // GL state
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    // Draw
    Shader& shader = OpenGLRenderer::g_shaders.bloodDecals;
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
    GLFrameBuffer& gBuffer = OpenGLRenderer::g_frameBuffers.gBuffer;
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
    Shader& shader = OpenGLRenderer::g_shaders.decals;
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

    GLFrameBuffer& gBuffer = OpenGLRenderer::g_frameBuffers.gBuffer;
    gBuffer.Bind();
    gBuffer.SetViewport();
    glDrawBuffer(gBuffer.GetColorAttachmentSlotByName("FinalLighting"));

    Shader& shader = OpenGLRenderer::g_shaders.lighting;
    shader.Use();

    shader.SetMat4("inverseProjection", renderData.cameraData[0].projectionInverse);
    shader.SetMat4("inverseView", renderData.cameraData[0].viewInverse);
    shader.SetFloat("clipSpaceXMin", renderData.cameraData[0].clipSpaceXMin);
    shader.SetFloat("clipSpaceXMax", renderData.cameraData[0].clipSpaceXMax);
    shader.SetFloat("clipSpaceYMin", renderData.cameraData[0].clipSpaceYMin);
    shader.SetFloat("clipSpaceYMax", renderData.cameraData[0].clipSpaceYMax);
    shader.SetFloat("time", Game::GetTime());

    if (Renderer::GetRenderMode() == COMPOSITE ||
        Renderer::GetRenderMode() == COMPOSITE_PLUS_POINT_CLOUD) {
        shader.SetInt("renderMode", 0);
    }
    else if(Renderer::GetRenderMode() == DIRECT_LIGHT) {
        shader.SetInt("renderMode", 1);
    }
    else if (Renderer::GetRenderMode() == POINT_CLOUD) {
        shader.SetInt("renderMode", 2);
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gBuffer.GetColorAttachmentHandleByName("BaseColor"));
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gBuffer.GetColorAttachmentHandleByName("Normal"));
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gBuffer.GetColorAttachmentHandleByName("RMA"));
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, gBuffer.GetDepthAttachmentHandle());
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, gBuffer.GetColorAttachmentHandleByName("EmissiveMask"));

    LightVolume* lightVolume = GlobalIllumination::GetLightVolumeByIndex(0);
    shader.SetInt("probeSpaceWidth", lightVolume->GetProbeSpaceWidth());
    shader.SetInt("probeSpaceHeight", lightVolume->GetProbeSpaceHeight());
    shader.SetInt("probeSpaceDepth", lightVolume->GetProbeSpaceDepth());
    shader.SetVec3("lightVolumePosition", lightVolume->GetPosition());
    shader.SetFloat("probeSpacing", PROBE_SPACING);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_3D, lightVolume->texutre3D.GetGLTexture3D().GetID());

    for (int i = 0; i < renderData.lights.size(); i++) {
        glActiveTexture(GL_TEXTURE6 + i);
        glBindTexture(GL_TEXTURE_CUBE_MAP, OpenGLRenderer::_shadowMaps[i]._depthTexture);
    }

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
    int meshIndex = AssetManager::GetQuadMeshIndex();
    Mesh* mesh = AssetManager::GetMeshByIndex(meshIndex);
    glDrawElementsBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), mesh->baseVertex);
}

void GlassPass(RenderData& renderData) {

    // Render target
    GLFrameBuffer& gBuffer = OpenGLRenderer::g_frameBuffers.gBuffer;
    BindFrameBuffer(gBuffer);

    glDrawBuffer(gBuffer.GetColorAttachmentSlotByName("Glass"));
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    Shader& shader = OpenGLRenderer::g_shaders.glass;
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
    ComputeShader& computeShader = OpenGLRenderer::g_shaders.glassComposite;
    computeShader.Use();
    glBindImageTexture(0, gBuffer.GetColorAttachmentHandleByName("FinalLighting"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
    glBindImageTexture(1, gBuffer.GetColorAttachmentHandleByName("Glass"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
    glDispatchCompute(gBuffer.GetWidth() / 16, gBuffer.GetHeight() / 4, 1);
}


void MuzzleFlashPass(RenderData& renderData) {

    // Render target
    GLFrameBuffer& gBuffer = OpenGLRenderer::g_frameBuffers.gBuffer;
    BindFrameBuffer(gBuffer);
    glDrawBuffer(gBuffer.GetColorAttachmentSlotByName("FinalLighting"));
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    Shader& animatedQuadShader = OpenGLRenderer::g_shaders.flipBook;
    animatedQuadShader.Use();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glActiveTexture(GL_TEXTURE0);
    AssetManager::GetTextureByName("MuzzleFlash_ALB")->GetGLTexture().Bind(0);

    Mesh* mesh = AssetManager::GetQuadMesh();
    glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());

    for (int i = 0; i < renderData.playerCount; i++) {
        animatedQuadShader.SetInt("playerIndex", i);
        ViewportInfo viewportInfo = RendererUtil::CreateViewportInfo(i, Game::GetSplitscreenMode(), gBuffer.GetWidth(), gBuffer.GetHeight());
        SetViewport(viewportInfo);
        glDrawElementsInstancedBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), 1, mesh->baseVertex);
    }
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}


void OutlinePass(RenderData& renderData) {

    if (!Editor::IsOpen()) {
        return;
    }

    Shader& shader = OpenGLRenderer::g_shaders.outline;
    shader.Use();

    std::vector<RenderItem3D> hoveredRenderItems = Editor::GetHoveredRenderItems();
    std::vector<RenderItem3D> selectionRenderItems = Editor::GetSelectedRenderItems();

    if (Editor::GetHoveredObjectIndex() != -1) {
        // Render mask
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_STENCIL_TEST);
        glStencilMask(0xff);
        glClearStencil(0);
        glClear(GL_STENCIL_BUFFER_BIT);
        glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
        glStencilFunc(GL_ALWAYS, 1, 1);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

        if (Editor::GetHoveredObjectType() == PhysicsObjectType::CSG_OBJECT_ADDITIVE) {
            CSGObject& cube = CSG::GetCSGObjects()[Editor::GetHoveredObjectIndex()];
            shader.SetMat4("model", glm::mat4(1));
            glBindVertexArray(OpenGLBackEnd::GetCSGVAO());
            glDrawElementsInstancedBaseVertex(GL_TRIANGLES, cube.m_vertexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * cube.m_baseIndex), 1, cube.m_baseVertex);
        }
        else {
            for (RenderItem3D& renderItem : hoveredRenderItems) {
                Mesh* mesh = AssetManager::GetMeshByIndex(renderItem.meshIndex);
                shader.SetMat4("model", renderItem.modelMatrix);
                glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
                glDrawElementsInstancedBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), 1, mesh->baseVertex);
            }
        }
        // Render outline
        glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
        glStencilMask(0x00);
        glEnable(GL_STENCIL_TEST);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDisable(GL_DEPTH_TEST);
        shader.SetVec3("Color", glm::vec3(0.5));

        if (Editor::GetHoveredObjectType() == PhysicsObjectType::CSG_OBJECT_ADDITIVE) {
            CSGObject& cube = CSG::GetCSGObjects()[Editor::GetHoveredObjectIndex()];
            shader.SetMat4("model", glm::mat4(1));
            glBindVertexArray(OpenGLBackEnd::GetCSGVAO());
            glDrawElementsInstancedBaseVertex(GL_TRIANGLES, cube.m_vertexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * cube.m_baseIndex), 5, cube.m_baseVertex);
        }
        else {
            for (RenderItem3D& renderItem : hoveredRenderItems) {
                Mesh* mesh = AssetManager::GetMeshByIndex(renderItem.meshIndex);
                shader.SetMat4("model", renderItem.modelMatrix);
                glDrawElementsInstancedBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), 5, mesh->baseVertex);
            }
        }
    }

    if (Editor::GetSelectedObjectIndex() != -1) {
        // Render mask
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_STENCIL_TEST);
        glStencilMask(0xff);
        glClearStencil(0);
        glClear(GL_STENCIL_BUFFER_BIT);
        glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
        glStencilFunc(GL_ALWAYS, 1, 1);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

        if (Editor::GetSelectedObjectType() == PhysicsObjectType::CSG_OBJECT_ADDITIVE) {
            CSGObject& cube = CSG::GetCSGObjects()[Editor::GetSelectedObjectIndex()];
            shader.SetMat4("model", glm::mat4(1));
            glBindVertexArray(OpenGLBackEnd::GetCSGVAO());
            glDrawElementsInstancedBaseVertex(GL_TRIANGLES, cube.m_vertexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * cube.m_baseIndex), 1, cube.m_baseVertex);
        }
        else {
            for (RenderItem3D& renderItem : selectionRenderItems) {
                Mesh* mesh = AssetManager::GetMeshByIndex(renderItem.meshIndex);
                shader.SetMat4("model", renderItem.modelMatrix);
                glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
                glDrawElementsInstancedBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), 1, mesh->baseVertex);
            }
        }
        // Render outline
        glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
        glStencilMask(0x00);
        glEnable(GL_STENCIL_TEST);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDisable(GL_DEPTH_TEST);
        shader.SetVec3("Color", YELLOW);

        if (Editor::GetSelectedObjectType() == PhysicsObjectType::CSG_OBJECT_ADDITIVE) {
            CSGObject& cube = CSG::GetCSGObjects()[Editor::GetSelectedObjectIndex()];
            shader.SetMat4("model", glm::mat4(1));
            glBindVertexArray(OpenGLBackEnd::GetCSGVAO());
            glDrawElementsInstancedBaseVertex(GL_TRIANGLES, cube.m_vertexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * cube.m_baseIndex), 5, cube.m_baseVertex);
        }
        else {
            for (RenderItem3D& renderItem : selectionRenderItems) {
                Mesh* mesh = AssetManager::GetMeshByIndex(renderItem.meshIndex);
                shader.SetMat4("model", renderItem.modelMatrix);
                glDrawElementsInstancedBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), 5, mesh->baseVertex);
            }
        }
    }

    // Cleanup
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_TRUE);
    glDisable(GL_STENCIL_TEST);


    // Draw CSG vertices
    if (Editor::ObjectIsSelected()) {
        CubeVolume* cubeVolume = nullptr;
        if (Editor::GetSelectedObjectType() == PhysicsObjectType::CSG_OBJECT_ADDITIVE) {
            cubeVolume = &Scene::g_cubeVolumesAdditive[Editor::GetSelectedObjectIndex()];
        }
        if (Editor::GetSelectedObjectType() == PhysicsObjectType::CSG_OBJECT_SUBTRACTIVE) {
            cubeVolume = &Scene::g_cubeVolumesSubtractive[Editor::GetSelectedObjectIndex()];
        }
        if (cubeVolume) {
            static int meshIndex = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Cube"))->GetMeshIndices()[0];
            Mesh* mesh = AssetManager::GetMeshByIndex(meshIndex);
            shader.SetMat4("model", cubeVolume->GetModelMatrix());
            shader.SetVec3("Color", ORANGE);
            glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
            glPointSize(2);
            glDrawElementsInstancedBaseVertex(GL_POINTS, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), 5, mesh->baseVertex);
        }
    }
}




void CSGSubtractivePass() {

    if (!Editor::IsOpen()) {
        return;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);

    Player* player = Game::GetPlayerByIndex(0);

    Shader& shader = OpenGLRenderer::g_shaders.csgSubtractive;
    shader.Use();
    shader.SetMat4("projection", player->GetProjectionMatrix());
    shader.SetMat4("view", player->GetViewMatrix());
    shader.SetBool("useUniformColor", false);


    std::vector<glm::mat4> planeTransforms;

    for (CubeVolume& cubeVolume : Scene::g_cubeVolumesSubtractive) {

        // cube test
        /*
        static int meshIndex = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Cube"))->GetMeshIndices()[0];
        shader.SetVec3("color", RED);
        shader.SetMat4("model", cubeVolume.GetModelMatrix());
        Mesh* mesh = AssetManager::GetMeshByIndex(meshIndex);
        glDrawElementsBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), mesh->baseVertex);
        */

        Transform translation;
        Transform rotaton;

        translation.position = glm::vec3(0, 0, 0.5f);
        rotaton.rotation = glm::vec3(0, 0, 0);
        planeTransforms.push_back(cubeVolume.GetModelMatrix() * translation.to_mat4() * rotaton.to_mat4());

        // z back
        translation.position = glm::vec3(0, 0, -0.5f);
        rotaton.rotation = glm::vec3(0, HELL_PI, 0);
        planeTransforms.push_back(cubeVolume.GetModelMatrix() * translation.to_mat4() * rotaton.to_mat4());

        // x front
        translation.position = glm::vec3(0.5f, 0, 0);
        rotaton.rotation = glm::vec3(0, HELL_PI * 0.5f, 0);
        planeTransforms.push_back(cubeVolume.GetModelMatrix() * translation.to_mat4() * rotaton.to_mat4());

        // x back
        translation.position = glm::vec3(-0.5f, 0, 0);
        rotaton.rotation = glm::vec3(0, HELL_PI * 1.5f, 0);
        planeTransforms.push_back(cubeVolume.GetModelMatrix() * translation.to_mat4() * rotaton.to_mat4());

        // y top
        translation.position = glm::vec3(0, 0.5f, 0);
        rotaton.rotation = glm::vec3(HELL_PI * 1.5f, 0, 0);
        planeTransforms.push_back(cubeVolume.GetModelMatrix() * translation.to_mat4() * rotaton.to_mat4());

        // y bottom
        translation.position = glm::vec3(0, -0.5f, 0);
        rotaton.rotation = glm::vec3(HELL_PI * 0.5f, 0, 0);
        planeTransforms.push_back(cubeVolume.GetModelMatrix() * translation.to_mat4() * rotaton.to_mat4());
    }

    // sort by view distance to camera
    sort(planeTransforms.begin(), planeTransforms.end(), [](const auto& lhs, const auto& rhs) {
        glm::vec3 viewPos = Game::GetPlayerByIndex(0)->GetViewPos();
        glm::vec3 planeCenterWorldPosA = Util::GetTranslationFromMatrix(lhs);
        glm::vec3 planeCenterWorldPosB = Util::GetTranslationFromMatrix(rhs);
        float distanceA = glm::distance(viewPos, planeCenterWorldPosA);
        float distanceB = glm::distance(viewPos, planeCenterWorldPosB);
        return distanceA > distanceB;
    });

    glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
    for (glm::mat4& matrix : planeTransforms) {
        int planeMeshIndex = AssetManager::GetHalfSizeQuadMeshIndex();
        shader.SetVec3("color", ORANGE);
        shader.SetMat4("model", matrix);
        Mesh* mesh = AssetManager::GetMeshByIndex(planeMeshIndex);
        glDrawElementsBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), mesh->baseVertex);
    }





    /*
    // Render sphere
    Model* model = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Sphere"));
    Mesh* sphereMesh = AssetManager::GetMeshByIndex(model->GetMeshIndices()[0]);

    Transform transform;
    transform.position = glm::vec3(0, 1, 0);
    shader.SetMat4("model", transform.to_mat4());

    struct Triangle {
        glm::vec3 v0;
        glm::vec3 v1;
        glm::vec3 v2;
        glm::vec3 center;
        int baseIndex;
    };

    std::vector<Triangle> triangles;

    for (int i = sphereMesh->baseIndex; i < sphereMesh->baseIndex + sphereMesh->indexCount; i += 3) {
        Triangle& triangle = triangles.emplace_back();

        int idx0 = AssetManager::GetIndices()[i + 0 ] + sphereMesh->baseVertex;
        int idx1 = AssetManager::GetIndices()[i + 1 ] + sphereMesh->baseVertex;
        int idx2 = AssetManager::GetIndices()[i + 2 ] + sphereMesh->baseVertex;

        triangle.v0 = AssetManager::GetVertices()[idx0].position;
        triangle.v1 = AssetManager::GetVertices()[idx1].position;
        triangle.v2 = AssetManager::GetVertices()[idx2].position;
        triangle.baseIndex = i;
        triangle.center = (triangle.v0 + triangle.v1 + triangle.v2) * glm::vec3(0.33333f);
    }

    // sort by view distance to camera
    sort(triangles.begin(), triangles.end(), [](const auto& lhs, const auto& rhs) {
        glm::vec3 viewPos = Game::GetPlayerByIndex(0)->GetViewPos();
        float distanceA = glm::distance(viewPos, lhs.center);
        float distanceB = glm::distance(viewPos, rhs.center);
        return distanceA > distanceB;
    });

    for (Triangle& triangle : triangles) {
        glDrawElementsBaseVertex(GL_TRIANGLES, sphereMesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int)* triangle.baseIndex), sphereMesh->baseVertex);
    }
    */














    // cube outline
    static OpenGLDetachedMesh mesh;
   // if (mesh.GetVertexCount() == 0)



    for (CubeVolume& cubeVolume : Scene::g_cubeVolumesSubtractive) {

        std::vector<Vertex> vertices;

        glm::vec3 lineABegin = cubeVolume.GetModelMatrix() * glm::vec4(-0.5f, 0.5f, 0.5f, 1.0f);
        glm::vec3 lineAEnd = cubeVolume.GetModelMatrix() * glm::vec4(-0.5f, -0.5f, 0.5f, 1.0f);

        glm::vec3 lineBBegin = cubeVolume.GetModelMatrix() * glm::vec4(0.5f, 0.5f, -0.5f, 1.0f);
        glm::vec3 lineBEnd = cubeVolume.GetModelMatrix() * glm::vec4(0.5f, -0.5f, -0.5f, 1.0f);
        glm::vec3 centerA = (lineABegin + lineAEnd) * 0.5f;
        glm::vec3 centerB = (lineBBegin + lineBEnd) * 0.5f;

        glm::vec3 normal = glm::normalize(glm::vec3(1, 0, 1));
        // left top front
        vertices.push_back(Vertex(glm::vec3(-0.5f, 0.5f, 0.5f), normal));
        // left bottom front
        vertices.push_back(Vertex(glm::vec3(-0.5f, -0.5f, 0.5f), normal));

        /*
        glm::vec3 viewPos = ;
        float distanceToA = glm::distance(centerA, viewPos);
        float distanceToB = glm::distance(centerB, viewPos);

        if (distanceToA > distanceToB) {
            vertices.push_back(Vertex(lineABegin));
            vertices.push_back(Vertex(lineAEnd));
        }
        else {
            vertices.push_back(Vertex(lineBBegin));
            vertices.push_back(Vertex(lineBEnd));
        }
        */
        std::vector<uint32_t> indices;
        for (int i = 0; i < vertices.size(); i++) {
            indices.push_back(i);
        }

        mesh.UpdateVertexBuffer(vertices, indices);

        glDisable(GL_DEPTH_TEST);
        glBindVertexArray(mesh.GetVAO());
        shader.SetBool("useUniformColor", true);
        shader.SetVec3("viewPos", Game::GetPlayerByIndex(0)->GetViewPos());
        shader.SetMat4("model", cubeVolume.GetModelMatrix());
        //glDrawElements(GL_LINES, mesh.GetIndexCount(), GL_UNSIGNED_INT, 0);
    }
}

void OpenGLRenderer::RaytracingTestPass(RenderData& renderData) {

    TLAS* tlas = Raytracing::GetTLASByIndex(0);
    if (!tlas) {
        return;
    }
    std::vector<BLASInstance> blasInstaces = Raytracing::GetBLASInstances(0);
    GLFrameBuffer& gBuffer = OpenGLRenderer::g_frameBuffers.gBuffer;
    ComputeShader& computeShader = OpenGLRenderer::g_shaders.raytracingTest;
    computeShader.Use();
    _ssbos.tlasNodes.Update(tlas->GetNodes().size() * sizeof(BVHNode), (void*)&tlas->GetNodes()[0]);
    _ssbos.blasNodes.Update(Raytracing::GetBLSANodes().size() * sizeof(BVHNode), (void*)&Raytracing::GetBLSANodes()[0]);
    _ssbos.blasInstances.Update(blasInstaces.size() * sizeof(BLASInstance), (void*)&blasInstaces[0]);
    _ssbos.triangleIndices.Update(Raytracing::GetTriangleIndices().size() * sizeof(unsigned int), (void*)&Raytracing::GetTriangleIndices()[0]);

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 20, OpenGLBackEnd::GetCSGVBO());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 21, OpenGLBackEnd::GetCSGEBO());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 22, _ssbos.blasNodes.GetHandle());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 23, _ssbos.tlasNodes.GetHandle());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 24, _ssbos.blasInstances.GetHandle());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 25, _ssbos.triangleIndices.GetHandle());
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
    glBindImageTexture(0, gBuffer.GetColorAttachmentHandleByName("FinalLighting"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gBuffer.GetColorAttachmentHandleByName("Normal"));
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gBuffer.GetDepthAttachmentHandle());

    glDispatchCompute(gBuffer.GetWidth() / 16, gBuffer.GetHeight() / 4, 1);
}


void PostProcessingPass(RenderData& renderData) {

    GLFrameBuffer& gBuffer = OpenGLRenderer::g_frameBuffers.gBuffer;
    ComputeShader& computeShader = OpenGLRenderer::g_shaders.postProcessing;
    computeShader.Use();
    glBindImageTexture(0, gBuffer.GetColorAttachmentHandleByName("FinalLighting"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
    glBindImageTexture(1, gBuffer.GetColorAttachmentHandleByName("Normal"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
    glDispatchCompute(gBuffer.GetWidth() / 16, gBuffer.GetHeight() / 4, 1);
}


void OpenGLRenderer::UpdatePointCloud() {

    if (Renderer::GetRenderMode() == RenderMode::DIRECT_LIGHT) {
        return;
    }

    int pointCount = GlobalIllumination::GetPointCloud().size();
    int invocationCount = std::ceil(pointCount / 64.0f);

    static int frameNumber = 0;
    frameNumber++;

    ComputeShader& shader = OpenGLRenderer::g_shaders.pointCloudDirectLigthing;
    shader.Use();
    shader.SetInt("pointCount", pointCount);
    shader.SetInt("frameNumber", frameNumber);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, OpenGLRenderer::_ssbos.lights);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, OpenGLBackEnd::GetPointCloudVBO());

    TLAS* tlas = Raytracing::GetTLASByIndex(0);
    if (tlas) {
        std::vector<BLASInstance> blasInstaces = Raytracing::GetBLASInstances(0);
        _ssbos.tlasNodes.Update(tlas->GetNodes().size() * sizeof(BVHNode), (void*)&tlas->GetNodes()[0]);
        _ssbos.blasNodes.Update(Raytracing::GetBLSANodes().size() * sizeof(BVHNode), (void*)&Raytracing::GetBLSANodes()[0]);
        _ssbos.blasInstances.Update(blasInstaces.size() * sizeof(BLASInstance), (void*)&blasInstaces[0]);
        _ssbos.triangleIndices.Update(Raytracing::GetTriangleIndices().size() * sizeof(unsigned int), (void*)&Raytracing::GetTriangleIndices()[0]);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 20, OpenGLBackEnd::GetCSGVBO());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 21, OpenGLBackEnd::GetCSGEBO());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 22, _ssbos.blasNodes.GetHandle());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 23, _ssbos.tlasNodes.GetHandle());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 24, _ssbos.blasInstances.GetHandle());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 25, _ssbos.triangleIndices.GetHandle());
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
    }

    glDispatchCompute(invocationCount, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}


void OpenGLRenderer::Triangle2DPass() {

    std::vector<glm::vec2> pixelCoords = {
     /* glm::vec2(100.0f, 100.0f),
      glm::vec2(300.0f, 100.0f),
      glm::vec2(200.0f, 300.0f),
      glm::vec2(10, 10),
      glm::vec2(40, 10),
      glm::vec2(40, 40),*/
    };

    Player* player = Game::GetPlayerByIndex(0);
    glm::mat4 mvp = player->GetProjectionMatrix() * player->GetViewMatrix();


    int screenCenterX = PRESENT_WIDTH * 0.5f;
    int screenCenterY = PRESENT_HEIGHT * 0.5f;


    player->_playerName = "";
    glm::vec3 viewPos = player->GetViewPos();
    float minPickUpDistance = 1.5f;

    player->m_pickUpInteractable = false;

    for (int j = 0; j < Scene::GetGamesObjects().size(); j++) {

        GameObject& gameObject = Scene::GetGamesObjects()[j];

        if (gameObject.m_collisionType == CollisionType::PICKUP) {
            if (gameObject.m_convexModelIndex != -1) {
                Model* model = AssetManager::GetModelByIndex(gameObject.m_convexModelIndex);
                Mesh* mesh = AssetManager::GetMeshByIndex(model->GetMeshIndices()[0]);

                // early cull by distance
                glm::vec3 pickUpWorldPos = Util::GetTranslationFromMatrix(gameObject.GetModelMatrix());
                float distanceToPlayer = glm::distance(pickUpWorldPos, viewPos);
                if (distanceToPlayer > 3.0f) {
                    continue;
                }

                for (int i = mesh->baseIndex; i < mesh->baseIndex + mesh->indexCount; i += 3) {

                    int idx0 = AssetManager::GetIndices()[i + 0] + mesh->baseVertex;
                    int idx1 = AssetManager::GetIndices()[i + 1] + mesh->baseVertex;
                    int idx2 = AssetManager::GetIndices()[i + 2] + mesh->baseVertex;

                    glm::vec3 v0 = gameObject.GetModelMatrix() * glm::vec4(AssetManager::GetVertices()[idx0].position, 1.0f);
                    glm::vec3 v1 = gameObject.GetModelMatrix() * glm::vec4(AssetManager::GetVertices()[idx1].position, 1.0f);
                    glm::vec3 v2 = gameObject.GetModelMatrix() * glm::vec4(AssetManager::GetVertices()[idx2].position, 1.0f);

                    // deeper distance cull
                    if (glm::distance(v0, viewPos) > minPickUpDistance ||
                        glm::distance(v1, viewPos) > minPickUpDistance ||
                        glm::distance(v2, viewPos) > minPickUpDistance) {
                        continue;
                    }

                    glm::vec3 d = glm::normalize(player->GetViewPos() - v0);
                    float ndotl = glm::dot(d, player->GetCameraForward());
                    if (ndotl < 0) {
                        continue;
                    }
                    d = glm::normalize(player->GetViewPos() - v1);
                    ndotl = glm::dot(d, player->GetCameraForward());
                    if (ndotl < 0) {
                        continue;
                    }
                    d = glm::normalize(player->GetViewPos() - v2);
                    ndotl = glm::dot(d, player->GetCameraForward());
                    if (ndotl < 0) {
                        continue;
                    }


                    glm::vec2 p0 = Util::CalculateScreenSpaceCoordinates(v0, mvp, PRESENT_WIDTH, PRESENT_HEIGHT);
                    glm::vec2 p1 = Util::CalculateScreenSpaceCoordinates(v1, mvp, PRESENT_WIDTH, PRESENT_HEIGHT);
                    glm::vec2 p2 = Util::CalculateScreenSpaceCoordinates(v2, mvp, PRESENT_WIDTH, PRESENT_HEIGHT);

                    pixelCoords.push_back(p0);
                    pixelCoords.push_back(p1);
                    pixelCoords.push_back(p2);

                    glm::vec2 circleCenter = glm::vec2(screenCenterX, screenCenterY);
                    if (Util::IsTriangleOverlappingCircle(p0, p1, p2, circleCenter, 40)) {
                        //std::cout << "OVERLAP WITH: " << gameObject.model->GetName() << "\n";

                        //player->_playerName = "OVERLAP WITH: " + gameObject.model->GetName();

                        if (Input::KeyPressed(HELL_KEY_E)) {
                            Scene::RemoveGameObjectByIndex(j);
                            Audio::PlayAudio("ItemPickUp.wav", 1.0f);
                          //  player->AddPickUpText("item", 1);

                            for (Light& light : Scene::g_lights) {
                                light.isDirty = true;
                            }

                        }
                        player->m_pickUpInteractable = true;
                    }
                }
            }

        }
    }



    std::vector<glm::vec2> triangleVertices = Util::Generate2DVerticesFromPixelCoords(pixelCoords, PRESENT_WIDTH, PRESENT_HEIGHT);
    OpenGLBackEnd::UploadTriangle2DData(triangleVertices);

    g_shaders.triangles2D.Use();
    glBindVertexArray(OpenGLBackEnd::GetTriangles2DVAO());
    glDrawArrays(GL_TRIANGLES, 0, triangleVertices.size());


    GLFrameBuffer& present = OpenGLRenderer::g_frameBuffers.present;
    g_shaders.debugCircle.Use();
    g_shaders.debugCircle.SetInt("screenCenterX", screenCenterX);
    g_shaders.debugCircle.SetInt("screenCenterY", screenCenterY);
    glBindImageTexture(0, present.GetColorAttachmentHandleByName("Color"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
    glDispatchCompute(present.GetWidth() / 16, present.GetHeight() / 4, 1);

}

void OpenGLRenderer::PresentFinalImage() {
    BlitDstCoords blitDstCoords;
    blitDstCoords.dstX0 = 0;
    blitDstCoords.dstY0 = 0;
    blitDstCoords.dstX1 = BackEnd::GetCurrentWindowWidth();
    blitDstCoords.dstY1 = BackEnd::GetCurrentWindowHeight();
    BlitPlayerPresentTargetToDefaultFrameBuffer(&g_frameBuffers.present, 0, "Color", "", GL_COLOR_BUFFER_BIT, GL_NEAREST, blitDstCoords);
}










void OpenGLRenderer::ProbeGridDebugPass() {

    static int frameNumber = 0;
    frameNumber++;

    LightVolume* lightVolume = GlobalIllumination::GetLightVolumeByIndex(0);
    OpenGLTexture3D& texture3D = lightVolume->texutre3D.GetGLTexture3D();


    static int run10Times = 0;

   if (run10Times < 10) {

        // Calculate probe lightings
        if (Renderer::GetRenderMode() == RenderMode::COMPOSITE ||
            Renderer::GetRenderMode() == RenderMode::COMPOSITE_PLUS_POINT_CLOUD) {

            GLFrameBuffer& gBuffer = OpenGLRenderer::g_frameBuffers.gBuffer;
            ComputeShader& computeShader = OpenGLRenderer::g_shaders.probeLighting;
            computeShader.Use();
            computeShader.SetInt("probeSpaceWidth", lightVolume->GetProbeSpaceWidth());
            computeShader.SetInt("probeSpaceHeight", lightVolume->GetProbeSpaceHeight());
            computeShader.SetInt("probeSpaceDepth", lightVolume->GetProbeSpaceDepth());
            computeShader.SetVec3("lightVolumePosition", lightVolume->GetPosition());
            computeShader.SetFloat("probeSpacing", PROBE_SPACING);
            computeShader.SetInt("cloudPointCount", GlobalIllumination::GetPointCloud().size());
            computeShader.SetInt("frameNumber", frameNumber);

            glBindImageTexture(0, gBuffer.GetColorAttachmentHandleByName("FinalLighting"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
            glBindImageTexture(1, texture3D.GetID(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, OpenGLBackEnd::GetPointCloudVBO());
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 20, OpenGLBackEnd::GetCSGVBO());
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 21, OpenGLBackEnd::GetCSGEBO());
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 22, _ssbos.blasNodes.GetHandle());
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 23, _ssbos.tlasNodes.GetHandle());
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 24, _ssbos.blasInstances.GetHandle());
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 25, _ssbos.triangleIndices.GetHandle());
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);

            int workGroupSize = 4;
            int workGroupX = (lightVolume->GetProbeSpaceWidth() + workGroupSize - 1) / workGroupSize;
            int workGroupY = (lightVolume->GetProbeSpaceHeight() + workGroupSize - 1) / workGroupSize;
            int workGroupZ = (lightVolume->GetProbeSpaceDepth() + workGroupSize - 1) / workGroupSize;
            glDispatchCompute(workGroupX, workGroupY, workGroupZ);
        }
        run10Times++;
    }


   // GLfloat testColor[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
   // glClearTexImage(lightVolume->texutre3D.GetGLTexture3D().GetID(), 0, GL_RGBA, GL_FLOAT, testColor);

    // Draw debug probes
    if (Renderer::ProbesVisible()) {
        static Model* model = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Cube"));
        Mesh* cubeMesh = AssetManager::GetMeshByIndex(model->GetMeshIndices()[0]);
        Player* player = Game::GetPlayerByIndex(0);
        glm::mat4 projection = player->GetProjectionMatrix();
        glm::mat4 view = player->GetViewMatrix();

        g_shaders.debugProbes.Use();
        glBindImageTexture(1, texture3D.GetID(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

        g_shaders.debugProbes.SetMat4("projection", projection);
        g_shaders.debugProbes.SetMat4("view", view);
        g_shaders.debugProbes.SetMat4("model", glm::mat4(1));
        g_shaders.debugProbes.SetVec3("color", RED);

        g_shaders.debugProbes.SetInt("probeSpaceWidth", lightVolume->GetProbeSpaceWidth());
        g_shaders.debugProbes.SetInt("probeSpaceHeight", lightVolume->GetProbeSpaceHeight());
        g_shaders.debugProbes.SetInt("probeSpaceDepth", lightVolume->GetProbeSpaceDepth());
        g_shaders.debugProbes.SetVec3("lightVolumePosition", lightVolume->GetPosition());
        g_shaders.debugProbes.SetFloat("probeSpacing", PROBE_SPACING);

        int instanceCount = lightVolume->GetProbeCount();
        glDrawElementsInstancedBaseVertex(GL_TRIANGLES, cubeMesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * cubeMesh->baseIndex), instanceCount, cubeMesh->baseVertex);
    }



    static Transform transform;
    transform.position = glm::vec3(0, 1, 0);
    transform.scale = glm::vec3(0.5f, 0.5f, 0.5f);
    //transform.rotation.y += 0.1f;

    static Model* model = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Cube"));
    Mesh* cubeMesh = AssetManager::GetMeshByIndex(model->GetMeshIndices()[0]);
    Player* player = Game::GetPlayerByIndex(0);
    glm::mat4 projection = player->GetProjectionMatrix();
    glm::mat4 view = player->GetViewMatrix();

    g_shaders.debugSolidColor.Use();

    g_shaders.debugSolidColor.SetMat4("projection", projection);
    g_shaders.debugSolidColor.SetMat4("view", view);
    g_shaders.debugSolidColor.SetMat4("model", transform.to_mat4());
    g_shaders.debugSolidColor.SetVec3("color", RED);

    // glDrawElementsInstancedBaseVertex(GL_TRIANGLES, cubeMesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * cubeMesh->baseIndex), 1, cubeMesh->baseVertex);

}