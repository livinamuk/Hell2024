#include "GL_backEnd.h"
#include "Types/GL_vertexBuffer.hpp"
#include "../../Core/AssetManager.h"
#include "../../Util.hpp"
#include <iostream>
#include <string>

namespace OpenGLBackEnd {

    GLuint _vertexDataVAO = 0;
    GLuint _vertexDataVBO = 0;
    GLuint _vertexDataEBO = 0;

    GLuint _weightedVertexDataVAO = 0;
    GLuint _weightedVertexDataVBO = 0;
    GLuint _weightedVertexDataEBO = 0;

    GLuint g_skinnedVertexDataVAO = 0;
    GLuint g_skinnedVertexDataVBO = 0;
    GLuint g_allocatedSkinnedVertexBufferSize = 0;

    GLuint g_pointCloudVAO = 0;
    GLuint g_pointCloudVBO = 0;

    GLuint g_constructiveSolidGeometryVAO = 0;
    GLuint g_constructiveSolidGeometryVBO = 0;
    GLuint g_constructiveSolidGeometryEBO = 0;

    GLuint g_triangle2DVAO = 0;
    GLuint g_triangle2DVBO = 0;

    GLuint GetVertexDataVAO() {
        return _vertexDataVAO;
    }

    GLuint GetVertexDataVBO() {
        return _vertexDataVBO;
    }

    GLuint GetVertexDataEBO() {
        return _vertexDataEBO;
    }

    GLuint GetWeightedVertexDataVAO() {
        return _weightedVertexDataVAO;
    }

    GLuint GetWeightedVertexDataVBO() {
        return _weightedVertexDataVBO;
    }

    GLuint GetWeightedVertexDataEBO() {
        return _weightedVertexDataEBO;
    }

    GLuint GetSkinnedVertexDataVAO() {
        return g_skinnedVertexDataVAO;
    }

    GLuint GetSkinnedVertexDataVBO() {
        return g_skinnedVertexDataVBO;
    }

    GLuint GetPointCloudVAO() {
        return g_pointCloudVAO;
    }

    GLuint GetPointCloudVBO() {
        return g_pointCloudVBO;
    }

    GLuint GetCSGVAO() {
        return g_constructiveSolidGeometryVAO;
    }

    GLuint GetCSGVBO() {
        return g_constructiveSolidGeometryVBO;
    }

    GLuint GetCSGEBO() {
        return g_constructiveSolidGeometryEBO;
    }
}

GLenum glCheckError_(const char* file, int line) {
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR)
    {
        std::string error;
        switch (errorCode)
        {
        case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
        case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
        case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
        case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
        case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
        case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
        case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
        }
        std::cout << error << " | " << file << " (" << line << ")\n";
    }
    return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void APIENTRY glDebugOutput(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei /*length*/, const char* message, const void* /*userParam*/) {
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return; // ignore these non-significant error codes
    std::cout << "---------------\n";
    std::cout << "Debug message (" << id << "): " << message << "\n";
    switch (source){
        case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
        case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
        case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
    }
    std::cout << "\n";
    switch (type) {
        case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break;
        case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
        case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
        case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
        case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
        case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
        case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
    }
    std::cout << "\n";
        switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
        case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
        case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
        }    std::cout << "\n\n\n";
}

void QuerySizes() {
    GLint max_layers;
    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &max_layers);
    std::cout << "Max texture array size is: " << max_layers << "\n";
    int max_compute_work_group_count[3];
    int max_compute_work_group_size[3];
    int max_compute_work_group_invocations;
    for (int idx = 0; idx < 3; idx++) {
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, idx, &max_compute_work_group_count[idx]);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, idx, &max_compute_work_group_size[idx]);
    }
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &max_compute_work_group_invocations);
    std::cout << "Max number of work groups in X dimension " << max_compute_work_group_count[0] << "\n";
    std::cout << "Max number of work groups in Y dimension " << max_compute_work_group_count[1] << "\n";
    std::cout << "Max number of work groups in Z dimension " << max_compute_work_group_count[2] << "\n";
    std::cout << "Max size of a work group in X dimension " << max_compute_work_group_size[0] << "\n";
    std::cout << "Max size of a work group in Y dimension " << max_compute_work_group_size[1] << "\n";
    std::cout << "Max size of a work group in Z dimension " << max_compute_work_group_size[2] << "\n";
    std::cout << "Number of invocations in a single local work group that may be dispatched to a compute shader " << max_compute_work_group_invocations << "\n";
}

