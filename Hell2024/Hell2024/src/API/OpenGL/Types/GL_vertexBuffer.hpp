#pragma once
#include "HellCommon.h"
#include <glad/glad.h>

struct OpenGLVertexBuffer {

public:
    void AllocateSpace(int vertexCount) {

        if (VAO == 0) {
            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);
        }
        // If there isn't enough space, destroy the buffer and recreate it
        GLsizeiptr bufferSize = vertexCount * sizeof(Vertex);
        if (allocatedSize < bufferSize) {
            glBindVertexArray(VAO);
            glDeleteBuffers(1, &VBO);
            glGenBuffers(1, &VBO);
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, bufferSize, nullptr, GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
            glEnableVertexAttribArray(3);
            glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));
            glEnableVertexAttribArray(0);
            glBindVertexArray(0);
            glBindVertexArray(0);
            allocatedSize = bufferSize;
        }
    }

    const GLuint& GetVAO() {
        return VAO;
    }

    const GLuint& GetVBO() {
        return VBO;
    }

private:
    GLuint VAO;
    GLuint VBO;
    GLsizeiptr allocatedSize;
};