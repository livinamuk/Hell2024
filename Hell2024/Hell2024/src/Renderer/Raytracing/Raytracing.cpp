#include "Raytracing.h"
//#include "3d_backend.h"
#include <vector>

#include <iostream>

namespace Raytracing {

    std::vector<BLAS> _bottomLevelAccelerationStructures;
    std::vector<TLAS> _topLevelAccelerationStructures;

    std::vector<BVHNode> _blasNodes;
    std::vector<unsigned int> _triangleIndices;

    int _nextBlasRootIndex = 0;
    int _nextBlasBaseTriangleIndex = 0;

    TLAS* CreateTopLevelAccelerationStruture() {
        return &_topLevelAccelerationStructures.emplace_back();
    }

    const void DestroyTopLevelAccelerationStructure(int index) {
        if (index >= 0 && index < _topLevelAccelerationStructures.size()) {
            _topLevelAccelerationStructures.erase(_topLevelAccelerationStructures.begin() + index);
        }
    }

    TLAS* GetTLASByIndex(int index) {
        if (index >= 0 && index < _topLevelAccelerationStructures.size()) {
            return &_topLevelAccelerationStructures[index];
        }
        else {
            return nullptr;
        }
    }

    const int CreateBLAS(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices, unsigned int meshBaseVertex, unsigned int meshBaseIndex) {

        BLAS& blas = _bottomLevelAccelerationStructures.emplace_back();
        blas.Create(vertices, indices, meshBaseVertex, meshBaseIndex);
        blas.rootIndex = _nextBlasRootIndex;
        blas.baseTriangleIndex = _nextBlasBaseTriangleIndex;

        // Add indices to global buffer
        _triangleIndices.reserve(_triangleIndices.size() + blas.triIndices.size());
        _triangleIndices.insert(_triangleIndices.end(), blas.triIndices.begin(), blas.triIndices.end());

        // Add bvh nodes to global buffer
        _blasNodes.insert(_blasNodes.end(), blas.bvhNodes.begin(), blas.bvhNodes.begin() + blas.nodesUsed);

        // Increment global counters
        _nextBlasRootIndex = (int)_blasNodes.size();
        _nextBlasBaseTriangleIndex = (int)_triangleIndices.size();

        // Return an index to this newly created BLAS
        return (int)_bottomLevelAccelerationStructures.size() - 1;
    }

    const void RecreateBLAS(unsigned int blasIndex, std::vector<Vertex>& vertices, std::vector<unsigned int>& indices, unsigned int meshBaseVertex, unsigned int meshBaseIndex) {

        BLAS& blas = _bottomLevelAccelerationStructures[blasIndex];

        // Remove triangles and BVH nodes from global buffers
        _triangleIndices.erase(_triangleIndices.begin() + blas.baseTriangleIndex, _triangleIndices.begin() + blas.baseTriangleIndex + blas.triIndices.size());
        _blasNodes.erase(_blasNodes.begin() + blas.rootIndex, _blasNodes.begin() + blas.rootIndex + blas.nodesUsed);

        // Build the new BVH, replacing the old one
        blas.Create(vertices, indices, meshBaseVertex, meshBaseIndex);

        // Add indices to global buffer
        _triangleIndices.reserve(_triangleIndices.size() + blas.triIndices.size());
        _triangleIndices.insert(_triangleIndices.begin() + blas.baseTriangleIndex, blas.triIndices.begin(), blas.triIndices.end());

        // Add bvh nodes to global buffer
        _blasNodes.reserve(_blasNodes.size() + blas.nodesUsed);
        _blasNodes.insert(_blasNodes.begin() + blas.rootIndex, blas.bvhNodes.begin(), blas.bvhNodes.begin() + blas.nodesUsed);

        // Increment global counters
        _nextBlasRootIndex = (int)_blasNodes.size();
        _nextBlasBaseTriangleIndex = (int)_triangleIndices.size();
    }

    BLAS* GetBLASByIndex(int index) {
        if (index >= 0 && index < _bottomLevelAccelerationStructures.size()) {
            return &_bottomLevelAccelerationStructures[index];
        }
        else {
            return nullptr;
        }
    }

