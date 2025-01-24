#include "GL_renderer.h"
#include "GL_backEnd.h"
#include "Types/GL_gBuffer.h"
#include "HellCommon.h"
#include "Types/GL_shader.h"
#include "Types/GL_shadowMap.h"
#include "Types/GL_shadowMapArray.h"
#include "Types/GL_ssbo.hpp"
#include "Types/GL_CubeMap2.h"
#include "Types/GL_frameBuffer.hpp"
#include "../../BackEnd/BackEnd.h"
#include "../../Core/AssetManager.h"
#include "../../Game/Scene.h"
#include "../../Game/Game.h"
#include "../../Game/Water.h"
#include "../../Editor/CSG.h"
#include "../../Editor/Editor.h"
#include "../../Editor/Gizmo.hpp"
#include "../../Renderer/GlobalIllumination.h"
#include "../../Renderer/TextBlitter.h"
#include "../../Renderer/RendererStorage.h"
#include "../../Renderer/Renderer.h"
#include "../../Renderer/RendererData.h"
#include "../../Renderer/RendererUtil.hpp"
#include "../../Renderer/Raytracing/Raytracing.h"
#include "../../Renderer/SSAO.hpp"
#include "Timer.hpp"
#include <glm/gtx/matrix_decompose.hpp>

#include "RapidHotload.h"

#define TILE_SIZE 24

struct LightVolumeData {
    glm::vec4 aabbMin;
    glm::vec4 aabbMax;
};

struct TileData {
    int lightCount;
    int lightIndices[127];
};


namespace OpenGLRenderer {

    struct FrameBuffers {
        GLFrameBuffer loadingScreen;
        GLFrameBuffer debugMenu;
        GLFrameBuffer present;
        GLFrameBuffer gBuffer;
        GLFrameBuffer lighting;
        GLFrameBuffer finalFullSizeImage;
        GLFrameBuffer genericRenderTargets;
        GLFrameBuffer megaTexture;
        GLFrameBuffer waterReflection;
        GLFrameBuffer water;
        GLFrameBuffer hair;
    } g_frameBuffers;

    struct BlurFrameBuffers {
        std::vector<GLFrameBuffer> p1;
        std::vector<GLFrameBuffer> p2;
        std::vector<GLFrameBuffer> p3;
        std::vector<GLFrameBuffer> p4;
    } g_blurBuffers;

    struct Shaders {
        Shader geometry;
        Shader geometryAlphaDiscard;
        Shader UI;
        Shader shadowMap;
        Shader shadowMapCSG;
        Shader debugSolidColor;
        Shader debugSolidColor2D;
        Shader debugPointCloud;
        Shader flipBook;
        Shader glass;
        Shader horizontalBlur;
        Shader verticalBlur;
        Shader decalsBlood;
        Shader decalsBullet;
        Shader vatBlood;
        Shader skyBox;
        Shader debugProbes;
        Shader csg;
        Shader outline;
        Shader gbufferSkinned;
        Shader csgSubtractive;
        Shader triangles2D;
        Shader heightMap;
        Shader debugLightVolumeAabb;
        Shader winston;
        Shader megaTextureBloodDecals;
        Shader christmasLightWireShader;
        Shader flipbookNew;

        // Hair
        Shader depthPeelDepth;
        Shader depthPeelColor;
        ComputeShader hairLayerComposite;
        ComputeShader hairFinalComposite;

        // water shaders
        Shader waterReflectionGeometry;
        Shader waterComposite;
        Shader waterMask;
        
        ComputeShader debugTileView;
        ComputeShader lighting;
        ComputeShader ssao;
        ComputeShader ssaoBlur;
        ComputeShader worldPosition;

        Shader lightVolumePrePassGeometry;
        ComputeShader lightVolumeFromCubeMap;
        ComputeShader lightVolumeFromPositionAndRadius;
        ComputeShader lightCulling;

        Shader p90MagFrontFaceLighting;
        Shader p90MagBackFaceLighting;
        ComputeShader p90MagFrontFaceComposite;
        ComputeShader p90MagBackFaceComposite;
        ComputeShader computeTest;

        ComputeShader debugCircle;
        ComputeShader postProcessing;
        ComputeShader glassComposite;
        ComputeShader emissiveComposite;
        ComputeShader pointCloudDirectLigthing;
        ComputeShader computeSkinning;
        ComputeShader raytracingTest;
        ComputeShader probeLighting;
        ComputeShader lightVolumeClear;

        ComputeShader waterColorComposite;
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
        //SSBO playerInstanceDataOffsets;
        SSBO geometryRenderItems;
        SSBO shadowMapGeometryRenderItems;
        SSBO lightVolumeData;
        SSBO tileData;
        SSBO playerData;

        GLuint samplers = 0;
        GLuint renderItems2D = 0;
        GLuint animatedRenderItems3D = 0;
        GLuint animatedTransforms = 0;
        GLuint lights = 0;
        GLuint glassRenderItems = 0;
        //GLuint shadowMapGeometryRenderItems = 0;
        GLuint skinningTransforms = 0;
        GLuint baseAnimatedTransformIndices = 0;
        GLuint cameraData = 0;
        GLuint skinnedMeshInstanceData = 0;
        GLuint muzzleFlashData = 0;
    } g_ssbos;

    GLuint _indirectBuffer = 0;
    ShadowMapArray g_shadowMapArray;
    
    CubeMap2 g_lightVolumePrePassCubeMap;

    OpenGLDetachedMesh gLightVolumeSphereMesh;
    OpenGLDetachedMesh g_sphereMesh;


    //std::vector<ShadowMap> _shadowMaps;

    void RaytracingTestPass(RenderData& renderData);
    void Triangle2DPass();
    void ProbeGridDebugPass();
    void IndirectLightingPass();
    void HeightMapPass(RenderData& renderData);
    void UploadSSBOsGPU(RenderData& renderData);
    void WaterPass(RenderData& renderData);
    void FlipbookPass();
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
void DebugPass(RenderData& renderData);
void MuzzleFlashPass(RenderData& renderData);
void GlassPass(RenderData& renderData);
void DownScaleGBuffer();
void ComputeSkin(RenderData& renderData);
void EmissivePass(RenderData& renderData);
void OutlinePass(RenderData& renderData);
void P90MagPass(RenderData& renderData);
void CSGSubtractivePass();
void LightVolumePrePass(RenderData& renderData);
void LightCullingPass(RenderData& renderData);
void DebugTileViewPass(RenderData& renderData);
void SSAOPass();
void WinstonPass(RenderData& renderData);

void OpenGLRenderer::HotloadShaders() {

    std::cout << "Hotloading shaders...\n";

    // Water shaders
    g_shaders.waterColorComposite.Load("res/shaders/OpenGL/GL_water_composite.comp");
    g_shaders.waterMask.Load("GL_water_mask.vert", "GL_water_mask.frag");
    g_shaders.waterReflectionGeometry.Load("GL_water_reflection_geometry.vert", "GL_water_reflection_geometry.frag");

    g_shaders.christmasLightWireShader.Load("GL_christmas_light_wire.vert", "GL_christmas_light_wire.frag");
    g_shaders.worldPosition.Load("res/shaders/OpenGL/GL_world_position.comp");
    g_shaders.ssaoBlur.Load("res/shaders/OpenGL/GL_ssaoBlur.comp");
    g_shaders.ssao.Load("res/shaders/OpenGL/GL_ssao.comp");
    g_shaders.lighting.Load("res/shaders/OpenGL/GL_lighting.comp");
    g_shaders.debugTileView.Load("res/shaders/OpenGL/GL_debug_tile_view.comp");
    g_shaders.lightVolumeClear.Load("res/shaders/OpenGL/GL_light_volume_clear.comp");
    g_shaders.winston.Load("GL_winston.vert", "GL_winston.frag");
    g_shaders.megaTextureBloodDecals.Load("GL_mega_texture_blood_decals.vert", "GL_mega_texture_blood_decals.frag");

    g_shaders.debugLightVolumeAabb.Load("GL_debug_light_volume_aabb.vert", "GL_debug_light_volume_aabb.frag");
    g_shaders.lightCulling.Load("res/shaders/OpenGL/GL_light_culling.comp");
    g_shaders.lightVolumeFromCubeMap.Load("res/shaders/OpenGL/GL_lightvolume_aabb_from_cubemap.comp");
    g_shaders.lightVolumeFromPositionAndRadius.Load("res/shaders/OpenGL/GL_lightvolume_aabb_from_pos_radius.comp");
    g_shaders.lightVolumePrePassGeometry.Load("GL_lightvolume_prepass_geometry.vert", "GL_lightvolume_prepass_geometry.frag", "GL_lightvolume_prepass_geometry.geom");
    g_shaders.heightMap.Load("GL_heightmap.vert", "GL_heightmap.frag");
    //g_shaders.triangles2D.Load("GL_triangles_2D.vert", "GL_triangles_2D.frag");
    g_shaders.csgSubtractive.Load("GL_csg_subtractive.vert", "GL_csg_subtractive.frag");
    g_shaders.csg.Load("GL_csg_test.vert", "GL_csg_test.frag");
    g_shaders.outline.Load("GL_outline.vert", "GL_outline.frag");
    g_shaders.UI.Load("GL_ui.vert", "GL_ui.frag");
    g_shaders.geometry.Load("GL_gbuffer.vert", "GL_gbuffer.frag");
    g_shaders.geometryAlphaDiscard.Load("GL_gbuffer.vert", "GL_gbuffer_alpha_discarded.frag");
    g_shaders.shadowMap.Load("GL_shadowMap.vert", "GL_shadowMap.frag");
    g_shaders.shadowMapCSG.Load("GL_shadowMap_csg.vert", "GL_shadowMap_csg.frag");
    g_shaders.flipBook.Load("GL_flipBook.vert", "GL_flipBook.frag");
    g_shaders.glass.Load("GL_glass.vert", "GL_glass.frag");
    g_shaders.horizontalBlur.Load("GL_blurHorizontal.vert", "GL_blur.frag");
    g_shaders.verticalBlur.Load("GL_blurVertical.vert", "GL_blur.frag");
    g_shaders.decalsBlood.Load("GL_decals_blood.vert", "GL_decals_blood.frag");
    g_shaders.decalsBullet.Load("GL_decals_bullet.vert", "GL_decals_bullet.frag");
    g_shaders.vatBlood.Load("GL_bloodVAT.vert", "GL_bloodVAT.frag");
    g_shaders.skyBox.Load("GL_skybox.vert", "GL_skybox.frag");
    g_shaders.debugSolidColor.Load("GL_debug_solid_color.vert", "GL_debug_solid_color.frag");
    g_shaders.debugSolidColor2D.Load("GL_debug_solid_color_2D.vert", "GL_debug_solid_color_2D.frag");
    g_shaders.debugPointCloud.Load("GL_debug_pointCloud.vert", "GL_debug_pointCloud.frag");
    g_shaders.debugProbes.Load("GL_debug_probes.vert", "GL_debug_probes.frag");
    g_shaders.gbufferSkinned.Load("GL_gbuffer_skinned.vert", "GL_gbuffer_skinned.frag");
    g_shaders.p90MagFrontFaceLighting.Load("GL_p90_mag_frontface_lighting.vert", "GL_p90_mag_frontface_lighting.frag");
    g_shaders.p90MagBackFaceLighting.Load("GL_p90_mag_backface_lighting.vert", "GL_p90_mag_backface_lighting.frag");
    g_shaders.p90MagFrontFaceComposite.Load("res/shaders/OpenGL/GL_p90_mag_frontface_composite.comp");
    g_shaders.p90MagBackFaceComposite.Load("res/shaders/OpenGL/GL_p90_mag_backface_composite.comp");
    g_shaders.emissiveComposite.Load("res/shaders/OpenGL/GL_emissiveComposite.comp");
    g_shaders.postProcessing.Load("res/shaders/OpenGL/GL_postProcessing.comp");
    g_shaders.glassComposite.Load("res/shaders/OpenGL/GL_glassComposite.comp");
    g_shaders.pointCloudDirectLigthing.Load("res/shaders/OpenGL/GL_pointCloudDirectLighting.comp");
    g_shaders.computeSkinning.Load("res/shaders/OpenGL/GL_computeSkinning.comp");
    g_shaders.raytracingTest.Load("res/shaders/OpenGL/GL_raytracing_test.comp");
    g_shaders.debugCircle.Load("res/shaders/OpenGL/GL_debug_circle.comp");
    g_shaders.probeLighting.Load("res/shaders/OpenGL/GL_probe_lighting.comp");
    g_shaders.computeTest.Load("res/shaders/OpenGL/GL_compute_test.comp");
    g_shaders.depthPeelDepth.Load("GL_depth_peel_depth.vert", "GL_depth_peel_depth.frag");
    g_shaders.depthPeelColor.Load("GL_depth_peel_color.vert", "GL_depth_peel_color.frag");
    g_shaders.hairLayerComposite.Load("res/shaders/OpenGL/GL_hair_layer_composite.comp");
    g_shaders.hairFinalComposite.Load("res/shaders/OpenGL/GL_hair_final_composite.comp");
    g_shaders.flipbookNew.Load("GL_flipbook_new.vert", "GL_flipbook_new.frag");  
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
    g_frameBuffers.gBuffer.CreateAttachment("P90MagDirectLighting", GL_RGBA8);
    g_frameBuffers.gBuffer.CreateAttachment("P90MagSpecular", GL_RGBA8);
    g_frameBuffers.gBuffer.CreateDepthAttachment(GL_DEPTH32F_STENCIL8);

    //g_frameBuffers.genericRenderTargets.Create("GenericRenderTargets", presentWidth * 2, presentHeight * 2);
    //g_frameBuffers.genericRenderTargets.CreateAttachment("WorldSpacePosition", GL_RGBA16F);
    //g_frameBuffers.genericRenderTargets.CreateAttachment("SSAO", GL_R32F);
    //g_frameBuffers.genericRenderTargets.CreateAttachment("SSAOBlur", GL_R32F);

    g_frameBuffers.water.Create("Water", presentWidth * 2, presentHeight * 2);
    g_frameBuffers.water.CreateAttachment("WorldPosXZ", GL_RG32F);
    g_frameBuffers.water.CreateAttachment("Mask", GL_R8);
    g_frameBuffers.water.CreateAttachment("Color", GL_RGBA8);

    g_frameBuffers.waterReflection.Create("Water", PRESENT_WIDTH * 0.5f, PRESENT_HEIGHT * 0.5f);
    g_frameBuffers.waterReflection.CreateAttachment("Color", GL_RGBA8);
    g_frameBuffers.waterReflection.CreateDepthAttachment(GL_DEPTH_COMPONENT24); // maybe even GL_DEPTH_COMPONENT16

    float hairDownscaleRatio = 1.0f;
    g_frameBuffers.hair.Create("Hair", g_frameBuffers.gBuffer.GetWidth() * hairDownscaleRatio, g_frameBuffers.gBuffer.GetHeight() * hairDownscaleRatio);
    g_frameBuffers.hair.CreateDepthAttachment(GL_DEPTH32F_STENCIL8);
    g_frameBuffers.hair.CreateAttachment("Color", GL_RGBA8);
    g_frameBuffers.hair.CreateAttachment("ViewspaceDepth", GL_R32F);
    g_frameBuffers.hair.CreateAttachment("ViewspaceDepthPrevious", GL_R32F);
    g_frameBuffers.hair.CreateAttachment("Composite", GL_RGBA8);
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

    //QueryAvaliability();
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

    glCreateBuffers(1, &g_ssbos.animatedRenderItems3D);
    glNamedBufferStorage(g_ssbos.animatedRenderItems3D, MAX_RENDER_OBJECTS_3D * sizeof(RenderItem3D), NULL, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &g_ssbos.renderItems2D);
    glNamedBufferStorage(g_ssbos.renderItems2D, MAX_RENDER_OBJECTS_2D * sizeof(RenderItem2D), NULL, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &g_ssbos.lights);
    glNamedBufferStorage(g_ssbos.lights, MAX_LIGHTS * sizeof(GPULight), NULL, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &g_ssbos.animatedTransforms);
    glNamedBufferStorage(g_ssbos.animatedTransforms, MAX_ANIMATED_TRANSFORMS * sizeof(glm::mat4), NULL, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &g_ssbos.glassRenderItems);
    glNamedBufferStorage(g_ssbos.glassRenderItems, MAX_GLASS_MESH_COUNT * sizeof(RenderItem3D), NULL, GL_DYNAMIC_STORAGE_BIT);

    g_ssbos.bulletHoleDecalRenderItems.PreAllocate(MAX_DECAL_COUNT * sizeof(RenderItem3D));
    g_ssbos.bloodDecalRenderItems.PreAllocate(MAX_BLOOD_DECAL_COUNT * sizeof(RenderItem3D));
    g_ssbos.bloodVATRenderItems.PreAllocate(MAX_VAT_INSTANCE_COUNT * sizeof(RenderItem3D));

    g_ssbos.geometryRenderItems.PreAllocate(MAX_RENDER_OBJECTS_3D * sizeof(RenderItem3D));
    g_ssbos.shadowMapGeometryRenderItems.PreAllocate(MAX_RENDER_OBJECTS_3D * sizeof(RenderItem3D));

    glCreateBuffers(1, &g_ssbos.skinningTransforms);
    glNamedBufferStorage(g_ssbos.skinningTransforms, MAX_ANIMATED_TRANSFORMS * sizeof(glm::mat4), NULL, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &g_ssbos.baseAnimatedTransformIndices);
    glNamedBufferStorage(g_ssbos.baseAnimatedTransformIndices, MAX_ANIMATED_TRANSFORMS * sizeof(uint32_t), NULL, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &g_ssbos.cameraData);
    glNamedBufferStorage(g_ssbos.cameraData, sizeof(CameraData) * 4, NULL, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &g_ssbos.skinnedMeshInstanceData);
    glNamedBufferStorage(g_ssbos.skinnedMeshInstanceData, sizeof(SkinnedRenderItem) * 4 * MAX_SKINNED_MESH, NULL, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &g_ssbos.muzzleFlashData);
    glNamedBufferStorage(g_ssbos.muzzleFlashData, sizeof(MuzzleFlashData) * 4, NULL, GL_DYNAMIC_STORAGE_BIT);

    g_shadowMapArray.Init(MAX_SHADOW_CASTING_LIGHTS);
    g_lightVolumePrePassCubeMap.Init(LIGHT_VOLUME_AABB_COLOR_MAP_SIZE);
    g_ssbos.lightVolumeData.PreAllocate(MAX_LIGHTS* sizeof(LightVolumeData));

    GLFrameBuffer& gBuffer = g_frameBuffers.gBuffer;
    int tileXCount = gBuffer.GetWidth() / 12;
    int tileYCount = gBuffer.GetHeight() / 12;
    int tileCount = tileXCount * tileYCount;
    //std::cout << "Tile count: " << tileCount << "\n";
    g_ssbos.tileData.PreAllocate(tileCount * sizeof(TileData));   

}

void OpenGLRenderer::BindBindlessTextures() {
    // Create the samplers SSBO if needed
    if (g_ssbos.samplers == 0) {
        glCreateBuffers(1, &g_ssbos.samplers);
        glNamedBufferStorage(g_ssbos.samplers, TEXTURE_ARRAY_SIZE * sizeof(glm::uvec2), NULL, GL_DYNAMIC_STORAGE_BIT);
    }
    // Get the handles and stash em in a vector
    std::vector<GLuint64> samplers;
    samplers.reserve(AssetManager::GetTextureCount());
    for (int i = 0; i < AssetManager::GetTextureCount(); i++) {
        samplers.push_back(AssetManager::GetTextureByIndex(i)->GetGLTexture().GetBindlessID());
    }
    // Send to GPU
    glNamedBufferSubData(g_ssbos.samplers, 0, samplers.size() * sizeof(glm::uvec2), &samplers[0]);
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

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, g_ssbos.samplers);

