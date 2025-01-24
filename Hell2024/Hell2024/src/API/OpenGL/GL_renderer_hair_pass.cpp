#include "GL_renderer.h"
#include "../../Core/AssetManager.h"
#include "../../Core/Audio.h"
#include "../../Input/Input.h"
#include "GL_backEnd.h"
#include "../../Game/Game.h"
#include "../../Game/Scene.h"
#include "../../Renderer/RendererUtil.hpp"
#include "../../Timer.hpp"

void SetViewportDIIIRTY(ViewportInfo viewportInfo) {
    glViewport(viewportInfo.xOffset, viewportInfo.yOffset, viewportInfo.width, viewportInfo.height);
}

void OpenGLRenderer::HairPass() {

    static int peelCount = 3;
    if (Input::KeyPressed(HELL_KEY_3) && peelCount < 7) {
        Audio::PlayAudio("UI_Select.wav", 1.0f);
        peelCount++;
        std::cout << "Depth peel layer count: " << peelCount << "\n";
    }
    if (Input::KeyPressed(HELL_KEY_4) && peelCount > 0) {
        Audio::PlayAudio("UI_Select.wav", 1.0f);
        peelCount--;
        std::cout << "Depth peel layer count: " << peelCount << "\n";
    }

    // Setup state
    GLFrameBuffer& hairFramebuffer = GetHairFrameBuffer();
    GLFrameBuffer& gBuffer = GetGBuffer();
    hairFramebuffer.Bind();
    hairFramebuffer.ClearAttachment("Composite", 0, 0, 0, 0);
    hairFramebuffer.SetViewport();

    glEnable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());

    std::vector<HairRenderItem> hairTopLayerRenderItems = Scene::GetHairTopLayerRenderItems();
    std::vector<HairRenderItem> hairBottomLayerRenderItems = Scene::GetHairBottomLayerRenderItems();

    OpenGLRenderer::RenderHairLayer(hairTopLayerRenderItems, peelCount);
    OpenGLRenderer::RenderHairLayer(hairBottomLayerRenderItems, peelCount);

    // Add the hair into the main image
    ComputeShader& hairFinalCompositeShader = GetHairFinalCompositeShader();
    hairFinalCompositeShader.Use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hairFramebuffer.GetColorAttachmentHandleByName("Composite"));
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gBuffer.GetColorAttachmentHandleByName("FinalLighting"));
    glBindImageTexture(0, gBuffer.GetColorAttachmentHandleByName("FinalLighting"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
    
    glDispatchCompute((gBuffer.GetWidth() + 7) / 8, (gBuffer.GetHeight() + 7) / 8, 1);

    // Cleanup
    glDepthFunc(GL_LESS);
}

void OpenGLRenderer::RenderHairLayer(std::vector<HairRenderItem>& renderItems, int peelCount) {

    GLFrameBuffer& hairFramebuffer = GetHairFrameBuffer();
    GLFrameBuffer& gBuffer = GetGBuffer();
    Shader& depthShader = GetDepthPeelDepthShader();
    Shader& colorShader = GetDepthPeelColorShader();
    ComputeShader& hairLayerCompositeShader = GetHairLayerCompositeShader();

    hairFramebuffer.Bind();
    hairFramebuffer.ClearAttachment("ViewspaceDepthPrevious", 1, 1, 1, 1);

    for (int i = 0; i < peelCount; i++) {
        // Viewspace depth pass
        CopyDepthBuffer(gBuffer, hairFramebuffer);

        for (int j = 0; j < Game::GetPlayerViewportCount(); j++) {
            int playerIndex = j;
            Player* player = Game::GetPlayerByIndex(playerIndex);
            ViewportInfo viewportInfo = RendererUtil::CreateViewportInfo(playerIndex, Game::GetSplitscreenMode(), hairFramebuffer.GetWidth(), hairFramebuffer.GetHeight());
            SetViewportDIIIRTY(viewportInfo);
            depthShader.SetInt("playerIndex", playerIndex);

            hairFramebuffer.Bind();
            hairFramebuffer.ClearAttachment("ViewspaceDepth", 0, 0, 0, 0);
            hairFramebuffer.DrawBuffer("ViewspaceDepth");
            glDepthFunc(GL_LESS);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, hairFramebuffer.GetColorAttachmentHandleByName("ViewspaceDepthPrevious"));
            depthShader.Use();
            depthShader.SetMat4("projectionView", player->GetProjectionMatrix() * player->GetViewMatrix());
            depthShader.SetMat4("view", player->GetViewMatrix());
            depthShader.SetFloat("nearPlane", NEAR_PLANE);
            depthShader.SetFloat("farPlane", FAR_PLANE);
            depthShader.SetFloat("viewportWidth", hairFramebuffer.GetWidth());
            depthShader.SetFloat("viewportHeight", hairFramebuffer.GetHeight());
            for (HairRenderItem& renderItem : renderItems) {
                depthShader.SetMat4("model", renderItem.modelMatrix);
                Mesh* mesh = AssetManager::GetMeshByIndex(renderItem.meshIndex);
                if (mesh) {
                    glDrawElementsInstancedBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), 1, mesh->baseVertex);
                }
            }
            // Color pass
            glDepthFunc(GL_EQUAL);
            hairFramebuffer.Bind();
            hairFramebuffer.ClearAttachment("Color", 0, 0, 0, 0);
            hairFramebuffer.DrawBuffer("Color");
            colorShader.Use();
            colorShader.SetMat4("projectionView", player->GetProjectionMatrix() * player->GetViewMatrix());
            colorShader.SetMat4("view", player->GetViewMatrix());
            colorShader.SetMat4("inverseView", player->GetInverseViewMatrix());
            colorShader.SetInt("playerIndex", j);

            for (HairRenderItem& renderItem : renderItems) {
                colorShader.SetMat4("model", renderItem.modelMatrix);
                colorShader.SetMat4("inverseModel", glm::inverse(renderItem.modelMatrix));
                Material* material = AssetManager::GetMaterialByIndex(renderItem.materialIndex);
                colorShader.SetInt("baseColorIndex", material->_basecolor);
                colorShader.SetInt("normalMapIndex", material->_normal);
                colorShader.SetInt("rmaIndex", material->_rma);
                Mesh* mesh = AssetManager::GetMeshByIndex(renderItem.meshIndex);
                if (mesh) {
                    glDrawElementsInstancedBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), 1, mesh->baseVertex);
                }
            }

            // TODO!: when you port this you can output previous viewspace depth in the pass above
            // TODO!: when you port this you can output previous viewspace depth in the pass above
            // TODO!: when you port this you can output previous viewspace depth in the pass above
            // TODO!: when you port this you can output previous viewspace depth in the pass above
            // TODO!: when you port this you can output previous viewspace depth in the pass above
            CopyColorBuffer(hairFramebuffer, hairFramebuffer, "ViewspaceDepth", "ViewspaceDepthPrevious");

            // Composite
            hairLayerCompositeShader.Use();
            hairLayerCompositeShader.SetInt("playerIndex", playerIndex);
            glBindImageTexture(0, hairFramebuffer.GetColorAttachmentHandleByName("Color"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
            glBindImageTexture(1, hairFramebuffer.GetColorAttachmentHandleByName("Composite"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
            //glDispatchCompute((hairFramebuffer.GetWidth() + 7) / 8, (hairFramebuffer.GetHeight() + 7) / 8, 1);
            glDispatchCompute((viewportInfo.width + 7) / 8, (viewportInfo.height + 7) / 8, 1);

        }
    }
}