void OpenGLBackEnd::InitMinimum() {
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD\n";
        return;
    }
    GLint major, minor;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    const GLubyte* vendor = glGetString(GL_VENDOR);
    const GLubyte* renderer = glGetString(GL_RENDERER);
    std::cout << "\nGPU: " << renderer << "\n";
    std::cout << "GL version: " << major << "." << minor << "\n\n";

    int flags;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
        //std::cout << "Debug GL context enabled\n";
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // makes sure errors are displayed synchronously
        glDebugMessageCallback(glDebugOutput, nullptr);
    }
    else {
        std::cout << "Debug GL context not available\n";
    }

    // Clear screen to black
    glClear(GL_COLOR_BUFFER_BIT);
}

void OpenGLBackEnd::AllocateSkinnedVertexBufferSpace(int vertexCount) {

    if (g_skinnedVertexDataVAO == 0) {
        glGenVertexArrays(1, &g_skinnedVertexDataVAO);
    }
    // Check if there is enough space
    if (g_allocatedSkinnedVertexBufferSize < vertexCount * sizeof(Vertex)) {

        // Destroy old VBO
        if (g_skinnedVertexDataVBO != 0) {
            glDeleteBuffers(1, &g_skinnedVertexDataVBO);
        }

        // Create new one
        glBindVertexArray(g_skinnedVertexDataVAO);
        glGenBuffers(1, &g_skinnedVertexDataVBO);
        glBindBuffer(GL_ARRAY_BUFFER, g_skinnedVertexDataVBO);
        glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(Vertex), nullptr, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        g_allocatedSkinnedVertexBufferSize = vertexCount * sizeof(Vertex);
    }
}