    RenderUI(renderItems, g_frameBuffers.loadingScreen, true);
    BlitFrameBuffer(&g_frameBuffers.loadingScreen, 0, "Color", "", GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

void OpenGLRenderer::UploadSSBOsGPU(RenderData& renderData) {

    glNamedBufferSubData(g_ssbos.lights, 0, RendererData::g_gpuLights.size() * sizeof(GPULight), &RendererData::g_gpuLights[0]);
    glNamedBufferSubData(g_ssbos.glassRenderItems, 0, renderData.glassDrawInfo.renderItems.size() * sizeof(RenderItem3D), &renderData.glassDrawInfo.renderItems[0]);
    glNamedBufferSubData(g_ssbos.skinningTransforms, 0, renderData.skinningTransforms.size() * sizeof(glm::mat4), &renderData.skinningTransforms[0]);
    glNamedBufferSubData(g_ssbos.baseAnimatedTransformIndices, 0, renderData.baseAnimatedTransformIndices.size() * sizeof(uint32_t), &renderData.baseAnimatedTransformIndices[0]);
    glNamedBufferSubData(g_ssbos.cameraData, 0, 4 * sizeof(CameraData), &renderData.cameraData[0]);
    glNamedBufferSubData(g_ssbos.skinnedMeshInstanceData, 0, sizeof(SkinnedRenderItem) * renderData.allSkinnedRenderItems.size(), &renderData.allSkinnedRenderItems[0]);
    glNamedBufferSubData(g_ssbos.muzzleFlashData, 0, sizeof(MuzzleFlashData) * 4, &renderData.muzzleFlashData[0]);    
    g_ssbos.bloodDecalRenderItems.Update(renderData.bloodDecalDrawInfo.renderItems.size() * sizeof(RenderItem3D), renderData.bloodDecalDrawInfo.renderItems.data());
    g_ssbos.bloodVATRenderItems.Update(renderData.bloodVATDrawInfo.renderItems.size() * sizeof(RenderItem3D), renderData.bloodVATDrawInfo.renderItems.data());
    g_ssbos.materials.Update(AssetManager::GetGPUMaterials().size() * sizeof(GPUMaterial), &AssetManager::GetGPUMaterials()[0]);

    g_ssbos.bulletHoleDecalRenderItems.Update(RendererData::g_bulletDecalRenderItems.size() * sizeof(RenderItem3D), &RendererData::g_bulletDecalRenderItems[0]);    
    
    g_ssbos.shadowMapGeometryRenderItems.Update(RendererData::g_shadowMapGeometryRenderItems.size() * sizeof(RenderItem3D), &RendererData::g_shadowMapGeometryRenderItems[0]);
    g_ssbos.playerData.Update(Game::g_playerData.size() * sizeof(PlayerData), &Game::g_playerData[0]);
}

void MegaTextureTestPass() {

    glClipControl(GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE);

    GLFrameBuffer& megaTextureFBO = OpenGLRenderer::g_frameBuffers.megaTexture;
    HeightMap& heightMap = AssetManager::g_heightMap;

    // Create FBO and render target
    if (megaTextureFBO.GetHandle() == 0) {
        int width = heightMap.m_width * 100;
        int height = heightMap.m_depth * 100;
        megaTextureFBO.Create("MegaTextureFBO", width, height);
        megaTextureFBO.CreateAttachment("Color", GL_R8);
        megaTextureFBO.Bind();
        megaTextureFBO.SetViewport();
        glDrawBuffer(megaTextureFBO.GetColorAttachmentSlotByName("Color"));
        glClearColor(0.0, 0.0, 0.0, 0.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    megaTextureFBO.Bind();
    megaTextureFBO.SetViewport(); 
    glDrawBuffer(megaTextureFBO.GetColorAttachmentSlotByName("Color"));
    //glClearColor(0.25, 0.25, 0.25, 1);
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    

    glm::vec3 centerPoint = heightMap.GetWorldSpaceCenter();
    float width = heightMap.m_width * heightMap.m_transform.scale.x;
    float height = heightMap.m_depth * heightMap.m_transform.scale.z;
    float nearPlane = 0.1f;
    float farPlane = 5000.0f;
    float left = centerPoint.x - width / 2.0f;
    float right = centerPoint.x + width / 2.0f;
    float bottom = centerPoint.z + height / 2.0f;
    float top = centerPoint.z - height / 2.0f;
    glm::vec3 cameraPosition = centerPoint + glm::vec3(0.0f, 50.0f, 0.0f);
    glm::vec3 cameraTarget = centerPoint;
    glm::vec3 upVector = glm::vec3(0.0f, 0.0f, -1.0f);

    glm::mat4 projection = glm::ortho(left, right, bottom, top, nearPlane, farPlane);
    glm::mat4 view = glm::lookAt(cameraPosition, cameraTarget, upVector);
  
    // Render decals
    
    
    
    
    glm::mat4 model = heightMap.m_transform.to_mat4();
    Shader& shader = OpenGLRenderer::g_shaders.megaTextureBloodDecals;
    shader.Use();
    shader.SetMat4("projection", projection);
    shader.SetMat4("view", view);
    shader.SetMat4("model", model);
    shader.SetFloat("heightMapWidth", heightMap.m_width);
    shader.SetFloat("heightMapDepth", heightMap.m_depth);
    shader.SetBool("useUniformColor", true);
    shader.SetVec3("uniformColor", RED);

    glBindVertexArray(heightMap.m_VAO);
    //glDrawElements(GL_TRIANGLE_STRIP, heightMap.m_indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    static int textureIndexType0 = AssetManager::GetTextureIndexByName("blood_decal_4");
    static int textureIndexType1 = AssetManager::GetTextureIndexByName("blood_decal_6");
    static int textureIndexType2 = AssetManager::GetTextureIndexByName("blood_decal_7");
    static int textureIndexType3 = AssetManager::GetTextureIndexByName("blood_decal_9");

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

    for (BloodDecal& bloodDecal : Scene::g_bloodDecalsForMegaTexture) {
        shader.SetMat4("model", bloodDecal.modelMatrix);
        shader.SetMat4("normalMatrix", glm::transpose(glm::inverse(heightMap.m_transform.to_mat4())));
        if (bloodDecal.type == 0) {
            shader.SetInt("textureIndex", textureIndexType0);
        }
        else if (bloodDecal.type == 1) {
            shader.SetInt("textureIndex", textureIndexType1);
        }
        else if (bloodDecal.type == 2) {
            shader.SetInt("textureIndex", textureIndexType2);
        }
        else if (bloodDecal.type == 3) {
            shader.SetInt("textureIndex", textureIndexType3);
        }


        Mesh* mesh = AssetManager::GetMeshByIndex(AssetManager::GetUpFacingPlaneMeshIndex());
        glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
        glDrawElementsInstancedBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), 1, mesh->baseVertex);      
    }


    Scene::g_bloodDecalsForMegaTexture.clear()


;




/*
    Player* player = Game::GetPlayerByIndex(0);

    if (Input::KeyPressed(HELL_KEY_SPACE)) {

        Transform transform;
        transform.position = player->GetFeetPosition();
        transform.scale = glm::vec3(1);
        model = transform.to_mat4();
        shader.SetMat4("model", model);
        shader.SetMat4("normalMatrix", glm::transpose(glm::inverse(heightMap.m_transform.to_mat4())));

        static int cubeMeshIndex = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Cube"))->GetMeshIndices()[0];
        Mesh* mesh = AssetManager::GetMeshByIndex(cubeMeshIndex);
        glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
        glDrawElementsInstancedBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), 1, mesh->baseVertex);
    }*/

    glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
}

void OpenGLRenderer::RenderFrame(RenderData& renderData) {

    GLFrameBuffer& gBuffer = g_frameBuffers.gBuffer;
    GLFrameBuffer& present = g_frameBuffers.present;

    OpenGLRenderer::UploadSSBOsGPU(renderData);

    // Update GI 
    if (GlobalIllumination::GetFrameCounter() < 9) {
        if (GlobalIllumination::GetFrameCounter() < 9) {
            UpdatePointCloud();
        }
        IndirectLightingPass();
        GlobalIllumination::IncrementFrameCounter();
        //std::cout << "Updating GI\n";
    }

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, g_ssbos.samplers);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, g_ssbos.geometryRenderItems.GetHandle());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, g_ssbos.lights);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, g_ssbos.materials.GetHandle());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, g_ssbos.animatedTransforms);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, g_ssbos.animatedRenderItems3D);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, g_ssbos.glassRenderItems);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 12, g_ssbos.shadowMapGeometryRenderItems.GetHandle());

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 13, g_ssbos.bulletHoleDecalRenderItems.GetHandle());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 14, g_ssbos.bloodDecalRenderItems.GetHandle());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 15, g_ssbos.bloodVATRenderItems.GetHandle());

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 16, g_ssbos.cameraData);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 17, g_ssbos.skinnedMeshInstanceData);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 18, g_ssbos.muzzleFlashData);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 19, g_ssbos.lightVolumeData.GetHandle());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 20, g_ssbos.tileData.GetHandle());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 21, g_ssbos.playerData.GetHandle());

    ClearRenderTargets();    
    RenderShadowMapss(renderData);
    LightVolumePrePass(renderData);
    ComputeSkin(renderData);

    MegaTextureTestPass();

    GeometryPass(renderData);
    HeightMapPass(renderData);
    DrawVATBlood(renderData);
    DrawBloodDecals(renderData);
    DrawBulletDecals(renderData);
    LightCullingPass(renderData);
    LightingPass(renderData);

    SkyBoxPass(renderData);
    WaterPass(renderData);
    HairPass();

    //SSAOPass();
    DebugTileViewPass(renderData);

    // DebugPassProbePass(renderData);
    P90MagPass(renderData);
    MuzzleFlashPass(renderData);
    FlipbookPass();
    GlassPass(renderData);
    EmissivePass(renderData);
    WinstonPass(renderData);
    MuzzleFlashPass(renderData);
    PostProcessingPass(renderData);

    ProbeGridDebugPass();


    RenderUI(renderData.renderItems2DHiRes, g_frameBuffers.gBuffer, false);
    DownScaleGBuffer();
    CSGSubtractivePass();
    OutlinePass(renderData);
    DebugPass(renderData);
    if (Editor::ObjectIsSelected()) {
        Gizmo::Draw(renderData.cameraData[0].projection, renderData.cameraData[0].view, g_frameBuffers.present.GetWidth(), g_frameBuffers.present.GetHeight());
    }
    Triangle2DPass();
    RenderUI(renderData.renderItems2D, g_frameBuffers.present, false);
   
}


