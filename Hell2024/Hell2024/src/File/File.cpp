#include "File.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cstdint>
#include <chrono>
#include "../Util.hpp"

#define PRINT_MODEL_HEADERS_ON_READ 0
#define PRINT_MODEL_HEADERS_ON_WRITE 0
#define PRINT_MESH_HEADERS_ON_READ 0
#define PRINT_MESH_HEADERS_ON_WRITE 0

/*
█▄ ▄█ █▀█ █▀▄ █▀▀ █   █▀▀
█ █ █ █ █ █ █ █▀▀ █   ▀▀█
▀   ▀ ▀▀▀ ▀▀  ▀▀▀ ▀▀▀ ▀▀▀ */

void File::ExportModel(const ModelData& modelData) {
    std::string outputPath = "res/assets/models/" + modelData.name + ".model";
    std::ofstream file(outputPath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for writing: " << outputPath << "\n";
        return;
    }
    ModelHeader modelHeader;
    modelHeader.version = 1;
    modelHeader.meshCount = modelData.meshCount;
    modelHeader.nameLength = modelData.name.size();
    modelHeader.timestamp = modelData.timestamp;
    modelHeader.aabbMin = modelData.aabbMin;
    modelHeader.aabbMax = modelData.aabbMax;
    file.write((char*)&modelHeader.version, sizeof(modelHeader.version));
    file.write((char*)&modelHeader.meshCount, sizeof(modelHeader.meshCount));
    file.write((char*)&modelHeader.nameLength, sizeof(modelHeader.nameLength));
    file.write(modelData.name.data(), modelHeader.nameLength);
    file.write(reinterpret_cast<char*>(&modelHeader.timestamp), sizeof(modelHeader.timestamp));
    file.write(reinterpret_cast<const char*>(&modelHeader.aabbMin), sizeof(glm::vec3));
    file.write(reinterpret_cast<const char*>(&modelHeader.aabbMax), sizeof(glm::vec3));
#if PRINT_MODEL_HEADERS_ON_WRITE
    PrintModelHeader(modelHeader, "Wrote model header: " + modelData.name);
#endif
    for (const MeshData& meshData : modelData.meshes) {
        MeshHeader meshHeader;
        meshHeader.nameLength = (uint32_t)meshData.name.size();
        meshHeader.vertexCount = (uint32_t)meshData.vertices.size();
        meshHeader.indexCount = (uint32_t)meshData.indices.size();
        meshHeader.aabbMin = meshData.aabbMin;
        meshHeader.aabbMax = meshData.aabbMax;
        file.write((char*)&meshHeader.nameLength, sizeof(meshHeader.nameLength));
        file.write((char*)&meshHeader.vertexCount, sizeof(meshHeader.vertexCount));
        file.write((char*)&meshHeader.indexCount, sizeof(meshHeader.indexCount));
        file.write(meshData.name.data(), meshHeader.nameLength);
        file.write(reinterpret_cast<const char*>(&meshHeader.aabbMin), sizeof(glm::vec3));
        file.write(reinterpret_cast<const char*>(&meshHeader.aabbMax), sizeof(glm::vec3));
        file.write(reinterpret_cast<const char*>(meshData.vertices.data()), meshData.vertices.size() * sizeof(Vertex));
        file.write(reinterpret_cast<const char*>(meshData.indices.data()), meshData.indices.size() * sizeof(uint32_t));
#if PRINT_MESH_HEADERS_ON_WRITE
        PrintMeshHeader(meshHeader, "Wrote mesh: " + meshData.name);
#endif
    }
    file.close();
    std::cout << "Exported: " << outputPath << "\n";
}

ModelHeader File::ReadModelHeader(const std::string& filepath) {
    ModelHeader header;
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to read model header: " << filepath << "\n";
        return header;
    }
    file.read((char*)&header.version, sizeof(header.version));
    file.read((char*)&header.meshCount, sizeof(header.meshCount));
    file.read((char*)&header.nameLength, sizeof(header.nameLength));
    std::string modelName(header.nameLength, '\0');
    file.read(&modelName[0], header.nameLength);
    file.read(reinterpret_cast<char*>(&header.timestamp), sizeof(header.timestamp));
    return header;
}

ModelData File::ImportModel(const std::string& filepath) {
    ModelData modelData;
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for reading: " << filepath << "\n";
        return modelData;
    }
    ModelHeader modelHeader;
    file.read((char*)&modelHeader.version, sizeof(modelHeader.version));
    file.read((char*)&modelHeader.meshCount, sizeof(modelHeader.meshCount));
    file.read((char*)&modelHeader.nameLength, sizeof(modelHeader.nameLength));
    std::string modelName(modelHeader.nameLength, '\0');
    file.read(&modelName[0], modelHeader.nameLength);
    file.read(reinterpret_cast<char*>(&modelHeader.timestamp), sizeof(modelHeader.timestamp));
    file.read(reinterpret_cast<char*>(&modelHeader.aabbMin), sizeof(glm::vec3));
    file.read(reinterpret_cast<char*>(&modelHeader.aabbMax), sizeof(glm::vec3));
    modelData.meshCount = modelHeader.meshCount;
    modelData.meshes.resize(modelHeader.meshCount);
    modelData.name = modelName;
    modelData.aabbMin = modelHeader.aabbMin;
    modelData.aabbMax = modelHeader.aabbMax;
    modelData.timestamp = modelHeader.timestamp;