void OpenGLBackEnd::UploadVertexData(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {

    if (_vertexDataVAO != 0) {
        glDeleteVertexArrays(1, &_vertexDataVAO);
        glDeleteBuffers(1, &_vertexDataVBO);
        glDeleteBuffers(1, &_vertexDataEBO);
    }

    glGenVertexArrays(1, &_vertexDataVAO);
    glGenBuffers(1, &_vertexDataVBO);
    glGenBuffers(1, &_vertexDataEBO);

    glBindVertexArray(_vertexDataVAO);
    glBindBuffer(GL_ARRAY_BUFFER, _vertexDataVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _vertexDataEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), &indices[0], GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void OpenGLBackEnd::UploadConstructiveSolidGeometry(std::vector<CSGVertex>& vertices, std::vector<uint32_t>& indices) {

    if (vertices.empty()) {
        return;
    }

    if (g_constructiveSolidGeometryVAO != 0) {
        glDeleteVertexArrays(1, &g_constructiveSolidGeometryVAO);
        glDeleteBuffers(1, &g_constructiveSolidGeometryVBO);
        glDeleteBuffers(1, &g_constructiveSolidGeometryEBO);
    }

    glGenVertexArrays(1, &g_constructiveSolidGeometryVAO);
    glGenBuffers(1, &g_constructiveSolidGeometryVBO);
    glGenBuffers(1, &g_constructiveSolidGeometryEBO);

    glBindVertexArray(g_constructiveSolidGeometryVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_constructiveSolidGeometryVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(CSGVertex), &vertices[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_constructiveSolidGeometryEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), &indices[0], GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(CSGVertex), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(CSGVertex), (void*)offsetof(CSGVertex, normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(CSGVertex), (void*)offsetof(CSGVertex, uv));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(CSGVertex), (void*)offsetof(CSGVertex, tangent));
    glEnableVertexAttribArray(4);
    glVertexAttribIPointer(4, 1, GL_INT, sizeof(CSGVertex), (void*)offsetof(CSGVertex, materialIndex));

    glBindVertexArray(0);
    /*
    std::cout << "Uploaded constructive geometry to gpu\n";
    std::cout << "-vertices: " << vertices.size() << "\n";
    std::cout << "-indices: " << indices.size() << "\n";
    std::cout << "-vao: " << g_constructiveSolidGeometryVAO << "\n";
    std::cout << "-vbo: " << g_constructiveSolidGeometryVBO << "\n";
    std::cout << "-ebo: " << g_constructiveSolidGeometryEBO << "\n";*/
}

void OpenGLBackEnd::UploadWeightedVertexData(std::vector<WeightedVertex>& vertices, std::vector<uint32_t>& indices) {

    if (vertices.empty() || indices.empty()) {
        return;
    }

    if (_weightedVertexDataVAO != 0) {
        glDeleteVertexArrays(1, &_weightedVertexDataVAO);
        glDeleteBuffers(1, &_weightedVertexDataVBO);
        glDeleteBuffers(1, &_weightedVertexDataEBO);
    }

    glGenVertexArrays(1, &_weightedVertexDataVAO);
    glGenBuffers(1, &_weightedVertexDataVBO);
    glGenBuffers(1, &_weightedVertexDataEBO);

    glBindVertexArray(_weightedVertexDataVAO);
    glBindBuffer(GL_ARRAY_BUFFER, _weightedVertexDataVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(WeightedVertex), &vertices[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _weightedVertexDataEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), &indices[0], GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(WeightedVertex), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(WeightedVertex), (void*)offsetof(WeightedVertex, normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(WeightedVertex), (void*)offsetof(WeightedVertex, uv));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(WeightedVertex), (void*)offsetof(WeightedVertex, tangent));
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_INT, sizeof(WeightedVertex), (void*)offsetof(WeightedVertex, boneID));
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(WeightedVertex), (void*)offsetof(WeightedVertex, weight));

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void OpenGLBackEnd::CreatePointCloudVertexBuffer(std::vector<CloudPoint>& pointCloud) {
    static int allocatedBufferSize = 0;
    if (g_pointCloudVAO == 0) {
        glGenVertexArrays(1, &g_pointCloudVAO);
        glGenBuffers(1, &g_pointCloudVBO);
    }
    if (pointCloud.empty()) {
        return;
    }
    glBindVertexArray(g_pointCloudVAO);
    if (pointCloud.size() * sizeof(CloudPoint) <= allocatedBufferSize) {
        glBindBuffer(GL_ARRAY_BUFFER, g_pointCloudVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, (GLuint)pointCloud.size() * sizeof(CloudPoint), &pointCloud[0]);
    }
    else {
        glDeleteBuffers(1, &g_pointCloudVBO);
        glGenBuffers(1, &g_pointCloudVBO);
        glBindBuffer(GL_ARRAY_BUFFER, g_pointCloudVBO);
        glBufferData(GL_ARRAY_BUFFER, pointCloud.size() * sizeof(CloudPoint), &pointCloud[0], GL_STATIC_DRAW);
    }
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(CloudPoint), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(CloudPoint), (void*)offsetof(CloudPoint, normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(CloudPoint), (void*)offsetof(CloudPoint, directLighting));
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    allocatedBufferSize = pointCloud.size() * sizeof(CloudPoint);
}

void OpenGLBackEnd::UploadTriangle2DData(std::vector<glm::vec2>& vertices) {

    static int allocatedBufferSize = 0;
    if (g_triangle2DVAO == 0) {
        glGenVertexArrays(1, &g_triangle2DVAO);
        glGenBuffers(1, &g_triangle2DVBO);
    }
    if (vertices.empty()) {
        return;
    }
    glBindVertexArray(g_triangle2DVAO);
    if (vertices.size() * sizeof(glm::vec2) <= allocatedBufferSize) {
        glBindBuffer(GL_ARRAY_BUFFER, g_triangle2DVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, (GLuint)vertices.size() * sizeof(glm::vec2), &vertices[0]);
    }
    else {
        glDeleteBuffers(1, &g_triangle2DVBO);
        glGenBuffers(1, &g_triangle2DVBO);
        glBindBuffer(GL_ARRAY_BUFFER, g_triangle2DVBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec2), &vertices[0], GL_STATIC_DRAW);
    }
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    allocatedBufferSize = vertices.size() * sizeof(glm::vec2);
}

GLuint OpenGLBackEnd::GetTriangles2DVAO() {
    return g_triangle2DVAO;
}

GLuint OpenGLBackEnd::GetTriangles2DVBO() {
    return g_triangle2DVBO;

}