void OpenGLRenderer::WaterPass(RenderData& renderData) {

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 16, g_ssbos.cameraData);

    GLFrameBuffer& gBuffer = OpenGLRenderer::g_frameBuffers.gBuffer;
    GLFrameBuffer& waterFrameBuffer = OpenGLRenderer::g_frameBuffers.water; 
    GLFrameBuffer& reflectionFBO = OpenGLRenderer::g_frameBuffers.waterReflection;

    reflectionFBO.Bind();
    reflectionFBO.DrawBuffers({ "Color" });

    // Render skybox into reflection texture
    static CubemapTexture* cubemapTexture = AssetManager::GetCubemapTextureByIndex(AssetManager::GetCubemapTextureIndexByName("NightSky"));
    static Mesh* mesh = AssetManager::GetMeshByIndex(AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Cube"))->GetMeshIndices()[0]);
    reflectionFBO.Bind();
    reflectionFBO.DrawBuffers({ "Color" });
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    Shader* skyboxShader = &g_shaders.skyBox;
    skyboxShader->Use();

    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (int i = 0; i < renderData.playerCount; i++) {
        Player* player = Game::GetPlayerByIndex(0);
        Transform skyBoxTransform;
        skyBoxTransform.position = player->GetViewPos();
        skyBoxTransform.scale = glm::vec3(FAR_PLANE * 0.99);
        ViewportInfo viewportInfo = RendererUtil::CreateViewportInfo(i, Game::GetSplitscreenMode(), reflectionFBO.GetWidth(), reflectionFBO.GetHeight());
        SetViewport(viewportInfo);
        // Draw
        Shader& shader = OpenGLRenderer::g_shaders.skyBox;
        skyboxShader->SetInt("playerIndex", i);
        skyboxShader->SetMat4("projection", Game::GetPlayerByIndex(i)->GetProjectionMatrix());
        skyboxShader->SetMat4("view", Game::GetPlayerByIndex(i)->GetWaterReflectionViewMatrix());
        skyboxShader->SetVec3("skyboxTint", Game::GameSettings().skyBoxTint * glm::vec3(0.45f));
        skyboxShader->SetMat4("model", skyBoxTransform.to_mat4());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture->GetGLTexture().GetID());
        glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
        glDrawElementsBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), mesh->baseVertex);
    }

    // Render reflection geometry
    Shader& reflectionGeometryShader = OpenGLRenderer::g_shaders.waterReflectionGeometry;
    reflectionGeometryShader.Use();
    reflectionGeometryShader.SetVec4("clippingPlane", glm::vec4(0, 1, 0, -Water::GetHeight()));
    glEnable(GL_CLIP_DISTANCE0);
    glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
    Frustum reflectionFrustum;
    for (int i = 0; i < renderData.playerCount; i++) {
        // Reflection Frustum
        glm::mat4 projectionView = Game::GetPlayerByIndex(i)->GetProjectionMatrix() * Game::GetPlayerByIndex(i)->GetWaterReflectionViewMatrix();
        reflectionFrustum.Update(projectionView);
        // Ok draw now
        reflectionGeometryShader.SetInt("playerIndex", i);
        reflectionGeometryShader.SetMat4("projection", Game::GetPlayerByIndex(i)->GetProjectionMatrix());
        reflectionGeometryShader.SetMat4("view", Game::GetPlayerByIndex(i)->GetWaterReflectionViewMatrix());
        reflectionGeometryShader.SetVec3("viewPos", Game::GetPlayerByIndex(i)->GetViewPos());
        ViewportInfo viewportInfo = RendererUtil::CreateViewportInfo(i, Game::GetSplitscreenMode(), reflectionFBO.GetWidth(), reflectionFBO.GetHeight());
        SetViewport(viewportInfo);
        glViewport(viewportInfo.xOffset, viewportInfo.yOffset, viewportInfo.width, viewportInfo.height);
        // Render all static mesh
        for (RenderItem3D& renderItem : Scene::GetGeometryRenderItems()) {            
            if (reflectionFrustum.IntersectsAABBFast(renderItem)) {
                reflectionGeometryShader.SetMat4("model", renderItem.modelMatrix);
                reflectionGeometryShader.SetMat4("inverseModel", renderItem.inverseModelMatrix);
                reflectionGeometryShader.SetInt("baseColorIndex", renderItem.baseColorTextureIndex);
                reflectionGeometryShader.SetInt("normalMapIndex", renderItem.normalMapTextureIndex);
                reflectionGeometryShader.SetInt("rmaIndex", renderItem.rmaTextureIndex);
                Mesh* mesh = AssetManager::GetMeshByIndex(renderItem.meshIndex);
                glDrawElementsInstancedBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), 1, mesh->baseVertex);
            }
        }
        std::vector<HairRenderItem> hairTopLayerRenderItems = Scene::GetHairTopLayerRenderItems();
        std::vector<HairRenderItem> hairBottomLayerRenderItems = Scene::GetHairBottomLayerRenderItems();
        std::vector<HairRenderItem> hairRenderItems;
        hairRenderItems.insert(std::end(hairRenderItems), std::begin(hairTopLayerRenderItems), std::end(hairTopLayerRenderItems));
        hairRenderItems.insert(std::end(hairRenderItems), std::begin(hairBottomLayerRenderItems), std::end(hairBottomLayerRenderItems));
        for (HairRenderItem& hairRenderItem : hairRenderItems) {
            Mesh* mesh = AssetManager::GetMeshByIndex(hairRenderItem.meshIndex);
            Material* material = AssetManager::GetMaterialByIndex(hairRenderItem.materialIndex);
            reflectionGeometryShader.SetMat4("model", hairRenderItem.modelMatrix);
            reflectionGeometryShader.SetMat4("inverseModel", glm::inverse(hairRenderItem.modelMatrix));
            reflectionGeometryShader.SetInt("baseColorIndex", material->_basecolor);
            reflectionGeometryShader.SetInt("normalMapIndex", material->_normal);
            reflectionGeometryShader.SetInt("rmaIndex", material->_rma);
            glDrawElementsInstancedBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), 1, mesh->baseVertex);
        }

        // Constructive Solid Geometry
        reflectionGeometryShader.SetMat4("model", glm::mat4(1));
        reflectionGeometryShader.SetMat4("inverseModel", glm::inverse(glm::mat4(1)));
        glBindVertexArray(OpenGLBackEnd::GetCSGVAO());
        std::vector<CSGObject>& cubes = CSG::GetCSGObjects();
        for (int j = 0; j < cubes.size(); j++) {
            CSGObject& cube = cubes[j];
            if (cube.m_disableRendering) {
                continue;
            }
            Material* material = AssetManager::GetMaterialByIndex(cube.m_materialIndex);
            if (material) {
                reflectionGeometryShader.SetInt("baseColorIndex", material->_basecolor);
                reflectionGeometryShader.SetInt("normalMapIndex", material->_normal);
                reflectionGeometryShader.SetInt("rmaIndex", material->_rma);
                glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLES, cube.m_vertexCount, GL_UNSIGNED_INT, (void*)(sizeof(uint32_t) * cube.m_baseIndex), 1, cube.m_baseVertex, j);
            }
        }
    }
    glDisable(GL_BLEND);



    // Render heightmap into reflection texture
    HeightMap& heightMap = AssetManager::g_heightMap;
    GLFrameBuffer& megaTextureFBO = OpenGLRenderer::g_frameBuffers.megaTexture;
    reflectionGeometryShader.Use();
    reflectionGeometryShader.SetVec4("clippingPlane", glm::vec4(0, 1, 0, -Water::GetHeight()));
    unsigned int attachments[3] = {
        GL_COLOR_ATTACHMENT0,
        GL_COLOR_ATTACHMENT1,
        GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByName("Ground_MudVeg_ALB")->GetGLTexture().GetID());
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByName("Ground_MudVeg_NRM")->GetGLTexture().GetID());
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByName("Ground_MudVeg_RMA")->GetGLTexture().GetID());
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, megaTextureFBO.GetColorAttachmentHandleByName("Color"));
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByName("TreeMap")->GetGLTexture().GetID());
    for (int i = 0; i < renderData.playerCount; i++) {
        Player* player = Game::GetPlayerByIndex(i);
        ViewportInfo viewportInfo = RendererUtil::CreateViewportInfo(i, Game::GetSplitscreenMode(), reflectionFBO.GetWidth(), reflectionFBO.GetHeight());
        SetViewport(viewportInfo);
        glViewport(viewportInfo.xOffset, viewportInfo.yOffset, viewportInfo.width, viewportInfo.height);
        reflectionGeometryShader.SetInt("playerIndex", i);
        reflectionGeometryShader.SetMat4("projection", Game::GetPlayerByIndex(i)->GetProjectionMatrix());
        reflectionGeometryShader.SetMat4("view", Game::GetPlayerByIndex(i)->GetWaterReflectionViewMatrix());
        reflectionGeometryShader.SetMat4("model", heightMap.m_transform.to_mat4());
        glBindVertexArray(heightMap.m_VAO);
        glDrawElements(GL_TRIANGLE_STRIP, heightMap.m_indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }



    glDisable(GL_CLIP_DISTANCE0);

    // Clear render targets
    waterFrameBuffer.Bind();
    //waterFrameBuffer.DrawBuffers({ "Mask", "Color" });
    waterFrameBuffer.DrawBuffers({ "WorldPosXZ", "Color", "Mask" });
    glClear(GL_COLOR_BUFFER_BIT);

    // Render water mask
    waterFrameBuffer.Bind();
    waterFrameBuffer.DrawBuffers({ "WorldPosXZ", "Mask" });
    waterFrameBuffer.BindExternalDepthBuffer(gBuffer.GetDepthAttachmentHandle());
    glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    Shader& shader = OpenGLRenderer::g_shaders.waterMask;
    shader.Use();
    shader.SetMat4("model", Water::GetModelMatrix());
    shader.SetFloat("waterHeight", Water::GetHeight());
    for (int i = 0; i < renderData.playerCount; i++) {
        ViewportInfo viewportInfo = RendererUtil::CreateViewportInfo(i, Game::GetSplitscreenMode(), gBuffer.GetWidth(), gBuffer.GetHeight());
        SetViewport(viewportInfo);
        shader.SetInt("playerIndex", i);
        shader.SetMat4("projection", renderData.cameraData[i].projection);
        shader.SetMat4("view", renderData.cameraData[i].view);
        Mesh* meshU = AssetManager::GetMeshByIndex(AssetManager::GetUpFacingPlaneMeshIndex());
        Mesh* meshD = AssetManager::GetMeshByIndex(AssetManager::GetDownFacingPlaneMeshIndex());
        glDrawElementsInstancedBaseVertex(GL_TRIANGLES, meshU->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * meshU->baseIndex), 1, meshU->baseVertex);
        //glDrawElementsInstancedBaseVertex(GL_TRIANGLES, meshD->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * meshD->baseIndex), 1, meshD->baseVertex);
    }
    //glFinish();
    waterFrameBuffer.UnbindDepthBuffer();
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glBindVertexArray(0);

    // Render water color
    ComputeShader& computeShader = OpenGLRenderer::g_shaders.waterColorComposite;
    computeShader.Use();
    computeShader.SetFloat("viewportWidth", gBuffer.GetWidth());
    computeShader.SetFloat("viewportHeight", gBuffer.GetHeight());
    computeShader.SetFloat("time", Game::GetTime());
    computeShader.SetFloat("waterHeight", Water::GetHeight());
    glBindImageTexture(0, waterFrameBuffer.GetColorAttachmentHandleByName("Color"), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, waterFrameBuffer.GetColorAttachmentHandleByName("WorldPosXZ"));
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gBuffer.GetColorAttachmentHandleByName("FinalLighting"));
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, gBuffer.GetDepthAttachmentHandle());
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByName("WaterNormals")->GetGLTexture().GetID());
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByName("WaterDUDV")->GetGLTexture().GetID());
    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, reflectionFBO.GetColorAttachmentHandleByName("Color"));
    glActiveTexture(GL_TEXTURE7);
    glBindTexture(GL_TEXTURE_2D, waterFrameBuffer.GetColorAttachmentHandleByName("Mask"));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gBuffer.GetColorAttachmentHandleByName("Normal"));
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
    glDispatchCompute(gBuffer.GetWidth() / 16, gBuffer.GetHeight() / 4, 1);


    BlitFrameBuffer(&waterFrameBuffer, &gBuffer, "Color", "FinalLighting", GL_COLOR_BUFFER_BIT, GL_LINEAR);

    // Cleanup
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}









