#pragma once
#include "../../../Common.h"
#include "GL_mesh.h"
#include "../../../Renderer/RendererCommon.h"
#include "../../../Physics/Physics.h"

struct OpenGLModel {

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

private:
    bool _baked = false;
};