    const std::vector<BVHNode>& GetBLSANodes() {
        return _blasNodes;
    }

    const std::vector<unsigned int>& GetTriangleIndices() {
        return _triangleIndices;
    }

    const std::vector<BLASInstance> GetBLASInstances(int tlasIndex) {

        std::vector<BLASInstance> blasInstnaces;

        TLAS* tlas = GetTLASByIndex(tlasIndex);
        if (tlas) {
            for (auto& index : tlas->GetSortedBLASInstanceIndices()) {
                BLAS* blas = GetBLASByIndex(tlas->GetInstanceBLASIndexByInstanceIndex(index));
                if (blas) {
                    BLASInstance& blasInstance = blasInstnaces.emplace_back();
                    blasInstance.worldTransform = tlas->GetInstanceWorldTransformByInstanceIndex(index);
                    blasInstance.blsaRootNodeIndex = blas->rootIndex;
                    blasInstance.baseTriangleIndex = blas->baseTriangleIndex;
                    blasInstance.baseVertex = blas->meshbaseVertex;
                    blasInstance.baseIndex = blas->meshbaseIndex;
                }
            }
        }
        else {
            //std::cout << "GetBLASInstances() returned empty vector because tlasIndex is out of range\n";
        }
        return blasInstnaces;
    }

    /*AABB CalculateMeshWorldSpaceAABB(Mesh& mesh, glm::mat4 worldMatrix) {

        glm::vec3 boundsMin = glm::vec3(1e30f);
        glm::vec3 boundsMax = glm::vec3(-1e30f);

        for (uint32_t i = mesh.baseIndex; i < mesh.baseIndex + mesh.indexCount; i += 3) {

            Vertex* v0 = &BackEnd::getVertices()[BackEnd::getIndices()[i + 0] + mesh.baseVertex];
            Vertex* v1 = &BackEnd::getVertices()[BackEnd::getIndices()[i + 1] + mesh.baseVertex];
            Vertex* v2 = &BackEnd::getVertices()[BackEnd::getIndices()[i + 2] + mesh.baseVertex];

            RdVector4F pos0 = worldMatrix * RdVector4F(v0->GetPosition(), 1.0);
            RdVector4F pos1 = worldMatrix * RdVector4F(v1->GetPosition(), 1.0);
            RdVector4F pos2 = worldMatrix * RdVector4F(v2->GetPosition(), 1.0);

            boundsMin.x() = std::min(pos0.x(), boundsMin.x());
            boundsMin.x() = std::min(pos1.x(), boundsMin.x());
            boundsMin.x() = std::min(pos2.x(), boundsMin.x());
            boundsMin.y() = std::min(pos0.y(), boundsMin.y());
            boundsMin.y() = std::min(pos1.y(), boundsMin.y());
            boundsMin.y() = std::min(pos2.y(), boundsMin.y());
            boundsMin.z() = std::min(pos0.z(), boundsMin.z());
            boundsMin.z() = std::min(pos1.z(), boundsMin.z());
            boundsMin.z() = std::min(pos2.z(), boundsMin.z());

            boundsMax.x() = std::max(pos0.x(), boundsMax.x());
            boundsMax.x() = std::max(pos1.x(), boundsMax.x());
            boundsMax.x() = std::max(pos2.x(), boundsMax.x());
            boundsMax.y() = std::max(pos0.y(), boundsMax.y());
            boundsMax.y() = std::max(pos1.y(), boundsMax.y());
            boundsMax.y() = std::max(pos2.y(), boundsMax.y());
            boundsMax.z() = std::max(pos0.z(), boundsMax.z());
            boundsMax.z() = std::max(pos1.z(), boundsMax.z());
            boundsMax.z() = std::max(pos2.z(), boundsMax.z());
        }
        return AABB(boundsMin, boundsMax);
    }*/

    void CleanUp() {
        _bottomLevelAccelerationStructures.clear();
        _topLevelAccelerationStructures.clear();
        _blasNodes.clear();
        _triangleIndices.clear();
        _nextBlasRootIndex = 0;
        _nextBlasBaseTriangleIndex = 0;
    }
}