void ClearRenderTargets() {
    GLFrameBuffer& gBuffer = OpenGLRenderer::g_frameBuffers.gBuffer;
    gBuffer.Bind();
    gBuffer.SetViewport();
    unsigned int attachments[8] = {
        GL_COLOR_ATTACHMENT0,
        GL_COLOR_ATTACHMENT1,
        GL_COLOR_ATTACHMENT2,
        GL_COLOR_ATTACHMENT6,
        GL_COLOR_ATTACHMENT7,
        gBuffer.GetColorAttachmentSlotByName("Glass"),
        gBuffer.GetColorAttachmentSlotByName("EmissiveMask"),
        gBuffer.GetColorAttachmentSlotByName("P90MagSpecular") };
    glDrawBuffers(7, attachments);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void SkyBoxPass(RenderData& renderData) {
    //return;
    static CubemapTexture* cubemapTexture = AssetManager::GetCubemapTextureByIndex(AssetManager::GetCubemapTextureIndexByName("NightSky"));
    static Mesh* mesh = AssetManager::GetMeshByIndex(AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Cube"))->GetMeshIndices()[0]);

    GLFrameBuffer& gBuffer = OpenGLRenderer::g_frameBuffers.gBuffer;
    gBuffer.Bind();
    Player* player = Game::GetPlayerByIndex(0);

    Transform skyBoxTransform;
    skyBoxTransform.position = player->GetViewPos();
    skyBoxTransform.scale = glm::vec3(FAR_PLANE * 0.99);

    // Render target
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
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

    glEnable(GL_CULL_FACE);
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

    glDepthMask(true);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
    glBindFramebuffer(GL_FRAMEBUFFER, OpenGLRenderer::g_shadowMapArray.m_ID);
    glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());

    // Regular geometry
    Shader& shader = OpenGLRenderer::g_shaders.shadowMap;
    shader.Use();

    for (Light& light : Scene::g_lights) {
        if (!light.m_shadowCasting || !light.m_shadowMapIsDirty) {
            continue;
        }
        shader.SetFloat("farPlane", light.radius);
        shader.SetVec3("lightPosition", light.position);
        shader.SetMat4("shadowMatrices[0]", light.m_projectionTransforms[0]);
        shader.SetMat4("shadowMatrices[1]", light.m_projectionTransforms[1]);
        shader.SetMat4("shadowMatrices[2]", light.m_projectionTransforms[2]);
        shader.SetMat4("shadowMatrices[3]", light.m_projectionTransforms[3]);
        shader.SetMat4("shadowMatrices[4]", light.m_projectionTransforms[4]);
        shader.SetMat4("shadowMatrices[5]", light.m_projectionTransforms[5]);
        for (int face = 0; face < 6; ++face) {
            shader.SetInt("faceIndex", face);
            GLuint layer = light.m_shadowMapIndex * 6 + face;
            glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, OpenGLRenderer::g_shadowMapArray.m_depthTexture, 0, layer);
            glClear(GL_DEPTH_BUFFER_BIT);            
            shader.SetInt("baseInstance", RendererData::g_shadowMapGeometryDrawInfo[light.m_shadowMapIndex][face].baseInstance);
            MultiDrawIndirect(RendererData::g_shadowMapGeometryDrawInfo[light.m_shadowMapIndex][face].commands, OpenGLBackEnd::GetVertexDataVAO());
        }
    }


    // CSG Geometry
    glBindVertexArray(OpenGLBackEnd::GetCSGVAO());

    Shader& csgShadowMapShader = OpenGLRenderer::g_shaders.shadowMapCSG;
    csgShadowMapShader.Use();

    for (Light& light : Scene::g_lights) {
        if (!light.m_shadowCasting || !light.m_shadowMapIsDirty) {
            continue;
        }
        csgShadowMapShader.SetFloat("farPlane", light.radius);
        csgShadowMapShader.SetVec3("lightPosition", light.position);
        csgShadowMapShader.SetMat4("shadowMatrices[0]", light.m_projectionTransforms[0]);
        csgShadowMapShader.SetMat4("shadowMatrices[1]", light.m_projectionTransforms[1]);
        csgShadowMapShader.SetMat4("shadowMatrices[2]", light.m_projectionTransforms[2]);
        csgShadowMapShader.SetMat4("shadowMatrices[3]", light.m_projectionTransforms[3]);
        csgShadowMapShader.SetMat4("shadowMatrices[4]", light.m_projectionTransforms[4]);
        csgShadowMapShader.SetMat4("shadowMatrices[5]", light.m_projectionTransforms[5]);
        csgShadowMapShader.SetMat4("model", glm::mat4(1));
        for (int face = 0; face < 6; ++face) {
            csgShadowMapShader.SetInt("faceIndex", face);
            GLuint layer = light.m_shadowMapIndex * 6 + face;
            glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, OpenGLRenderer::g_shadowMapArray.m_depthTexture, 0, layer);
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
    glNamedBufferSubData(OpenGLRenderer::g_ssbos.renderItems2D, 0, renderItems.size() * sizeof(RenderItem2D), &renderItems[0]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, OpenGLRenderer::g_ssbos.renderItems2D);

    // Draw instanced
    Mesh* mesh = AssetManager::GetQuadMesh();
    glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
    glDrawElementsInstancedBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), renderItems.size(), mesh->baseVertex);
}


std::vector<Vertex> CreateSphereVertices(const Sphere& sphere, int segments, const glm::vec3& color) {
    std::vector<Vertex> vertices;
    // Ensure segments is at least 4 to form a basic sphere
    segments = std::max(segments, 4);
    // Angles for generating circle points
    float theta_step = glm::two_pi<float>() / segments;
    float phi_step = glm::pi<float>() / segments;
    // Iterate over latitude (phi) and longitude (theta)
    for (int i = 0; i <= segments; ++i) {
        float phi = i * phi_step;
        for (int j = 0; j <= segments; ++j) {
            float theta = j * theta_step;
            // Spherical to Cartesian conversion
            glm::vec3 point_on_sphere(
                sphere.radius * sin(phi) * cos(theta),
                sphere.radius * cos(phi),
                sphere.radius * sin(phi) * sin(theta)
            );
            // Transform the point to the sphere's origin
            glm::vec3 current_point = sphere.origin + point_on_sphere;
            // Draw line to the next point in theta direction
            if (j > 0) {
                float prev_theta = (j - 1) * theta_step;
                glm::vec3 prev_point_on_sphere(
                    sphere.radius * sin(phi) * cos(prev_theta),
                    sphere.radius * cos(phi),
                    sphere.radius * sin(phi) * sin(prev_theta)
                );
                glm::vec3 previous_point = sphere.origin + prev_point_on_sphere;
                Vertex v0 = Vertex(previous_point, color);
                Vertex v1 = Vertex(current_point, color);
                vertices.push_back(v0);
                vertices.push_back(v1);
            }
            // Draw line to the next point in phi direction
            if (i > 0) {
                float prev_phi = (i - 1) * phi_step;
                glm::vec3 prev_point_on_sphere(
                    sphere.radius * sin(prev_phi) * cos(theta),
                    sphere.radius * cos(prev_phi),
                    sphere.radius * sin(prev_phi) * sin(theta)
                );
                glm::vec3 previous_point = sphere.origin + prev_point_on_sphere;
                Vertex v0 = Vertex(previous_point, color);
                Vertex v1 = Vertex(current_point, color);
                vertices.push_back(v0);
                vertices.push_back(v1);
            }
        }
    }
    return vertices;
}


/*

    █▀▀▄ █▀▀ █▀▀█ █  █ █▀▀▀ 　 █▀▀█ █▀▀█ █▀▀ █▀▀
    █  █ █▀▀ █▀▀▄ █  █ █ ▀█ 　 █▄▄█ █▄▄█ ▀▀█ ▀▀█
    █▄▄▀ ▀▀▀ ▀▀▀▀ ▀▀▀▀ ▀▀▀▀ 　 ▀    ▀  ▀ ▀▀▀ ▀▀▀  */

