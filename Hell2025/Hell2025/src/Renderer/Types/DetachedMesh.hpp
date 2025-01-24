#pragma once
#include "RendererCommon.h"
#include "../../API/Vulkan/Types/vk_detachedMesh.hpp"
#include "../../API/OpenGL/Types/gl_detachedMesh.hpp"
#include "../../BackEnd/BackEnd.h"

struct DetachedMesh {

private:
    OpenGLDetachedMesh openglDetachedMesh;
    VulkanDetachedMesh vulkanDetachedMesh;

public:
    void UpdateVertexBuffer(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
        if (BackEnd::GetAPI() == API::OPENGL) {
            openglDetachedMesh.UpdateVertexBuffer(vertices, indices);
        }
        else if (BackEnd::GetAPI() == API::VULKAN) {
            vulkanDetachedMesh.UpdateVertexBuffer(vertices, indices);
        }
    }
    OpenGLDetachedMesh& GetGLMesh() {
        return openglDetachedMesh;
    }
    VulkanDetachedMesh& GetVKMesh() {
        return vulkanDetachedMesh;
    }
};