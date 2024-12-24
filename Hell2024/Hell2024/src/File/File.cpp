#include "File.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cstdint>
#include "../Util.hpp"

bool debugPringHeaders = false;
bool debugPrintData = false;

void File::ExportModel(const ModelData& modelData) {
    std::string outputPath = MODEL_DIR + modelData.name + ".model";
    std::ofstream file(outputPath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for writing: " << outputPath << "\n";
        return;
    }
    ModelHeader header;
    header.version = 1;
    header.meshCount = modelData.meshCount;
    header.nameLength = modelData.name.size();
    file.write((char*)&header.version, sizeof(header.version));
    file.write((char*)&header.meshCount, sizeof(header.meshCount));
    file.write((char*)&header.nameLength, sizeof(header.nameLength));
    file.write(modelData.name.data(), header.nameLength);
    if (debugPringHeaders) {
        std::cout << "\nWROTE MODEL\n";
        std::cout << " Name: " << modelData.name << "\n";
        PrintModelHeader(header);
    }
    for (auto& mesh : modelData.meshes) {
        MeshHeader meshHeader;
        meshHeader.nameLength = (uint32_t)mesh.name.size();
        meshHeader.vertexCount = (uint32_t)mesh.vertices.size();
        meshHeader.indexCount = (uint32_t)mesh.indices.size();
        meshHeader.aabbMin = mesh.aabbMin;
        meshHeader.aabbMax = mesh.aabbMax;
        file.write((char*)&meshHeader.nameLength, sizeof(meshHeader.nameLength));
        file.write((char*)&meshHeader.vertexCount, sizeof(meshHeader.vertexCount));
        file.write((char*)&meshHeader.indexCount, sizeof(meshHeader.indexCount));
        file.write(mesh.name.data(), meshHeader.nameLength);
        file.write(reinterpret_cast<const char*>(&meshHeader.aabbMin), sizeof(glm::vec3));
        file.write(reinterpret_cast<const char*>(&meshHeader.aabbMax), sizeof(glm::vec3));
        file.write(reinterpret_cast<const char*>(mesh.vertices.data()), mesh.vertices.size() * sizeof(Vertex));
        file.write(reinterpret_cast<const char*>(mesh.indices.data()), mesh.indices.size() * sizeof(uint32_t));
        if (debugPringHeaders) {
            std::cout << "\nWROTE MESH\n";
            std::cout << " Name: " << mesh.name << "\n";
            PrintMeshHeader(meshHeader);
        }
    }
    file.close();
    std::cout << "Exported: " << outputPath << "\n";
}