void DebugPass(RenderData& renderData) {

    OpenGLDetachedMesh& linesMesh = renderData.debugLinesMesh->GetGLMesh();
    OpenGLDetachedMesh& linesMesh2D = renderData.debugLinesMesh2D->GetGLMesh();
    OpenGLDetachedMesh& pointsMesh = renderData.debugPointsMesh->GetGLMesh();
    OpenGLDetachedMesh& trianglesMesh = renderData.debugTrianglesMesh->GetGLMesh();

    // Render target
    GLFrameBuffer& presentBuffer = OpenGLRenderer::g_frameBuffers.present;
    BindFrameBuffer(presentBuffer);

    for (int i = 0; i < renderData.playerCount; i++) {
        ViewportInfo viewportInfo = RendererUtil::CreateViewportInfo(i, Game::GetSplitscreenMode(), presentBuffer.GetWidth(), presentBuffer.GetHeight());
        SetViewport(viewportInfo);
        glViewport(viewportInfo.xOffset, viewportInfo.yOffset, viewportInfo.width, viewportInfo.height);
        
        Shader& shader = OpenGLRenderer::g_shaders.debugSolidColor;
        shader.Use();
        shader.SetMat4("projection", renderData.cameraData[i].projection);
        shader.SetMat4("view", renderData.cameraData[i].view);

        //// Water ray cast test
        static Model* model = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Sphere"));
        static Mesh* sphereMesh = AssetManager::GetMeshByIndex(model->GetMeshIndices()[0]);
        static Transform sphereTransform(glm::vec3(0,-10,0));
        //if (Input::KeyPressed(HELL_KEY_3)) {
        //    WaterRayIntersectionResult rayResult = Water::GetMouseRayIntersection(renderData.cameraData[i].projection, renderData.cameraData[i].view);
        //    if (rayResult.hitFound) {
        //        sphereTransform.position = rayResult.hitPosition;
        //    }
        //}
        //


    //   // Draw Christmas lights
    //   glEnable(GL_DEPTH_TEST);
    //   for (ChristmasLights& lights : Scene::g_christmasLights) {
    //       auto& mesh = lights.g_wireMesh;
    //       glBindVertexArray(mesh.GetVAO());
    //       glDrawElements(GL_TRIANGLES, mesh.GetIndexCount(), GL_UNSIGNED_INT, 0);
    //   }




        glEnable(GL_DEPTH_TEST);
        shader.SetMat4("model", sphereTransform.to_mat4());
        glDrawElementsInstancedBaseVertex(GL_TRIANGLES, sphereMesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * sphereMesh->baseIndex), 1, sphereMesh->baseVertex);
        shader.SetMat4("model", Water::GetModelMatrix());
        //Mesh* mesh = AssetManager::GetMeshByIndex(AssetManager::GetUpFacingPlaneMeshIndex());
        //glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
        //glDrawElementsInstancedBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), 1, mesh->baseVertex);

        glDisable(GL_DEPTH_TEST);
        shader.SetMat4("model", glm::mat4(1));

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

        // 2D lines
        shader.SetBool("useUniformColor", false);
        shader.SetMat4("projection", glm::mat4(1));
        shader.SetMat4("view", glm::mat4(1));
        if (linesMesh2D.GetIndexCount() > 0) {
            glBindVertexArray(linesMesh2D.GetVAO());
            glDrawElements(GL_LINES, linesMesh2D.GetIndexCount(), GL_UNSIGNED_INT, 0);
        }

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

    //std::cout << Editor::GetHoveredObjectType() << '\n';
    //std::cout << Editor::GetHoveredObjectType() << '\n';

    // Render selected light AABB
    if (Editor::GetSelectedObjectType() == ObjectType::LIGHT) {
        int lightIndex = Editor::GetSelectedObjectIndex();
        Sphere sphere;
        sphere.origin = Scene::g_lights[lightIndex].position;
        sphere.radius = Scene::g_lights[lightIndex].radius;
        std::vector<Vertex> vertices = CreateSphereVertices(sphere, 12, YELLOW);
        std::vector<uint32_t> indices(vertices.size());
        for (int i = 0; i < indices.size(); i++) {
            indices[i] = i;
        }
        OpenGLRenderer::gLightVolumeSphereMesh.UpdateVertexBuffer(vertices, indices);
        for (int i = 0; i < renderData.playerCount; i++) {
            ViewportInfo viewportInfo = RendererUtil::CreateViewportInfo(i, Game::GetSplitscreenMode(), presentBuffer.GetWidth(), presentBuffer.GetHeight());
            SetViewport(viewportInfo);
            glViewport(viewportInfo.xOffset, viewportInfo.yOffset, viewportInfo.width, viewportInfo.height);
            Shader& aabbShader = OpenGLRenderer::g_shaders.debugLightVolumeAabb;
            aabbShader.Use();
            aabbShader.SetMat4("projection", renderData.cameraData[i].projection);
            aabbShader.SetMat4("view", renderData.cameraData[i].view);
            aabbShader.SetMat4("model", glm::mat4(1));
            aabbShader.SetInt("lightIndex", lightIndex);
            glDisable(GL_DEPTH_TEST);
            if (OpenGLRenderer::gLightVolumeSphereMesh.GetIndexCount() > 0) {
                glBindVertexArray(OpenGLRenderer::gLightVolumeSphereMesh.GetVAO());
                glDrawElements(GL_LINES, OpenGLRenderer::gLightVolumeSphereMesh.GetIndexCount(), GL_UNSIGNED_INT, 0);
            }
        }
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
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, OpenGLRenderer::g_ssbos.skinningTransforms);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, OpenGLRenderer::g_ssbos.baseAnimatedTransformIndices);
            computeShader.SetInt("vertexCount", mesh->vertexCount);
            computeShader.SetInt("baseInputVertex", mesh->baseVertexGlobal);
            computeShader.SetInt("baseOutputVertex", baseOutputVertex);
            computeShader.SetInt("animatedGameObjectIndex", j);
            computeShader.SetInt("vertexCount", mesh->vertexCount);
            //GLuint numGroups = (mesh->vertexCount + 32 - 1) / 32;
            //glDispatchCompute(numGroups, 1, 1);
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

                // cube.m_vertexCount is almost certainly wrong above, check
                // cube.m_vertexCount is almost certainly wrong above, check
                // cube.m_vertexCount is almost certainly wrong above, check
                // cube.m_vertexCount is almost certainly wrong above, check
                // cube.m_vertexCount is almost certainly wrong above, check

            }
        }
    }

    // Geometry (non blended)
    static Material* goldMaterial = AssetManager::GetMaterialByIndex(AssetManager::GetMaterialIndex("Gold"));
    OpenGLRenderer::g_ssbos.geometryRenderItems.Update(RendererData::g_geometryRenderItems.size() * sizeof(RenderItem3D), &RendererData::g_geometryRenderItems[0]);
    Shader& geometryShader = OpenGLRenderer::g_shaders.geometry;
    geometryShader.Use();
    for (int i = 0; i < renderData.playerCount; i++) {
        ViewportInfo viewportInfo = RendererUtil::CreateViewportInfo(i, Game::GetSplitscreenMode(), gBuffer.GetWidth(), gBuffer.GetHeight());
        SetViewport(viewportInfo);
        glViewport(viewportInfo.xOffset, viewportInfo.yOffset, viewportInfo.width, viewportInfo.height);
        geometryShader.SetMat4("projection", renderData.cameraData[i].projection);
        geometryShader.SetMat4("view", renderData.cameraData[i].view);
        geometryShader.SetInt("playerIndex", i);
        geometryShader.SetInt("instanceDataOffset", RendererData::g_geometryDrawInfo[i].baseInstance);
        MultiDrawIndirect(RendererData::g_geometryDrawInfo[i].commands, OpenGLBackEnd::GetVertexDataVAO());
    }    
    // Geometry (Alpha discarded)
    Shader& geometryShaderAlphaDiscarded = OpenGLRenderer::g_shaders.geometryAlphaDiscard;
    geometryShaderAlphaDiscarded.Use();
    OpenGLRenderer::g_ssbos.geometryRenderItems.Update(RendererData::g_geometryRenderItemsAlphaDiscarded .size() * sizeof(RenderItem3D), &RendererData::g_geometryRenderItemsAlphaDiscarded[0]);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    for (int i = 0; i < renderData.playerCount; i++) {
        ViewportInfo viewportInfo = RendererUtil::CreateViewportInfo(i, Game::GetSplitscreenMode(), gBuffer.GetWidth(), gBuffer.GetHeight());
        SetViewport(viewportInfo);
        glViewport(viewportInfo.xOffset, viewportInfo.yOffset, viewportInfo.width, viewportInfo.height);
        geometryShaderAlphaDiscarded.SetMat4("projection", renderData.cameraData[i].projection);
        geometryShaderAlphaDiscarded.SetMat4("view", renderData.cameraData[i].view);
        geometryShaderAlphaDiscarded.SetInt("playerIndex", i);
        geometryShaderAlphaDiscarded.SetInt("instanceDataOffset", RendererData::g_geometryDrawInfoAlphaDiscarded[i].baseInstance);
        MultiDrawIndirect(RendererData::g_geometryDrawInfoAlphaDiscarded[i].commands, OpenGLBackEnd::GetVertexDataVAO());
    }
    // Geometry (blended)
    OpenGLRenderer::g_ssbos.geometryRenderItems.Update(RendererData::g_geometryRenderItemsBlended.size() * sizeof(RenderItem3D), &RendererData::g_geometryRenderItemsBlended[0]);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);
    for (int i = 0; i < renderData.playerCount; i++) {
        ViewportInfo viewportInfo = RendererUtil::CreateViewportInfo(i, Game::GetSplitscreenMode(), gBuffer.GetWidth(), gBuffer.GetHeight());
        SetViewport(viewportInfo);
        glViewport(viewportInfo.xOffset, viewportInfo.yOffset, viewportInfo.width, viewportInfo.height);
        geometryShader.SetMat4("projection", renderData.cameraData[i].projection);
        geometryShader.SetMat4("view", renderData.cameraData[i].view);
        geometryShader.SetInt("playerIndex", i);
        geometryShader.SetInt("instanceDataOffset", RendererData::g_geometryDrawInfoBlended[i].baseInstance);
        MultiDrawIndirect(RendererData::g_geometryDrawInfoBlended[i].commands, OpenGLBackEnd::GetVertexDataVAO());
    }
    glEnable(GL_CULL_FACE);
    glDisable(GL_BLEND); 
    glDepthMask(GL_TRUE);

    // Render Christmas lights
    Shader& christmasLightsShader = OpenGLRenderer::g_shaders.christmasLightWireShader;
    christmasLightsShader.Use();
    glEnable(GL_DEPTH_TEST);
    for (int i = 0; i < renderData.playerCount; i++) {
        ViewportInfo viewportInfo = RendererUtil::CreateViewportInfo(i, Game::GetSplitscreenMode(), gBuffer.GetWidth(), gBuffer.GetHeight());
        SetViewport(viewportInfo);
        christmasLightsShader.SetInt("playerIndex", i);
        christmasLightsShader.SetMat4("projection", renderData.cameraData[i].projection);
        christmasLightsShader.SetMat4("view", renderData.cameraData[i].view);

        // Draw Christmas lights
        for (ChristmasLights& lights : Scene::g_christmasLights) {
            auto& mesh = lights.g_wireMesh;
            glBindVertexArray(mesh.GetVAO());
            glDrawElements(GL_TRIANGLES, mesh.GetIndexCount(), GL_UNSIGNED_INT, 0);
        }
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

            if (mesh->name != "Magazine_low" && mesh->name != "Magazine_low2") {
                glDrawElementsInstancedBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), 1, skinnedRenderItem.baseVertex);

            }
            k++;
        }
    }

    glBindVertexArray(0);

// Mesh names
//      58 Magazine_low
//      91 Magazine_low2

// Bone names
//      Magazine
//      Magazine2


    /*
    static int p90MeshIndex = -1;
    if (p90MeshIndex == -1) {
        SkinnedModel* p90Model = AssetManager::GetSkinnedModelByName("P90");
        for (auto& meshIndex : p90Model->GetMeshIndices()) {
            SkinnedMesh* skinnedMesh = AssetManager::GetSkinnedMeshByIndex(meshIndex);
            if (skinnedMesh->name == "Magazine_low2") {
                p90MeshIndex = meshIndex;
                break;
            }
        }

        //for (int i = 0; i < p90Model->m_joints.size(); i++) {
        //    std::cout << i << ": " << p90Model->m_joints[i].m_name << "\n";
        //}
    }

    SkinnedMesh* magMesh = AssetManager::GetSkinnedMeshByIndex(p90MeshIndex);

 //   std::cout << magMesh->name << "\n";

    glBindVertexArray(OpenGLBackEnd::GetWeightedVertexDataVAO());
    glBindBuffer(GL_ARRAY_BUFFER, OpenGLBackEnd::GetWeightedVertexDataVBO());
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, OpenGLBackEnd::GetWeightedVertexDataEBO());

    Player* player = Game::GetPlayerByIndex(0);

    Shader& shader = OpenGLRenderer::g_shaders.p90Shader;
    shader.Use();

    Transform transform;
    transform.position = glm::vec3(0, 1, 0);

    AnimatedGameObject* viewWeapon = player->GetViewWeaponAnimatedGameObject();

    if (viewWeapon) {
        glm::mat4 p90WorldTransform = viewWeapon->GetModelMatrix() * viewWeapon->GetBoneWorldMatrixFromBoneName("Magazine");

        Shader& p90shader = OpenGLRenderer::g_shaders.csgSubtractive;
        p90shader.Use();
        p90shader.SetMat4("projection", player->GetProjectionMatrix());
        p90shader.SetMat4("view", player->GetViewMatrix());
        //p90shader.SetMat4("model", transform.to_mat4());
        p90shader.SetMat4("model", p90WorldTransform);


        glDisable(GL_DEPTH_TEST);
        glDrawElementsInstancedBaseVertex(GL_TRIANGLES, magMesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * magMesh->baseIndex), 1, magMesh->baseVertexGlobal);

    }


    */

}


