#pragma once
#include "../../../Common.h"
#include "GL_mesh.h"
#include "../../../Renderer/RendererCommon.h"
#include "../../../Physics/Physics.h"

// BRO GET THIS OUT OF HEREEEEEEE
// BRO GET THIS OUT OF HEREEEEEEE
// BRO GET THIS OUT OF HEREEEEEEE
// BRO GET THIS OUT OF HEREEEEEEE
//#include "../Renderer/Vulkan/Types/VK_Mesh.h"
#include "../../../API/Vulkan/VK_AssetManager.h"
//#include "../Renderer/Vulkan/VK_Util.hpp"
// BRO GET THIS OUT OF HEREEEEEEE
// BRO GET THIS OUT OF HEREEEEEEE
// BRO GET THIS OUT OF HEREEEEEEE
// BRO GET THIS OUT OF HEREEEEEEE

struct OpenGLModel {

    void Load(std::string filepath, const bool bake_on_load = true);
    void Draw();
    void CreateTriangleMesh();
    void Bake();;
    bool IsBaked();

    std::vector<OpenGLMesh> _meshes;
    std::vector<Triangle> _triangles;;
    std::vector<std::string> _meshNames;
    PxTriangleMesh* _triangleMesh;
    std::string _name;
    std::string _filename;
    BoundingBox _boundingBox;

    static void CreateVulkanModel(const char* filepath, VulkanModel& vulkanModelOut);

private:
    bool _baked = false;
};