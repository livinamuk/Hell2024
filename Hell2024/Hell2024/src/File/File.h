#pragma once
#include "FileFormats.h"

#define MODEL_DIR "res/assets/models/"

namespace File {

    void Test();

    void ExportModel(const ModelData& modelData);
    ModelData ImportModel(const std::string& filepath);

    void PrintModelHeader(ModelHeader header);
    void PrintMeshHeader(MeshHeader header);
}