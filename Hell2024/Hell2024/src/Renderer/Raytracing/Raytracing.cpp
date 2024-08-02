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

    const int CreateBLAS(std::vector<CSGVertex>& vertices, std::vector<unsigned int>& indices, unsigned int meshBaseVertex, unsigned int meshBaseIndex) {

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

    const void RecreateBLAS(unsigned int blasIndex, std::vector<CSGVertex>& vertices, std::vector<unsigned int>& indices, unsigned int meshBaseVertex, unsigned int meshBaseIndex) {

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
                    blasInstance.inverseModelMatrix = glm::inverse(tlas->GetInstanceWorldTransformByInstanceIndex(index));
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

    void CleanUp() {
        _bottomLevelAccelerationStructures.clear();
        _topLevelAccelerationStructures.clear();
        _blasNodes.clear();
        _triangleIndices.clear();
        _nextBlasRootIndex = 0;
        _nextBlasBaseTriangleIndex = 0;
    }

    int GetBottomLevelAccelerationStructureCount() {
        return _bottomLevelAccelerationStructures.size();
    }

}