#if PRINT_MODEL_HEADERS_ON_READ
    PrintModelHeader(modelHeader, "Read model header: " + modelData.name);
#endif
    for (uint32_t i = 0; i < modelHeader.meshCount; ++i) {
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
#if PRINT_MESH_HEADERS_ON_READ
        PrintMeshHeader(meshHeader, "Read mesh: " + meshData.name);
#endif
    }
    file.close();
    return modelData;
}

/*
 ▀█▀   █ █▀█
  █  ▄▀  █ █
 ▀▀▀ ▀   ▀▀▀ */

void File::DeleteFile(const std::string& filepath) {
    try {
        if (std::filesystem::remove(filepath)) {
            // File deleted successfully
        }
        else {
            std::cout << "File::DeleteFile() failed to delete '" << filepath << "', file does not exist or could not be deleted!\n";
        }
    }
    catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "File::DeleteFile() exception: " << e.what() << "\n";
    }
}

/*
 ▀▀█▀▀ █ █▀▄▀█ █▀▀ █▀▀ ▀█▀ █▀▀█ █▀▄▀█ █▀▀█ █▀▀
   █   █ █ ▀ █ █▀▀ ▀▀█  █  █▄▄█ █ ▀ █ █▄▄█ ▀▀█
   ▀   ▀ ▀   ▀ ▀▀▀ ▀▀▀  ▀  ▀  ▀ ▀   ▀ ▀    ▀▀▀ */

uint64_t File::GetCurrentTimestamp() {
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

std::string File::TimestampToString(uint64_t timestamp) {
    std::time_t time = static_cast<std::time_t>(timestamp);
    std::tm timeStruct{};
    if (localtime_s(&timeStruct, &time) != 0) {
        return "Invalid Timestamp";
    }
    char buffer[26];
    if (std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeStruct)) {
        return std::string(buffer);
    }
    return "Invalid Timestamp";
}

uint64_t File::GetLastModifiedTime(const std::string& filePath) {
    try {
        auto ftime = std::filesystem::last_write_time(filePath);
        auto systemTime = std::chrono::clock_cast<std::chrono::system_clock>(ftime);
        return std::chrono::duration_cast<std::chrono::seconds>(systemTime.time_since_epoch()).count();
    }
    catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error reading file timestamp: " << e.what() << "\n";
        return 0;
    }
}

/*
█▀▄ █▀▀ █▀▄ █ █ █▀▀
█ █ █▀▀ █▀▄ █ █ █ █
▀▀  ▀▀▀ ▀▀  ▀▀▀ ▀▀▀ */

void File::PrintModelHeader(ModelHeader header, const std::string& identifier) {
    std::cout << identifier << "\n";
    std::cout << " Version: " << header.version << "\n";
    std::cout << " Mesh Count: " << header.meshCount << "\n";
    std::cout << " Name Length: " << header.nameLength << "\n";
    std::cout << " Timestamp: " << header.timestamp << "\n\n";
    std::cout << " AABB min: " << Util::Vec3ToString(header.aabbMin) << "\n";
    std::cout << " AABB max: " << Util::Vec3ToString(header.aabbMax) << "\n\n";
}

void File::PrintMeshHeader(MeshHeader header, const std::string& identifier) {
    std::cout << identifier << "\n";
    std::cout << " Name Length: " << header.nameLength << "\n";
    std::cout << " Vertex Count: " << header.vertexCount << "\n";
    std::cout << " Index Count: " << header.indexCount << "\n";
    std::cout << " AABB min: " << Util::Vec3ToString(header.aabbMin) << "\n";
    std::cout << " AABB max: " << Util::Vec3ToString(header.aabbMax) << "\n\n";
}

void File::SaveMeshDataToOBJ(const std::string& filepath, const MeshData& mesh) {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for writing: " << filepath << "\n";
        return;
    }
    for (const auto& vertex : mesh.vertices) {
        file << "v " << vertex.position.x << " " << vertex.position.y << " " << vertex.position.z << "\n";
    }
    for (const auto& vertex : mesh.vertices) {
        file << "vn " << vertex.normal.x << " " << vertex.normal.y << " " << vertex.normal.z << "\n";
    }
    for (const auto& vertex : mesh.vertices) {
        file << "vt " << vertex.uv.x << " " << vertex.uv.y << "\n";
    }
    for (size_t i = 0; i < mesh.indices.size(); i += 3) {
        file << "f "
            << mesh.indices[i] + 1 << "/" << mesh.indices[i] + 1 << "/" << mesh.indices[i] + 1 << " "
            << mesh.indices[i + 1] + 1 << "/" << mesh.indices[i + 1] + 1 << "/" << mesh.indices[i + 1] + 1 << " "
            << mesh.indices[i + 2] + 1 << "/" << mesh.indices[i + 2] + 1 << "/" << mesh.indices[i + 2] + 1 << "\n";
    }
    file.close();
    std::cout << "Exported OBJ: " << filepath << "\n";
}