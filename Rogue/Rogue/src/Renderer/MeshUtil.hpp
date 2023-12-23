#include "Mesh.h"
#include <iostream>

namespace MeshUtil {

	inline Mesh CreateUpFacingPlane(float xMin, float xMax, float zMin, float zMax, float height) {
        float width = xMax - xMin;
        float depth = zMax - zMin;
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        vertices.push_back({ glm::vec3(xMin, height, zMax), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(width, depth) });
        vertices.push_back({ glm::vec3(xMin, height, zMin), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(width, 0.0f) });
        vertices.push_back({ glm::vec3(xMax, height, zMin), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 0.0f) });
        vertices.push_back({ glm::vec3(xMax, height, zMax), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, depth) });
        indices.push_back(3);
        indices.push_back(1);
        indices.push_back(0);
        indices.push_back(3);
        indices.push_back(2);
        indices.push_back(1);
        return Mesh(vertices, indices, "UpPlane");
	}

	inline Mesh CreateDownFacingPlane(float xMin, float xMax, float zMin, float zMax, float height) {
        float width = xMax - xMin;
        float depth = zMax - zMin;
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        vertices.push_back({ glm::vec3(xMin, height, zMax), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(width, depth) });
        vertices.push_back({ glm::vec3(xMin, height, zMin), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(width, 0.0f) });
        vertices.push_back({ glm::vec3(xMax, height, zMin), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 0.0f) });
        vertices.push_back({ glm::vec3(xMax, height, zMax), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, depth) });
        indices.push_back(0);
        indices.push_back(1);
        indices.push_back(3);
        indices.push_back(1);
        indices.push_back(2);
        indices.push_back(3);
        return Mesh(vertices, indices, "DownPlane");
	}

    inline Mesh CreateCube(float size, float textureScaling = 1.0f, bool outFacing = true) {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        float d = size * 0.5f;
        // Top
        vertices.push_back({ glm::vec3(-d, d, d), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0, textureScaling) });
        vertices.push_back({ glm::vec3(-d, d, -d), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0, 0.0f) });
        vertices.push_back({ glm::vec3(d, d, -d), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(textureScaling, 0.0f) });
        vertices.push_back({ glm::vec3(d, d, d), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(textureScaling, textureScaling) });
        // Bottom
        vertices.push_back({ glm::vec3(-d, -d, d), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(textureScaling, textureScaling) });
        vertices.push_back({ glm::vec3(-d, -d, -d), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(textureScaling, 0.0f) });
        vertices.push_back({ glm::vec3(d, -d, -d), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0, 0.0f) });
        vertices.push_back({ glm::vec3(d, -d, d), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0, textureScaling) });
        // Z front
        vertices.push_back({ glm::vec3(-d, d, d), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0, 0) });
        vertices.push_back({ glm::vec3(-d, -d, d), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0, textureScaling) });
        vertices.push_back({ glm::vec3(d, -d, d), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(textureScaling, textureScaling) });
        vertices.push_back({ glm::vec3(d, d, d), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(textureScaling, 0) });
        // Z back
        vertices.push_back({ glm::vec3(-d, d, -d), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(textureScaling, 0) });
        vertices.push_back({ glm::vec3(-d, -d, -d), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(textureScaling, textureScaling) });
        vertices.push_back({ glm::vec3(d, -d, -d), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0, textureScaling) });
        vertices.push_back({ glm::vec3(d, d, -d), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0, 0) });
        // X front
        vertices.push_back({ glm::vec3(d, d, -d), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(textureScaling, 0) });
        vertices.push_back({ glm::vec3(d, -d, -d), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(textureScaling, textureScaling) });
        vertices.push_back({ glm::vec3(d, -d, d), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0, textureScaling) });
        vertices.push_back({ glm::vec3(d, d, d), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0, 0) });
        // X back
        vertices.push_back({ glm::vec3(-d, d, -d), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0, 0) });
        vertices.push_back({ glm::vec3(-d, -d, -d), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0, textureScaling) });
        vertices.push_back({ glm::vec3(-d, -d, d), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(textureScaling, textureScaling) });
        vertices.push_back({ glm::vec3(-d, d, d), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(textureScaling, 0) });
        if (outFacing)
            indices = { 3, 1, 0, 3, 2, 1, 4, 5, 7, 5, 6, 7, 8, 9, 11, 9, 10, 11, 15, 13, 12, 15, 14, 13, 19, 17, 16, 19, 18, 17, 20, 21, 23, 21, 22, 23 };
        else
            indices = { 0, 1, 3, 1, 2, 3, 7, 5, 4, 7, 6, 5, 11, 9, 8, 11, 10, 9, 12, 13, 15, 13, 14, 15, 16, 17, 19, 17, 18, 19, 23, 21, 20, 23, 22, 21 };
        return Mesh(vertices, indices, "Cube");
    }

    inline Mesh CreateCuboid(float width, float height, float depth, float textureScaling = 1.0f, bool outFacing = true) {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        float w = width * 0.5f;
        float h = height * 0.5f;
        float d = depth * 0.5f;
        // Top
        vertices.push_back({ glm::vec3(-w, h, d), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0, depth * textureScaling) });
        vertices.push_back({ glm::vec3(-w, h, -d), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0, 0.0f) });
        vertices.push_back({ glm::vec3(w, h, -d), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(width * textureScaling, 0.0f) });
        vertices.push_back({ glm::vec3(w, h, d), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(width * textureScaling, depth * textureScaling) });
        // Bottom
        vertices.push_back({ glm::vec3(-w, -h, d), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(width * textureScaling, depth * textureScaling) });
        vertices.push_back({ glm::vec3(-w, -h, -d), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(width * textureScaling, 0.0f) });
        vertices.push_back({ glm::vec3(w, -h, -d), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0, 0.0f) });
        vertices.push_back({ glm::vec3(w, -h, d), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0, depth * textureScaling) });
        // X front
        vertices.push_back({ glm::vec3(-w, h, d), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0, 0) });
        vertices.push_back({ glm::vec3(-w, -h, d), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0, height * textureScaling) });
        vertices.push_back({ glm::vec3(w, -h, d), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(width * textureScaling, height * textureScaling) });
        vertices.push_back({ glm::vec3(w, h, d), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(width * textureScaling, 0) });
        // X back
        vertices.push_back({ glm::vec3(-w, h, -d), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(width * textureScaling, 0) });
        vertices.push_back({ glm::vec3(-w, -h, -d), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(width * textureScaling, height * textureScaling) });
        vertices.push_back({ glm::vec3(w, -h, -d), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0, height * textureScaling) });
        vertices.push_back({ glm::vec3(w, h, -d), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0, 0) });
        // Z front
        vertices.push_back({ glm::vec3(w, h, -d), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(depth * textureScaling, 0) });
        vertices.push_back({ glm::vec3(w, -h, -d), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(depth * textureScaling, height * textureScaling) });
        vertices.push_back({ glm::vec3(w, -h, d), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0, height * textureScaling) });
        vertices.push_back({ glm::vec3(w, h, d), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0, 0) });
        // Z back
        vertices.push_back({ glm::vec3(-w, h, -d), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0, 0) });
        vertices.push_back({ glm::vec3(-w, -h, -d), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0, height * textureScaling) });
        vertices.push_back({ glm::vec3(-w, -h, d), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(depth * textureScaling,  height * textureScaling) });
        vertices.push_back({ glm::vec3(-w, h, d), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(depth * textureScaling, 0) });
        if (outFacing)
            indices = { 3, 1, 0, 3, 2, 1, 4, 5, 7, 5, 6, 7, 8, 9, 11, 9, 10, 11, 15, 13, 12, 15, 14, 13, 19, 17, 16, 19, 18, 17, 20, 21, 23, 21, 22, 23 };
        else
            indices = { 0, 1, 3, 1, 2, 3, 7, 5, 4, 7, 6, 5, 11, 9, 8, 11, 10, 9, 12, 13, 15, 13, 14, 15, 16, 17, 19, 17, 18, 19, 23, 21, 20, 23, 22, 21 };
        return Mesh(vertices, indices, "Cuboid");
    }

    inline Mesh CreateCubeFaceZFront(float size, float textureScaling = 1.0f) {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        float d = size * 0.5f;
        vertices.push_back({ glm::vec3(-d, d, d), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0, 0) });
        vertices.push_back({ glm::vec3(-d, -d, d), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0, textureScaling) });
        vertices.push_back({ glm::vec3(d, -d, d), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(textureScaling, textureScaling) });
        vertices.push_back({ glm::vec3(d, d, d), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(textureScaling, 0) });
        indices = { 0, 1, 3, 1, 2, 3 };
        return Mesh(vertices, indices, "VoxelZFaceFront");
    }    

    inline Mesh CreateCubeFaceZBack(float size, float textureScaling = 1.0f) {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        float d = size * 0.5f;
        vertices.push_back({ glm::vec3(-d, d, -d), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(textureScaling, 0) });
        vertices.push_back({ glm::vec3(-d, -d, -d), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(textureScaling, textureScaling) });
        vertices.push_back({ glm::vec3(d, -d, -d), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(0, textureScaling) });
        vertices.push_back({ glm::vec3(d, d, -d), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(0, 0) });
        indices = { 3, 1, 0, 3, 2, 1 };
        return Mesh(vertices, indices, "VoxelZFaceBack");
    }

    inline Mesh CreateCubeFaceXFront(float size, float textureScaling = 1.0f) {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        float d = size * 0.5f;
        vertices.push_back({ glm::vec3(d, d, -d), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(textureScaling, 0) });
        vertices.push_back({ glm::vec3(d, -d, -d), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(textureScaling, textureScaling) });
        vertices.push_back({ glm::vec3(d, -d, d), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(0, textureScaling) });
        vertices.push_back({ glm::vec3(d, d, d), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(0, 0) });
        indices = { 3, 1, 0, 3, 2, 1 };
        return Mesh(vertices, indices, "VoxelXFaceFront");
    }

    inline Mesh CreateCubeFaceXBack(float size, float textureScaling = 1.0f) {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        float d = size * 0.5f;
        vertices.push_back({ glm::vec3(-d, d, -d), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(0, 0) });
        vertices.push_back({ glm::vec3(-d, -d, -d), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(0, textureScaling) });
        vertices.push_back({ glm::vec3(-d, -d, d), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(textureScaling, textureScaling) });
        vertices.push_back({ glm::vec3(-d, d, d), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(textureScaling, 0) });
        indices = { 0, 1, 3, 1, 2, 3 };
        return Mesh(vertices, indices, "VoxelXFaceBack");
    }
    inline Mesh CreateCubeFaceYTop(float size, float textureScaling = 1.0f) {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        float d = size * 0.5f;
        vertices.push_back({ glm::vec3(-d, d, d), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0, textureScaling) });
        vertices.push_back({ glm::vec3(-d, d, -d), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0, 0.0f) });
        vertices.push_back({ glm::vec3(d, d, -d), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(textureScaling, 0.0f) });
        vertices.push_back({ glm::vec3(d, d, d), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(textureScaling, textureScaling) });
        indices = { 3, 1, 0, 3, 2, 1 };
        return Mesh(vertices, indices, "VoxelYFaceUp");
    }
    inline Mesh CreateCubeFaceYBottom(float size, float textureScaling = 1.0f) {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        float d = size * 0.5f;
        vertices.push_back({ glm::vec3(-d, -d, d), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(textureScaling, textureScaling) });
        vertices.push_back({ glm::vec3(-d, -d, -d), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(textureScaling, 0.0f) });
        vertices.push_back({ glm::vec3(d, -d, -d), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(0, 0.0f) });
        vertices.push_back({ glm::vec3(d, -d, d), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(0, textureScaling) });
        indices = { 0, 1, 3, 1, 2, 3 };
        return Mesh(vertices, indices, "VoxelYFaceDown");
    }
}