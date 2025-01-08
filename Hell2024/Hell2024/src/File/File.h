#pragma once
#include "FileFormats.h"
#include <string>

namespace File {
    // Models
    void ExportModel(const ModelData& modelData);
    ModelData ImportModel(const std::string& filepath);
    ModelHeader ReadModelHeader(const std::string& filepath);

    // I/O
    void DeleteFile(const std::string& filePath);

    // Time
    uint64_t GetCurrentTimestamp();
    std::string TimestampToString(uint64_t timestamp);
    uint64_t GetLastModifiedTime(const std::string& filePath);

    // Debug
    void SaveMeshDataToOBJ(const std::string& filepath, const MeshData& mesh);
    void PrintModelHeader(ModelHeader header, const std::string& identifier);
    void PrintMeshHeader(MeshHeader header, const std::string& identifier);
}