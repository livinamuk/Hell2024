#pragma once
#include "../../API/OpenGL/Types/GL_vertexBuffer.hpp"
#include "../../API/Vulkan/Types/VK_vertexBuffer.hpp"

struct VertexBuffer {

    void AllocateSpace(int vertexCount);

    OpenGLVertexBuffer glVertexBuffer;
    VulkanVertexBuffer vkVertexBuffer;
};