void P90MagPass(RenderData& renderData) {

    GLFrameBuffer& gBuffer = OpenGLRenderer::g_frameBuffers.gBuffer;
    Shader& frontFaceShader = OpenGLRenderer::g_shaders.p90MagFrontFaceLighting;
    Shader& backFaceShader = OpenGLRenderer::g_shaders.p90MagBackFaceLighting;
    ComputeShader& frontfaceCompositeShader = OpenGLRenderer::g_shaders.p90MagFrontFaceComposite;
    ComputeShader& backfaceCompositeShader = OpenGLRenderer::g_shaders.p90MagBackFaceComposite;

    gBuffer.Bind();
    static int materialIndex = AssetManager::GetMaterialIndex("P90_Mag");

    // Back face lighting pass
    glDrawBuffer(gBuffer.GetColorAttachmentSlotByName("P90MagSpecular"));
    glBindVertexArray(OpenGLBackEnd::GetSkinnedVertexDataVAO());
    glBindBuffer(GL_ARRAY_BUFFER, OpenGLBackEnd::GetSkinnedVertexDataVBO());
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, OpenGLBackEnd::GetWeightedVertexDataEBO());
    glDepthMask(GL_TRUE);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

    backFaceShader.Use();
    backFaceShader.SetInt("materialIndex", materialIndex);

    for (int i = 0; i < renderData.playerCount; i++) {
        Player* player = Game::GetPlayerByIndex(i);
        ViewportInfo viewportInfo = RendererUtil::CreateViewportInfo(i, Game::GetSplitscreenMode(), gBuffer.GetWidth(), gBuffer.GetHeight());
        SetViewport(viewportInfo);
        backFaceShader.SetMat4("projection", player->GetProjectionMatrix());
        backFaceShader.SetMat4("view", player->GetViewMatrix());
        backFaceShader.SetMat4("inverseView", player->GetViewMatrix());
        backFaceShader.SetVec3("cameraForward", player->GetCameraForward());
        backFaceShader.SetVec3("viewPos", player->GetViewPos());
        backFaceShader.SetInt("playerIndex", i);
       
        
        /*
        static Model* model = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Cube"));
        Mesh* cubeMesh = AssetManager::GetMeshByIndex(model->GetMeshIndices()[0]);

        Transform transform;
        transform.position = glm::vec3(0, 1.3f, 0);
        transform.scale = glm::vec3(0.5f);
        backFaceShader.SetMat4("model", transform.to_mat4());
        glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
        glDrawElementsInstancedBaseVertex(GL_TRIANGLES, cubeMesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * cubeMesh->baseIndex), 1, cubeMesh->baseVertex);
        */

        
        for (SkinnedRenderItem& skinnedRenderItem : renderData.skinnedRenderItems[i]) {
            SkinnedMesh* mesh = AssetManager::GetSkinnedMeshByIndex(skinnedRenderItem.originalMeshIndex);
            if (mesh->name == "Magazine_low" || mesh->name == "Magazine_low2") {
                backFaceShader.SetMat4("model", skinnedRenderItem.modelMatrix);
                glDrawElementsInstancedBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), 1, skinnedRenderItem.baseVertex);
            }
        }

    }


    
    // Backface composite that render back into the lighting texture
    gBuffer.SetViewport();
    backfaceCompositeShader.Use();
    glBindImageTexture(0, gBuffer.GetColorAttachmentHandleByName("FinalLighting"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
    glBindImageTexture(1, gBuffer.GetColorAttachmentHandleByName("P90MagSpecular"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
    glDispatchCompute(gBuffer.GetWidth() / 16, gBuffer.GetHeight() / 4, 1);
    



    // Front face lighting pass
    unsigned int attachments2[2] = {
        gBuffer.GetColorAttachmentSlotByName("P90MagSpecular"),
        gBuffer.GetColorAttachmentSlotByName("P90MagDirectLighting") 
    };
    glDrawBuffers(2, attachments2);
    glBindVertexArray(OpenGLBackEnd::GetSkinnedVertexDataVAO());
    glBindBuffer(GL_ARRAY_BUFFER, OpenGLBackEnd::GetSkinnedVertexDataVBO());
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, OpenGLBackEnd::GetWeightedVertexDataEBO());
    glDepthMask(GL_TRUE);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glCullFace(GL_BACK);
    frontFaceShader.Use();
    frontFaceShader.SetInt("materialIndex", materialIndex);
    for (int i = 0; i < renderData.playerCount; i++) {
        Player* player = Game::GetPlayerByIndex(i);
        ViewportInfo viewportInfo = RendererUtil::CreateViewportInfo(i, Game::GetSplitscreenMode(), gBuffer.GetWidth(), gBuffer.GetHeight());
        SetViewport(viewportInfo);
        frontFaceShader.SetMat4("projection", player->GetProjectionMatrix());
        frontFaceShader.SetMat4("view", player->GetViewMatrix());
        frontFaceShader.SetVec3("viewPos", player->GetViewPos());
        frontFaceShader.SetInt("playerIndex", i);
        frontFaceShader.SetMat4("inverseView", player->GetViewMatrix());
        frontFaceShader.SetVec3("cameraForward", player->GetCameraForward());
        for (SkinnedRenderItem& skinnedRenderItem : renderData.skinnedRenderItems[i]) {
            SkinnedMesh* mesh = AssetManager::GetSkinnedMeshByIndex(skinnedRenderItem.originalMeshIndex);
            if (mesh->name == "Magazine_low" || mesh->name == "Magazine_low2") {
                frontFaceShader.SetMat4("model", skinnedRenderItem.modelMatrix);
                glDrawElementsInstancedBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), 1, skinnedRenderItem.baseVertex);
            }
        }
    }

    // Front face composite that render back into the lighting texture
    gBuffer.SetViewport();
    frontfaceCompositeShader.Use();
    glBindImageTexture(0, gBuffer.GetColorAttachmentHandleByName("FinalLighting"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
    glBindImageTexture(1, gBuffer.GetColorAttachmentHandleByName("P90MagSpecular"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
    glBindImageTexture(2, gBuffer.GetColorAttachmentHandleByName("P90MagDirectLighting"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
    glDispatchCompute(gBuffer.GetWidth() / 16, gBuffer.GetHeight() / 4, 1);

    // Cleanup
    glDisable(GL_CULL_FACE);
    glCullFace(GL_BACK);
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
    Shader& shader = OpenGLRenderer::g_shaders.decalsBlood;
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
    Shader& shader = OpenGLRenderer::g_shaders.decalsBullet;
    shader.Use();

    for (int i = 0; i < renderData.playerCount; i++) {
        shader.SetInt("playerIndex", i);
        ViewportInfo viewportInfo = RendererUtil::CreateViewportInfo(i, Game::GetSplitscreenMode(), gBuffer.GetWidth(), gBuffer.GetHeight());
        SetViewport(viewportInfo);
        shader.SetInt("instanceDataOffset", RendererData::g_bulletDecalDrawInfo[i].baseInstance);
        MultiDrawIndirect(RendererData::g_bulletDecalDrawInfo[i].commands, OpenGLBackEnd::GetVertexDataVAO());
    }
}


/*

█    █ █▀▀▀ █  █ ▀▀█▀▀ █ █▀▀█ █▀▀▀ 　 █▀▀█ █▀▀█ █▀▀ █▀▀
█    █ █ ▀█ █▀▀█   █   █ █  █ █ ▀█ 　 █▄▄█ █▄▄█ ▀▀█ ▀▀█
█▄▄█ ▀ ▀▀▀▀ ▀  ▀   ▀   ▀ ▀  ▀ ▀▀▀▀ 　 ▀    ▀  ▀ ▀▀▀ ▀▀▀  */

void LightingPass(RenderData& renderData) {

    GLFrameBuffer& gBuffer = OpenGLRenderer::g_frameBuffers.gBuffer;
    LightVolume* lightVolume = GlobalIllumination::GetLightVolumeByIndex(0);

    ComputeShader& computeShader = OpenGLRenderer::g_shaders.lighting;
    computeShader.Use();
    computeShader.SetFloat("viewportWidth", gBuffer.GetWidth());
    computeShader.SetFloat("viewportHeight", gBuffer.GetHeight());
    computeShader.SetInt("lightCount", Scene::g_lights.size());
    computeShader.SetInt("tileXCount", gBuffer.GetWidth() / 12);
    computeShader.SetInt("tileYCount", gBuffer.GetHeight() / 12);
    computeShader.SetInt("tileYCount", gBuffer.GetHeight() / 12);
    computeShader.SetInt("probeSpaceWidth", lightVolume->GetProbeSpaceWidth());
    computeShader.SetInt("probeSpaceHeight", lightVolume->GetProbeSpaceHeight());
    computeShader.SetInt("probeSpaceDepth", lightVolume->GetProbeSpaceDepth());
    computeShader.SetVec3("lightVolumePosition", lightVolume->GetPosition());
    computeShader.SetFloat("probeSpacing", PROBE_SPACING);
    computeShader.SetFloat("time", Game::GetTime());
    computeShader.SetInt("rendererOverrideState", RendererData::GetRendererOverrideStateAsInt());
    computeShader.SetFloat("waterHeight", Water::GetHeight());
    if (Renderer::GetRenderMode() == COMPOSITE || Renderer::GetRenderMode() == COMPOSITE_PLUS_POINT_CLOUD) {
        computeShader.SetInt("renderMode", 0);
    }
    else if (Renderer::GetRenderMode() == DIRECT_LIGHT) {
        computeShader.SetInt("renderMode", 1);
    }
    else if (Renderer::GetRenderMode() == POINT_CLOUD) {
        computeShader.SetInt("renderMode", 2);
    }
    glBindImageTexture(0, gBuffer.GetColorAttachmentHandleByName("FinalLighting"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gBuffer.GetDepthAttachmentHandle());
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, OpenGLRenderer::g_shadowMapArray.m_depthTexture);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gBuffer.GetColorAttachmentHandleByName("BaseColor"));
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, gBuffer.GetColorAttachmentHandleByName("Normal"));
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, gBuffer.GetColorAttachmentHandleByName("RMA"));
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, gBuffer.GetColorAttachmentHandleByName("EmissiveMask"));
    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_3D, lightVolume->texutre3D.GetGLTexture3D().GetID());

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 19, OpenGLRenderer::g_ssbos.lightVolumeData.GetHandle());

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    glDispatchCompute(gBuffer.GetWidth() / TILE_SIZE, gBuffer.GetHeight() / TILE_SIZE, 1);
}

float RandomFloat(float min, float max) {
    return min + static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * (max - min);
}

float Lerp(float a, float b, float f) {
    return a + f * (b - a);
}

void SSAOPass() {

    return;

    GLFrameBuffer& gBuffer = OpenGLRenderer::g_frameBuffers.gBuffer;
    GLFrameBuffer& genericRenderTargets = OpenGLRenderer::g_frameBuffers.genericRenderTargets;
    ComputeShader& shader = OpenGLRenderer::g_shaders.ssao;
    ComputeShader& blurShader = OpenGLRenderer::g_shaders.ssaoBlur;
    ComputeShader& worldPosShader = OpenGLRenderer::g_shaders.worldPosition;

    // World Space Pos Hack
    worldPosShader.Use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gBuffer.GetDepthAttachmentHandle());
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gBuffer.GetColorAttachmentHandleByName("Normal"));
    glBindImageTexture(0, genericRenderTargets.GetColorAttachmentHandleByName("WorldSpacePosition"), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
    glDispatchCompute((genericRenderTargets.GetWidth() + 7) / 8, (genericRenderTargets.GetHeight() + 7) / 8, 1);
   
    // Create SSAO noise texture and kernel
    static std::vector<glm::vec3> ssaoKernel;
    static GLuint noiseTexture = 0;
    int kernelSize = 8;
    if (noiseTexture == 0) {
        for (unsigned int i = 0; i < kernelSize; ++i) {
            glm::vec3 sample = glm::normalize(glm::vec3(RandomFloat(-1, 1), RandomFloat(-1, 1), RandomFloat(0, 1)));
            float scale = float(i) / float(kernelSize);
            scale = Lerp(0.1f, 1.0f, scale * scale);
            sample *= scale;
            ssaoKernel.push_back(sample);
        }
        std::vector<glm::vec3> ssaoNoise;
        for (unsigned int i = 0; i < 16; i++) {
            glm::vec3 noise = glm::vec3(RandomFloat(-1, 1), RandomFloat(-1, 1), 0);
            ssaoNoise.push_back(noise);
        }
        glGenTextures(1, &noiseTexture);
        glBindTexture(GL_TEXTURE_2D, noiseTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
    // Calculate SSAO
    shader.Use();
    for (unsigned int i = 0; i < kernelSize; ++i) {
        std::string name = "samples[" + std::to_string(i) + "]";
        shader.SetVec3(name.c_str(), ssaoKernel[i]);
    }
    glBindImageTexture(0, genericRenderTargets.GetColorAttachmentHandleByName("SSAO"), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gBuffer.GetColorAttachmentHandleByName("Normal"));
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, noiseTexture);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, genericRenderTargets.GetColorAttachmentHandleByName("WorldSpacePosition"));
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, gBuffer.GetDepthAttachmentHandle());
    glDispatchCompute((gBuffer.GetWidth() + 7) / 8, (gBuffer.GetHeight() + 7) / 8, 1);

    // Blur the result
    blurShader.Use();
    glBindImageTexture(0, genericRenderTargets.GetColorAttachmentHandleByName("SSAOBlur"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, genericRenderTargets.GetColorAttachmentHandleByName("SSAO"));
    glDispatchCompute((gBuffer.GetWidth() + 7) / 8, (gBuffer.GetHeight() + 7) / 8, 1);
}




void LightVolumePrePass(RenderData& renderData) {

    // Light volume from positon/radius
    bool found = false;
    for (Light& light : Scene::g_lights) {
        if (light.m_aabbLightVolumeMode == AABBLightVolumeMode::POSITION_RADIUS && light.m_aaabbVolumeIsDirty) {
            light.m_aaabbVolumeIsDirty = false;
            found = true;
        }
    }
    if (found) {
        ComputeShader& computeShader = OpenGLRenderer::g_shaders.lightVolumeFromPositionAndRadius;
        computeShader.Use();
        computeShader.SetInt("lightCount", Scene::g_lights.size());
        int invocationCount = std::ceil(Scene::g_lights.size() / 64.0f);
        glDispatchCompute(invocationCount, 1, 1);
        std::cout << "Computing light volume AABBs from position/radius\n";
    }

    // Light volume from world pos cube map
    for (int i = 0; i < Scene::g_lights.size(); i++) {
        int lightIndex = i;
        Light& light = Scene::g_lights[i];
        if (light.m_aabbLightVolumeMode == AABBLightVolumeMode::WORLDSPACE_CUBE_MAP && light.m_aaabbVolumeIsDirty) {
            light.m_aaabbVolumeIsDirty = false;
            light.UpdateMatricesAndFrustum();

            CubeMap2& cubemap = OpenGLRenderer::g_lightVolumePrePassCubeMap;
            int width = OpenGLRenderer::g_lightVolumePrePassCubeMap.m_size;
            int height = OpenGLRenderer::g_lightVolumePrePassCubeMap.m_size;

            ComputeShader& clearShader = OpenGLRenderer::g_shaders.lightVolumeClear;
            clearShader.Use();
            clearShader.SetInt("lightIndex", i);
            //glBindImageTexture(0, OpenGLRenderer::g_lightVolumePrePassCubeMap.m_textureView, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
            //glDispatchCompute(width / 8, height / 8, 1);

            Shader& shader = OpenGLRenderer::g_shaders.lightVolumePrePassGeometry;
            shader.Use();
            shader.SetFloat("farPlane", light.radius);
            shader.SetVec3("lightPosition", light.position);
            shader.SetMat4("shadowMatrices[0]", light.m_projectionTransforms[0]);
            shader.SetMat4("shadowMatrices[1]", light.m_projectionTransforms[1]);
            shader.SetMat4("shadowMatrices[2]", light.m_projectionTransforms[2]);
            shader.SetMat4("shadowMatrices[3]", light.m_projectionTransforms[3]);
            shader.SetMat4("shadowMatrices[4]", light.m_projectionTransforms[4]);
            shader.SetMat4("shadowMatrices[5]", light.m_projectionTransforms[5]);

            // Render csg geometry
            glBindFramebuffer(GL_FRAMEBUFFER, OpenGLRenderer::g_lightVolumePrePassCubeMap.m_ID);
            glViewport(0, 0, cubemap.m_size, cubemap.m_size);
            glBindFramebuffer(GL_FRAMEBUFFER, cubemap.m_ID);
            //glClearColor(light.position.x, light.position.y, light.position.z, 1.0f);
            glClearColor(0, 0, 0, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glEnable(GL_DEPTH_TEST);
            glDisable(GL_BLEND);
            glDepthMask(true);
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            glBindVertexArray(OpenGLBackEnd::GetCSGVAO());
            shader.SetMat4("model", glm::mat4(1));

            Frustum frustum[6];
            for (int j = 0; j < 6; j++) {
                frustum[j].Update(light.m_projectionTransforms[j]);
            }
            for (CSGObject& csgObject : CSG::GetCSGObjects()) {
                bool found = false;
                for (int j = 0; j < 6; j++) {
                    if (frustum[j].IntersectsAABBFast(csgObject.m_aabb)) {
                        found = true;
                        break;
                    }
                }
                if (found) {
                    glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLES, csgObject.m_vertexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * csgObject.m_baseIndex), 1, csgObject.m_baseVertex, 0);
                }
            }

            // Compute the min and max worldspace bounds of the light source volume
            ComputeShader& computeShader = OpenGLRenderer::g_shaders.lightVolumeFromCubeMap;
            computeShader.Use();
            computeShader.SetInt("lightIndex", i);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D_ARRAY, OpenGLRenderer::g_lightVolumePrePassCubeMap.m_textureView);
            glDispatchCompute(1, 1, 1);
            std::cout << "updating light " << i << "\n";
        }
    }
}

