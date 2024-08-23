#pragma once
#include "TLAS.hpp"
#include "BLAS.hpp"

namespace Raytracing {

    const int CreateBLAS(std::vector<CSGVertex>& vertices, std::vector<unsigned int>& indices, unsigned int meshBaseVertex, unsigned int meshBaseIndex);
    const void RecreateBLAS(unsigned int blasIndex, std::vector<CSGVertex>& vertices, std::vector<unsigned int>& indices, unsigned int meshBaseVertex, unsigned int meshBaseIndex);
    TLAS* CreateTopLevelAccelerationStruture();
    const void DestroyTopLevelAccelerationStructure(int index);
    const std::vector<BVHNode>& GetBLSANodes();
    const std::vector<unsigned int>& GetTriangleIndices();
    const std::vector<BLASInstance> GetBLASInstances(int tlasIndex);
    BLAS* GetBLASByIndex(int index);
    TLAS* GetTLASByIndex(int index);
    //AABB CalculateMeshWorldSpaceAABB(Mesh& mesh, glm::mat4 worldMatrix);
    void CleanUp();
    int GetBottomLevelAccelerationStructureCount();
    bool LineOfSight(glm::vec3 rayOrigin, glm::vec3 rayDirection, float rayLength);

}