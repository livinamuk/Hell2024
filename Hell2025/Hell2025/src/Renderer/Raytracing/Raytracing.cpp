#include "Raytracing.h"
#include "../../BackEnd/BackEnd.h"
#include "../../Core/AssetManager.h"
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

    struct Ray {
        glm::vec3 origin;
        glm::vec3 dir;
        glm::vec3 inverseDir;
        float t;
    };

    float DistanceSquared(glm::vec3 A, glm::vec3 B) {
        glm::vec3 C = A - B;
        return glm::dot(C, C);
    }

    float SafeInverse(float x) {
        const float epsilon = 0.001;
        if (abs(x) <= epsilon) {
            if (x >= 0) return 1.f / epsilon;
            return -1.f / epsilon;
        }
        return 1.f / x;
    }

    bool IntersectAABB(Ray ray, glm::vec3 bmin, glm::vec3 bmax) {
        float tx1 = (bmin.x - ray.origin.x) * ray.inverseDir.x, tx2 = (bmax.x - ray.origin.x) * ray.inverseDir.x;
        float tmin = std::min(tx1, tx2), tmax = std::max(tx1, tx2);
        float ty1 = (bmin.y - ray.origin.y) * ray.inverseDir.y, ty2 = (bmax.y - ray.origin.y) * ray.inverseDir.y;
        tmin = std::max(tmin, std::min(ty1, ty2)), tmax = std::min(tmax, std::max(ty1, ty2));
        float tz1 = (bmin.z - ray.origin.z) * ray.inverseDir.z, tz2 = (bmax.z - ray.origin.z) * ray.inverseDir.z;
        tmin = std::max(tmin, std::min(tz1, tz2)), tmax = std::min(tmax, std::max(tz1, tz2));
        return tmax >= tmin && tmin < ray.t && tmax > 0;
    }

    bool TriIntersectBoolVersion(glm::vec3& origin, glm::vec3& direction, float rayMin, float rayMax, glm::vec3& v0, glm::vec3& v1, glm::vec3& v2) {

        glm::vec3 a = v0 - v1;
        glm::vec3 b = v2 - v0;
        glm::vec3 p = v0 - origin;
        glm::vec3 n = cross(b, a);
        float r = dot(direction, n);

        //ignore back face
        //if (r > 0)
        //	return false;

        // some other early out
        //if (abs(r) < 0.00001)
        //	return false;

        glm::vec3 q = glm::cross(p, direction);
        r = 1.0 / r;
        float u = glm::dot(q, b) * r;
        float v = glm::dot(q, a) * r;
        float t = glm::dot(n, p) * r;

        if (u < 0.0 || v < 0.0 || (u + v)>1.0)
            t = -1.0;

        if (t > rayMin && t < rayMax) {
            return true;
        }
        return false;
    }


    bool IntersectBLAS(Ray ray, const unsigned int nodeIdx, const unsigned int blasIndex, const std::vector<BVHNode>& blasNodes, std::vector<BLASInstance>& blasInstaces, const std::vector<unsigned int>& triangleIndices) {

        const glm::mat4 inverseModelMatrix = blasInstaces[blasIndex].inverseModelMatrix;
        const unsigned int rootIndex = blasInstaces[blasIndex].blsaRootNodeIndex;
        const unsigned int baseTriangleIndex = blasInstaces[blasIndex].baseTriangleIndex;
        const unsigned int baseVertex = blasInstaces[blasIndex].baseVertex;
        const unsigned int baseIndex = blasInstaces[blasIndex].baseIndex;

        auto& vertices = AssetManager::GetVertices();
        auto& indices = AssetManager::GetIndices();

        Ray adjustedRay;
        adjustedRay.dir = glm::normalize(inverseModelMatrix * glm::vec4(ray.dir, 0.0));
        adjustedRay.origin = (inverseModelMatrix * glm::vec4(ray.origin, 1.0));
        adjustedRay.inverseDir.x = SafeInverse(adjustedRay.dir.x);
        adjustedRay.inverseDir.y = SafeInverse(adjustedRay.dir.y);
        adjustedRay.inverseDir.z = SafeInverse(adjustedRay.dir.z);
        adjustedRay.t = ray.t;

        // Create the stack
        unsigned int stack[16];
        unsigned int stackIndex = 0;

        // Push root node onto the stack
        stack[stackIndex] = rootIndex;
        stackIndex++;

        // Iterate the stack while it still contains nodes
        while (stackIndex > 0) {

            // Pop current node
            stackIndex--;

            unsigned int currentNodeIndex = stack[stackIndex];
            BVHNode childNode = blasNodes[currentNodeIndex];

            if (IntersectAABB(adjustedRay, childNode.aabbMin, childNode.aabbMax)) {


                std::cout << "blas aabb\n";

                // If intersection was a leaf then it has children triangles, so return red
                if (childNode.instanceCount > 0) {

                    for (int j = 0; j < childNode.instanceCount; j++) {

                        const unsigned int result = triangleIndices[blasNodes[currentNodeIndex].leftFirst + j + baseTriangleIndex];

                        unsigned int idx0 = indices[result * 3 + 0 + baseIndex] + baseVertex;
                        unsigned int idx1 = indices[result * 3 + 1 + baseIndex] + baseVertex;
                        unsigned int idx2 = indices[result * 3 + 2 + baseIndex] + baseVertex;

                        glm::vec3 v0 = glm::vec3(vertices[idx0].position[0], vertices[idx0].position[1], vertices[idx0].position[2]);
                        glm::vec3 v1 = glm::vec3(vertices[idx1].position[0], vertices[idx1].position[1], vertices[idx1].position[2]);
                        glm::vec3 v2 = glm::vec3(vertices[idx2].position[0], vertices[idx2].position[1], vertices[idx2].position[2]);

                        float rayMin = 0.0000;
                        float rayMax = adjustedRay.t;

                        bool triHit = TriIntersectBoolVersion(adjustedRay.origin, adjustedRay.dir, rayMin, rayMax, v0, v1, v2);

                        if (triHit) {
                            std::cout << "TRI HIT! \n";
                            return true;
                        }
                        else {

                            std::cout << "-but no tri hit\n";
                        }
                    }
                }
                else {
                    // If not a leaf, add children to stack
                    stack[stackIndex] = childNode.leftFirst + rootIndex;
                    stackIndex++;
                    stack[stackIndex] = childNode.leftFirst + 1 + rootIndex;
                    stackIndex++;
                }
            }

            if (stackIndex == 0) {
                false;
            }
        }
        // If you made it this far the BVH is broken
        return false;
    }

    bool IntersectTLAS(Ray ray, const unsigned int nodeIdx) {

        int tlasIndex = 0;
        TLAS* tlas = Raytracing::GetTLASByIndex(tlasIndex);
        if (!tlas) {
            std::cout << "Invalid TLAS!\n";
            return false;
        }

        std::vector<BVHNode>& tlasNodes = tlas->GetNodes();
        const std::vector<BVHNode>& blasNodes = Raytracing::GetBLSANodes();
        std::vector<BLASInstance> blasInstaces = Raytracing::GetBLASInstances(tlasIndex);
        const std::vector<unsigned int>& triangleIndices = Raytracing::GetTriangleIndices();

        BVHNode rootNode = tlasNodes[nodeIdx];


        // Early out if ray doesn't even hit the root node
        if (!IntersectAABB(ray, rootNode.aabbMin, rootNode.aabbMax)) {
            std::cout << "Ray didn't even hit the root node!\n";
            return false;
        }

        // Create the stack
        unsigned int stack[16];
        unsigned int stackIndex = 0;

        // Push root node to the stack nodes
        stack[stackIndex] = 0;
        stackIndex++;

        // Iterate the stack while it still contains nodes
        while (stackIndex > 0) {

            std::cout << "stackIndex: " << stackIndex << "\n";

            // Pop current node
            stackIndex--;

            unsigned int currentNodeIndex = stack[stackIndex];
            BVHNode childNode = tlasNodes[currentNodeIndex];

            if (IntersectAABB(ray, childNode.aabbMin, childNode.aabbMax)) {

                std::cout << "Ray hit on AABB: " << Util::Vec3ToString(childNode.aabbMin) << " " << Util::Vec3ToString(childNode.aabbMax) << "\n";

                // If intersection was a leaf then it has children
                if (childNode.instanceCount > 0) {

                    std::cout << "- was a leafnode..\n";
                    // Successful hit
                    unsigned int blasIndex = childNode.leftFirst;
                    if (IntersectBLAS(ray, 0, blasIndex, blasNodes, blasInstaces, triangleIndices)) {
                        return true;
                    }
                }
                // If not a leaf, add children to stack
                else {
                    std::cout << "-was NOT a leafnode, adding children\n";
                    stack[stackIndex] = childNode.leftFirst;
                    stackIndex++;
                    stack[stackIndex] = childNode.leftFirst + 1;
                    stackIndex++;
                }
            }
            else {
                std::cout << "Failed ray hit on AABB: " << Util::Vec3ToString(childNode.aabbMin) << " " << Util::Vec3ToString(childNode.aabbMax) << "\n";
            }
            if (stackIndex == 0) {
                std::cout << "TLAS stack index 0\n";
                return false;
            }
        }
        // If you made it this far the BVH is broken
        std::cout << "BVH broken probably\n";
        return false;
    }

    bool LineOfSight(glm::vec3 rayOrigin, glm::vec3 rayDirection, float rayLength) {

        Ray ray;
        ray.origin = rayOrigin;
        ray.dir = rayDirection;
        ray.inverseDir.x = SafeInverse(ray.dir.x);
        ray.inverseDir.y = SafeInverse(ray.dir.y);
        ray.inverseDir.z = SafeInverse(ray.dir.z);
        ray.t = rayLength;

        std::cout << "Ray origin: " << Util::Vec3ToString(rayOrigin) << "\n";
        std::cout << "Ray direction: " << Util::Vec3ToString(rayDirection) << "\n";
        std::cout << "Ray length: " << rayLength << "\n\n";

        return IntersectTLAS(ray, 0);
    }

}