void LightCullingPass(RenderData& renderData) {
    GLFrameBuffer& gBuffer = OpenGLRenderer::g_frameBuffers.gBuffer;
    ComputeShader& computeShader = OpenGLRenderer::g_shaders.lightCulling;
    computeShader.Use();
    computeShader.SetFloat("viewportWidth", gBuffer.GetWidth());
    computeShader.SetFloat("viewportHeight", gBuffer.GetHeight());
    computeShader.SetInt("lightCount", Scene::g_lights.size());
    computeShader.SetInt("tileXCount", gBuffer.GetWidth() / 12);
    computeShader.SetInt("tileYCount", gBuffer.GetHeight() / 12);
    glBindImageTexture(0, gBuffer.GetColorAttachmentHandleByName("FinalLighting"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
    glBindImageTexture(1, gBuffer.GetColorAttachmentHandleByName("Normal"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gBuffer.GetDepthAttachmentHandle());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 19, OpenGLRenderer::g_ssbos.lightVolumeData.GetHandle());
    glDispatchCompute(gBuffer.GetWidth() / TILE_SIZE, gBuffer.GetHeight() / TILE_SIZE, 1);
}

void DebugTileViewPass(RenderData& renderData) {
    GLFrameBuffer& gBuffer = OpenGLRenderer::g_frameBuffers.gBuffer;
    ComputeShader& debugTileViewShader = OpenGLRenderer::g_shaders.debugTileView;
    debugTileViewShader.Use();
    if (Renderer::GetRenderMode() == TILE_HEATMAP) {
        debugTileViewShader.SetInt("mode", 0);
    }
    else if (Renderer::GetRenderMode() == LIGHTS_PER_TILE) {
        debugTileViewShader.SetInt("mode", 1);
    }
    else if (Renderer::GetRenderMode() == LIGHTS_PER_PIXEL) {
        debugTileViewShader.SetInt("mode", 2);
    }
    else {
        return;
    }
    debugTileViewShader.SetFloat("viewportWidth", gBuffer.GetWidth());
    debugTileViewShader.SetFloat("viewportHeight", gBuffer.GetHeight());
    debugTileViewShader.SetInt("lightCount", Scene::g_lights.size());
    debugTileViewShader.SetInt("tileXCount", gBuffer.GetWidth() / 12);
    debugTileViewShader.SetInt("tileYCount", gBuffer.GetHeight() / 12);
    glBindImageTexture(0, gBuffer.GetColorAttachmentHandleByName("FinalLighting"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
    glBindImageTexture(1, gBuffer.GetColorAttachmentHandleByName("Normal"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gBuffer.GetDepthAttachmentHandle());
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, OpenGLRenderer::g_shadowMapArray.m_depthTexture);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 19, OpenGLRenderer::g_ssbos.lightVolumeData.GetHandle());
    glDispatchCompute(gBuffer.GetWidth() / TILE_SIZE, gBuffer.GetHeight() / TILE_SIZE, 1);
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

    GLFrameBuffer& presentBuffer = OpenGLRenderer::g_frameBuffers.present;
    BindFrameBuffer(presentBuffer);
    presentBuffer.SetViewport();

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
        if (Editor::GetHoveredObjectType() == ObjectType::CSG_OBJECT_ADDITIVE_CUBE ||
            Editor::GetHoveredObjectType() == ObjectType::CSG_OBJECT_ADDITIVE_WALL_PLANE ||
            Editor::GetHoveredObjectType() == ObjectType::CSG_OBJECT_ADDITIVE_FLOOR_PLANE ||
            Editor::GetHoveredObjectType() == ObjectType::CSG_OBJECT_ADDITIVE_CEILING_PLANE) {
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

        if (Editor::GetHoveredObjectType() == ObjectType::CSG_OBJECT_ADDITIVE_CUBE ||
            Editor::GetHoveredObjectType() == ObjectType::CSG_OBJECT_ADDITIVE_WALL_PLANE ||
            Editor::GetHoveredObjectType() == ObjectType::CSG_OBJECT_ADDITIVE_FLOOR_PLANE ||
            Editor::GetHoveredObjectType() == ObjectType::CSG_OBJECT_ADDITIVE_CEILING_PLANE) {
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

        if (Editor::GetSelectedObjectType() == ObjectType::CSG_OBJECT_ADDITIVE_CUBE ||
            Editor::GetSelectedObjectType() == ObjectType::CSG_OBJECT_ADDITIVE_WALL_PLANE ||
            Editor::GetSelectedObjectType() == ObjectType::CSG_OBJECT_ADDITIVE_FLOOR_PLANE ||
            Editor::GetSelectedObjectType() == ObjectType::CSG_OBJECT_ADDITIVE_CEILING_PLANE) {
            CSGObject& csgObject = CSG::GetCSGObjects()[Editor::GetSelectedObjectIndex()];
            shader.SetMat4("model", glm::mat4(1));
            glBindVertexArray(OpenGLBackEnd::GetCSGVAO());
            glDrawElementsInstancedBaseVertex(GL_TRIANGLES, csgObject.m_vertexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * csgObject.m_baseIndex), 1, csgObject.m_baseVertex);
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

        if (Editor::GetSelectedObjectType() == ObjectType::CSG_OBJECT_ADDITIVE_CUBE ||
            Editor::GetSelectedObjectType() == ObjectType::CSG_OBJECT_ADDITIVE_WALL_PLANE ||
            Editor::GetSelectedObjectType() == ObjectType::CSG_OBJECT_ADDITIVE_FLOOR_PLANE ||
            Editor::GetSelectedObjectType() == ObjectType::CSG_OBJECT_ADDITIVE_CEILING_PLANE) {
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
        CSGCube* cubeVolume = nullptr;
        if (Editor::GetSelectedObjectType() == ObjectType::CSG_OBJECT_ADDITIVE_CUBE) {
            cubeVolume = &Scene::g_csgAdditiveCubes[Editor::GetSelectedObjectIndex()];
        }
        if (Editor::GetSelectedObjectType() == ObjectType::CSG_OBJECT_SUBTRACTIVE) {
            cubeVolume = &Scene::g_csgSubtractiveCubes[Editor::GetSelectedObjectIndex()];
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

    GLFrameBuffer& presentBuffer = OpenGLRenderer::g_frameBuffers.present;
    BindFrameBuffer(presentBuffer);
    presentBuffer.SetViewport();

    Player* player = Game::GetPlayerByIndex(0);

    Shader& shader = OpenGLRenderer::g_shaders.csgSubtractive;
    shader.Use();
    shader.SetMat4("projection", player->GetProjectionMatrix());
    shader.SetMat4("view", player->GetViewMatrix());
    shader.SetBool("useUniformColor", false);


    std::vector<glm::mat4> planeTransforms;

    for (CSGCube& cubeVolume : Scene::g_csgSubtractiveCubes) {

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

    Transform transform;
    transform.scale = glm::vec3(0.5f);
    glm::mat4 scale = transform.to_mat4();

    glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
    for (glm::mat4& matrix : planeTransforms) {
        int planeMeshIndex = AssetManager::GetHalfSizeQuadMeshIndex();
        shader.SetVec3("color", ORANGE);
        shader.SetMat4("model", matrix * scale);
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



    for (CSGCube& cubeVolume : Scene::g_csgSubtractiveCubes) {

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
    g_ssbos.tlasNodes.Update(tlas->GetNodes().size() * sizeof(BVHNode), (void*)&tlas->GetNodes()[0]);
    g_ssbos.blasNodes.Update(Raytracing::GetBLSANodes().size() * sizeof(BVHNode), (void*)&Raytracing::GetBLSANodes()[0]);
    g_ssbos.blasInstances.Update(blasInstaces.size() * sizeof(BLASInstance), (void*)&blasInstaces[0]);
    g_ssbos.triangleIndices.Update(Raytracing::GetTriangleIndices().size() * sizeof(unsigned int), (void*)&Raytracing::GetTriangleIndices()[0]);

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 20, OpenGLBackEnd::GetCSGVBO());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 21, OpenGLBackEnd::GetCSGEBO());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 22, g_ssbos.blasNodes.GetHandle());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 23, g_ssbos.tlasNodes.GetHandle());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 24, g_ssbos.blasInstances.GetHandle());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 25, g_ssbos.triangleIndices.GetHandle());
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
    GLFrameBuffer& waterFrameBuffer = OpenGLRenderer::g_frameBuffers.water;
    GLFrameBuffer& hairFrameBuffer = OpenGLRenderer::g_frameBuffers.hair;
    GLFrameBuffer& genericRenderTargets = OpenGLRenderer::g_frameBuffers.genericRenderTargets;
    ComputeShader& computeShader = OpenGLRenderer::g_shaders.postProcessing;
    computeShader.Use();
    computeShader.SetFloat("viewportWidth", gBuffer.GetWidth());
    computeShader.SetFloat("viewportHeight", gBuffer.GetHeight());
    computeShader.SetFloat("time", Game::GetTime());
    computeShader.SetFloat("waterHeight", Water::GetHeight());
    glBindImageTexture(0, gBuffer.GetColorAttachmentHandleByName("FinalLighting"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
    glBindImageTexture(1, gBuffer.GetColorAttachmentHandleByName("Normal"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
    glBindImageTexture(3, waterFrameBuffer.GetColorAttachmentHandleByName("Color"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
    glBindImageTexture(4, hairFrameBuffer.GetColorAttachmentHandleByName("Composite"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);


    glDispatchCompute(gBuffer.GetWidth() / 16, gBuffer.GetHeight() / 4, 1);
    //glFinish();
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
    shader.SetInt("lightCount", Scene::g_lights.size());

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, OpenGLRenderer::g_ssbos.lights);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, OpenGLBackEnd::GetPointCloudVBO());

    TLAS* tlas = Raytracing::GetTLASByIndex(0);
    if (tlas) {
        std::vector<BLASInstance> blasInstaces = Raytracing::GetBLASInstances(0);
        g_ssbos.tlasNodes.Update(tlas->GetNodes().size() * sizeof(BVHNode), (void*)&tlas->GetNodes()[0]);
        g_ssbos.blasNodes.Update(Raytracing::GetBLSANodes().size() * sizeof(BVHNode), (void*)&Raytracing::GetBLSANodes()[0]);
        g_ssbos.blasInstances.Update(blasInstaces.size() * sizeof(BLASInstance), (void*)&blasInstaces[0]);
        g_ssbos.triangleIndices.Update(Raytracing::GetTriangleIndices().size() * sizeof(unsigned int), (void*)&Raytracing::GetTriangleIndices()[0]);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 20, OpenGLBackEnd::GetCSGVBO());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 21, OpenGLBackEnd::GetCSGEBO());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 22, g_ssbos.blasNodes.GetHandle());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 23, g_ssbos.tlasNodes.GetHandle());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 24, g_ssbos.blasInstances.GetHandle());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 25, g_ssbos.triangleIndices.GetHandle());
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
    }

    glDispatchCompute(invocationCount, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}


void OpenGLRenderer::Triangle2DPass() {

    return;



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
                        }
                        player->m_pickUpInteractable = true;
                    }
                }
            }

        }
    }



    std::vector<glm::vec2> triangleVertices = Util::Generate2DVerticesFromPixelCoords(pixelCoords, PRESENT_WIDTH, PRESENT_HEIGHT);
    OpenGLBackEnd::UploadTriangle2DData(triangleVertices);

    g_shaders.debugSolidColor2D.Use();
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


   // if (g_frameBuffers.megaTexture.GetHandle() != 0) {
   //     blitDstCoords.dstX0 = 0;
   //     blitDstCoords.dstY0 = 0;
   //     blitDstCoords.dstX1 = BackEnd::GetCurrentWindowWidth() * 0.25;
   //     blitDstCoords.dstY1 = BackEnd::GetCurrentWindowHeight() * 0.25;
   //     BlitPlayerPresentTargetToDefaultFrameBuffer(&g_frameBuffers.megaTexture, 0, "Color", "", GL_COLOR_BUFFER_BIT, GL_NEAREST, blitDstCoords);
   // }
}