ModelData File::ImportModel(const std::string& filename) {
    ModelData modelData;
    std::ifstream file(MODEL_DIR + filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for reading: " << MODEL_DIR + filename << "\n";
        return modelData;
    }
    ModelHeader header;
    file.read((char*)&header.version, sizeof(header.version));
    file.read((char*)&header.meshCount, sizeof(header.meshCount));
    file.read((char*)&header.nameLength, sizeof(header.nameLength));
    std::string modelName(header.nameLength, '\0');
    file.read(&modelName[0], header.nameLength);
    if (debugPringHeaders) {
        std::cout << "\nREAD MODEL\n";
        std::cout << " Name: " << modelData.name << "\n";
        PrintModelHeader(header);
    }
    modelData.meshCount = header.meshCount;
    modelData.meshes.resize(header.meshCount);
    modelData.name = modelName;

    glm::vec3 modelAabbMin = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 modelAabbMax = glm::vec3(-std::numeric_limits<float>::max());

    for (uint32_t i = 0; i < header.meshCount; ++i) {
        MeshHeader meshHeader;
        file.read((char*)&meshHeader.nameLength, sizeof(meshHeader.nameLength));
        file.read((char*)&meshHeader.vertexCount, sizeof(meshHeader.vertexCount));
        file.read((char*)&meshHeader.indexCount, sizeof(meshHeader.indexCount));
        std::string meshName(meshHeader.nameLength, '\0');
        file.read(&meshName[0], meshHeader.nameLength);
        file.read(reinterpret_cast<char*>(&meshHeader.aabbMin), sizeof(glm::vec3));
        file.read(reinterpret_cast<char*>(&meshHeader.aabbMax), sizeof(glm::vec3));

        MeshData& meshData = modelData.meshes[i];
        meshData.name = meshName;
        meshData.vertexCount = meshHeader.vertexCount;
        meshData.indexCount = meshHeader.indexCount;
        meshData.vertices.resize(meshData.vertexCount);
        meshData.indices.resize(meshData.indexCount);
        meshData.aabbMin = meshHeader.aabbMin;
        meshData.aabbMax = meshHeader.aabbMax;
        file.read(reinterpret_cast<char*>(meshData.vertices.data()), meshHeader.vertexCount * sizeof(Vertex));
        file.read(reinterpret_cast<char*>(meshData.indices.data()), meshHeader.indexCount * sizeof(uint32_t));
        if (debugPringHeaders) {
            std::cout << "\nREAD MESH\n";
            std::cout << " Name: " << meshData.name << "\n";
            PrintMeshHeader(meshHeader);
        }
        if (debugPrintData) {
            for (Vertex& vertex : meshData.vertices) {
                std::cout << " " << Util::Vec3ToString(vertex.position) << " " << Util::Vec3ToString(vertex.normal) << "\n";
            }
            for (uint32_t& index : meshData.indices) {
                std::cout << " " << index << "\n";
            }
        }
        modelAabbMin = Util::Vec3Min(modelAabbMin, meshHeader.aabbMin);
        modelAabbMax = Util::Vec3Max(modelAabbMax, meshHeader.aabbMax);
    }
    modelData.aabbMin = modelAabbMin;
    modelData.aabbMax = modelAabbMax;

    file.close();
    //std::cout << "Loaded: " << filename << "\n";
    return modelData;
}

void File::PrintModelHeader(ModelHeader header) {
    std::cout << " Version: " << header.version << "\n";
    std::cout << " Mesh Count: " << header.meshCount << "\n";
    std::cout << " Name Length: " << header.nameLength << "\n";
}

void File::PrintMeshHeader(MeshHeader header) {
    std::cout << " Name Length: " << header.nameLength << "\n";
    std::cout << " Vertex Count: " << header.vertexCount << "\n";
    std::cout << " Index Count: " << header.indexCount << "\n";
    std::cout << " AABB min: " << Util::Vec3ToString(header.aabbMin) << "\n";
    std::cout << " AABB max: " << Util::Vec3ToString(header.aabbMax) << "\n";
}

void File::Test() {

    MeshData meshDataA;
    meshDataA.name = "MeshA";
    meshDataA.vertices.push_back(Vertex(glm::vec3(0, 1, 2), glm::vec3(1, 1, 1)));
    meshDataA.vertices.push_back(Vertex(glm::vec3(2, 3, 4), glm::vec3(2, 2, 2)));
    meshDataA.vertices.push_back(Vertex(glm::vec3(5, 6, 7), glm::vec3(3, 3, 3)));
    meshDataA.vertexCount = 3;
    meshDataA.indices = { 0, 1, 2 };
    meshDataA.indexCount = 3;

    MeshData meshDataB;
    meshDataB.name = "MeshB";
    meshDataB.vertices.push_back(Vertex(glm::vec3(8, 9, 10), glm::vec3(1, 1, 1)));
    meshDataB.vertices.push_back(Vertex(glm::vec3(11, 12, 13), glm::vec3(2, 2, 2)));
    meshDataB.vertices.push_back(Vertex(glm::vec3(14, 15, 16), glm::vec3(3, 3, 3)));
    meshDataB.vertexCount = 3;
    meshDataB.indices = { 0, 1, 2 };
    meshDataB.indexCount = 3;

    ModelData modelDataOut;
    modelDataOut.name = "TestModel";
    modelDataOut.meshCount = 2;
    modelDataOut.meshes.push_back(meshDataA);
    modelDataOut.meshes.push_back(meshDataB);
    modelDataOut.meshCount = 2;
    File::ExportModel(modelDataOut);

    ModelData modelDataIn;
    File::ImportModel("TestModel.model");

}