void OpenGLRenderer::IndirectLightingPass() {
    if (Renderer::GetRenderMode() == RenderMode::COMPOSITE ||
        Renderer::GetRenderMode() == RenderMode::COMPOSITE_PLUS_POINT_CLOUD) {

        LightVolume* lightVolume = GlobalIllumination::GetLightVolumeByIndex(0);
        OpenGLTexture3D& texture3D = lightVolume->texutre3D.GetGLTexture3D();
        GLFrameBuffer& gBuffer = OpenGLRenderer::g_frameBuffers.gBuffer;
        ComputeShader& computeShader = OpenGLRenderer::g_shaders.probeLighting;

        computeShader.Use();
        computeShader.SetInt("frameNumber", GlobalIllumination::GetFrameCounter()); 
        computeShader.SetInt("probeSpaceWidth", lightVolume->GetProbeSpaceWidth());
        computeShader.SetInt("probeSpaceHeight", lightVolume->GetProbeSpaceHeight());
        computeShader.SetInt("probeSpaceDepth", lightVolume->GetProbeSpaceDepth());
        computeShader.SetInt("cloudPointCount", GlobalIllumination::GetPointCloud().size());
        computeShader.SetVec3("lightVolumePosition", lightVolume->GetPosition());
        computeShader.SetFloat("probeSpacing", PROBE_SPACING);

        glBindImageTexture(0, texture3D.GetID(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, OpenGLBackEnd::GetPointCloudVBO());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 20, OpenGLBackEnd::GetCSGVBO());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 21, OpenGLBackEnd::GetCSGEBO());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 22, g_ssbos.blasNodes.GetHandle());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 23, g_ssbos.tlasNodes.GetHandle());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 24, g_ssbos.blasInstances.GetHandle());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 25, g_ssbos.triangleIndices.GetHandle());
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);

        int workGroupSize = 4;
        int workGroupX = (lightVolume->GetProbeSpaceWidth() + workGroupSize - 1) / workGroupSize;
        int workGroupY = (lightVolume->GetProbeSpaceHeight() + workGroupSize - 1) / workGroupSize;
        int workGroupZ = (lightVolume->GetProbeSpaceDepth() + workGroupSize - 1) / workGroupSize;
        glDispatchCompute(workGroupX, workGroupY, workGroupZ);
    }
}

void OpenGLRenderer::ProbeGridDebugPass() {

    if (Renderer::ProbesVisible()) {

        static Model* model = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("Cube"));
        Mesh* cubeMesh = AssetManager::GetMeshByIndex(model->GetMeshIndices()[0]);

        LightVolume* lightVolume = GlobalIllumination::GetLightVolumeByIndex(0);
        OpenGLTexture3D& texture3D = lightVolume->texutre3D.GetGLTexture3D();

        Player* player = Game::GetPlayerByIndex(0);
        glm::mat4 projection = player->GetProjectionMatrix();
        glm::mat4 view = player->GetViewMatrix();

        g_shaders.debugProbes.Use();
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
        glBindImageTexture(1, texture3D.GetID(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
        glDrawElementsInstancedBaseVertex(GL_TRIANGLES, cubeMesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * cubeMesh->baseIndex), instanceCount, cubeMesh->baseVertex);
    }
    /*


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
    */
}


void OpenGLRenderer::HeightMapPass(RenderData& renderData) {

    glDisable(GL_BLEND);

    HeightMap& heightMap = AssetManager::g_heightMap;
    GLFrameBuffer& gBuffer = OpenGLRenderer::g_frameBuffers.gBuffer;
    GLFrameBuffer& megaTextureFBO = OpenGLRenderer::g_frameBuffers.megaTexture;

    gBuffer.Bind();

    unsigned int attachments[3] = {
        GL_COLOR_ATTACHMENT0,
        GL_COLOR_ATTACHMENT1,
        GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByName("Ground_MudVeg_ALB")->GetGLTexture().GetID());
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByName("Ground_MudVeg_NRM")->GetGLTexture().GetID());
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByName("Ground_MudVeg_RMA")->GetGLTexture().GetID());
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, megaTextureFBO.GetColorAttachmentHandleByName("Color"));
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByName("TreeMap")->GetGLTexture().GetID());
       
    for (int i = 0; i < renderData.playerCount; i++) {
        Player* player = Game::GetPlayerByIndex(i);
        ViewportInfo viewportInfo = RendererUtil::CreateViewportInfo(i, Game::GetSplitscreenMode(), gBuffer.GetWidth(), gBuffer.GetHeight());
        SetViewport(viewportInfo);
        glViewport(viewportInfo.xOffset, viewportInfo.yOffset, viewportInfo.width, viewportInfo.height);
        Shader& shader = OpenGLRenderer::g_shaders.heightMap;
        shader.Use();
        shader.SetMat4("mvp", player->GetProjectionMatrix() * player->GetViewMatrix() * heightMap.m_transform.to_mat4());
        shader.SetMat4("normalMatrix", glm::transpose(glm::inverse(heightMap.m_transform.to_mat4())));
        shader.SetInt("playerIndex", i);
        shader.SetFloat("heightMapWidth", heightMap.m_width);
        shader.SetFloat("heightMapDepth", heightMap.m_depth);
        glBindVertexArray(heightMap.m_VAO);
        glDrawElements(GL_TRIANGLE_STRIP, heightMap.m_indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
}


void OpenGLRenderer::QueryAvaliability() {
    GLint maxLayers;
    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &maxLayers);
    //std::cout << "GL_MAX_ARRAY_TEXTURE_LAYERS: " << maxLayers << "\n";
    std::cout << "Max cubemaps in cubemap array: " << (maxLayers / 6) << "\n";

    GLint maxSSBOBindings = 0;
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &maxSSBOBindings);
    std::cout << "Max SSBO bindings: " << maxSSBOBindings << "\n";

    GLint maxWorkgroupSize[3];
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &maxWorkgroupSize[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &maxWorkgroupSize[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &maxWorkgroupSize[2]);
    printf("Max Workgroup Size (X): %d\n", maxWorkgroupSize[0]);
    printf("Max Workgroup Size (Y): %d\n", maxWorkgroupSize[1]);
    printf("Max Workgroup Size (Z): %d\n", maxWorkgroupSize[2]);

    GLint maxInvocations;
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &maxInvocations);
    printf("Max Workgroup Invocations: %d\n", maxInvocations);
    std::cout << "\n";
}



void WinstonPass(RenderData& renderData) {

    if (OpenGLRenderer::g_sphereMesh.GetVAO() == 0) {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        Util::CreateSolidSphereVerticsAndIndices(vertices, indices, 1.0, 12);
        OpenGLRenderer::g_sphereMesh.UpdateVertexBuffer(vertices, indices);
    }

    GLFrameBuffer& gBuffer = OpenGLRenderer::g_frameBuffers.gBuffer;
    BindFrameBuffer(gBuffer);
    glDrawBuffer(gBuffer.GetColorAttachmentSlotByName("FinalLighting"));

    glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());

    for (int i = 0; i < renderData.playerCount; i++) {
        ViewportInfo viewportInfo = RendererUtil::CreateViewportInfo(i, Game::GetSplitscreenMode(), gBuffer.GetWidth(), gBuffer.GetHeight());
        SetViewport(viewportInfo);
        glViewport(viewportInfo.xOffset, viewportInfo.yOffset, viewportInfo.width, viewportInfo.height);

        Shader& shader = OpenGLRenderer::g_shaders.winston;
        shader.Use();
        shader.SetMat4("projection", renderData.cameraData[i].projection);
        shader.SetMat4("view", renderData.cameraData[i].view);
        shader.SetVec3("color", { 0, 0.9f, 1 });
        shader.SetFloat("alpha", 0.01f);
        shader.SetVec2("screensize", glm::vec2(viewportInfo.width, viewportInfo.height));
        shader.SetFloat("near", NEAR_PLANE);
        shader.SetFloat("far", FAR_PLANE);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gBuffer.GetDepthAttachmentHandle());
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glEnable(GL_BLEND);

        Player* player = Game::GetPlayerByIndex(i);
        if (player->m_interactbleGameObjectIndex != -1) {
            GameObject& gameObject = Scene::GetGamesObjects()[player->m_interactbleGameObjectIndex];
            for (RenderItem3D& renderItem : gameObject.GetRenderItems()) {
                shader.SetMat4("model", renderItem.modelMatrix);
                Mesh* mesh = AssetManager::GetMeshByIndex(renderItem.meshIndex);
                glDrawElementsBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), mesh->baseVertex);
            };
        }

       //for (GameObject& gameObject : Scene::GetGamesObjects()) {
       //    if (true) {
       //        for (RenderItem3D& renderItem : gameObject.GetRenderItems()) {
       //            shader.SetMat4("model", renderItem.modelMatrix);
       //            Mesh* mesh = AssetManager::GetMeshByIndex(renderItem.meshIndex);
       //            glDrawElementsBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), mesh->baseVertex);
       //        };
       //    }
       //}


        //if (OpenGLRenderer::g_sphereMesh.GetIndexCount() > 0) {
        //    glBindVertexArray(OpenGLRenderer::g_sphereMesh.GetVAO());
        //    glDrawElements(GL_TRIANGLES, OpenGLRenderer::g_sphereMesh.GetIndexCount(), GL_UNSIGNED_INT, 0);
        //}
    }
}

void OpenGLRenderer::CopyDepthBuffer(GLFrameBuffer & srcFrameBuffer, GLFrameBuffer & dstFrameBuffer) {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, srcFrameBuffer.GetHandle());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dstFrameBuffer.GetHandle());
    glBlitFramebuffer(0, 0, srcFrameBuffer.GetWidth(), srcFrameBuffer.GetHeight(), 0, 0, dstFrameBuffer.GetWidth(), dstFrameBuffer.GetHeight(), GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void OpenGLRenderer::CopyColorBuffer(GLFrameBuffer& srcFrameBuffer, GLFrameBuffer& dstFrameBuffer, const char* srcAttachmentName, const char* dstAttachmentName) {
    GLenum srcAttachmentSlot = srcFrameBuffer.GetColorAttachmentSlotByName(srcAttachmentName);
    GLenum dstAttachmentSlot = dstFrameBuffer.GetColorAttachmentSlotByName(dstAttachmentName);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, srcFrameBuffer.GetHandle());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dstFrameBuffer.GetHandle());
    glReadBuffer(srcAttachmentSlot);
    glDrawBuffer(dstAttachmentSlot);
    glBlitFramebuffer(0, 0, srcFrameBuffer.GetWidth(), srcFrameBuffer.GetHeight(), 0, 0, dstFrameBuffer.GetWidth(), dstFrameBuffer.GetHeight(), GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

GLFrameBuffer& OpenGLRenderer::GetHairFrameBuffer() {
    return OpenGLRenderer::g_frameBuffers.hair;
}

GLFrameBuffer& OpenGLRenderer::GetGBuffer() {
    return OpenGLRenderer::g_frameBuffers.gBuffer;
}

Shader& OpenGLRenderer::GetDepthPeelDepthShader() {
    return OpenGLRenderer::g_shaders.depthPeelDepth;
}

Shader& OpenGLRenderer::GetDepthPeelColorShader() {
    return OpenGLRenderer::g_shaders.depthPeelColor;
}

Shader& OpenGLRenderer::GetSolidColorShader() {
    return OpenGLRenderer::g_shaders.debugSolidColor;
}

ComputeShader& OpenGLRenderer::GetHairFinalCompositeShader() {
    return OpenGLRenderer::g_shaders.hairFinalComposite;
}

ComputeShader& OpenGLRenderer::GetHairLayerCompositeShader() {
    return OpenGLRenderer::g_shaders.hairLayerComposite;
}

void OpenGLRenderer::FlipbookPass() {

    Player* player = Game::GetPlayerByIndex(0);

    GLFrameBuffer frameBuffer = g_frameBuffers.gBuffer;
    frameBuffer.Bind();
    frameBuffer.SetViewport();
    frameBuffer.DrawBuffers({ "FinalLighting" });

    Shader& shader = g_shaders.flipbookNew;
    shader.Use();
    shader.SetMat4("projection", player->GetProjectionMatrix());
    shader.SetMat4("view", player->GetViewMatrix());

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());

    for (FlipbookObject& flipbookObject : Scene::g_flipbookObjects) {

        if (!flipbookObject.IsBillboard()) {
            shader.SetMat4("model", flipbookObject.GetModelMatrix());
        }
        else {
            shader.SetMat4("model", flipbookObject.GetBillboardModelMatrix(-player->GetCameraForward(), player->GetCameraRight(), player->GetCameraUp()));
        }
        shader.SetFloat("mixFactor", flipbookObject.GetMixFactor());
        shader.SetInt("index", flipbookObject.GetFrameIndex());
        shader.SetInt("indexNext", flipbookObject.GetNextFrameIndex());
        FlipbookTexture* flipbookTexture = AssetManager::GetFlipbookByName(flipbookObject.GetTextureName());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D_ARRAY, flipbookTexture->m_arrayHandle);


      // Model* model = AssetManager::GetModelByName("Quads");
      //
      // std::cout << "model.GetMeshCount()" << model->GetMeshCount() << "\n";
      // for (int i = 0; i < model->GetMeshIndices().size(); i++) {
      //     int meshIndex = model->GetMeshIndices()[i];
      //     Mesh* mesh = AssetManager::GetMeshByIndex(meshIndex);
      //     std::cout << " " << i << "; " << mesh->name << "\n";
      // }
      // std::cout << "\n";

        Mesh* mesh = AssetManager::GetMeshByModelNameAndMeshName("Quads", "FlipBookQuadBottomAligned");
        
        if (mesh) {
            glDrawElementsInstancedBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), 1, mesh->baseVertex);
        }

        glFinish();
    }

    // Cleanup
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
}

