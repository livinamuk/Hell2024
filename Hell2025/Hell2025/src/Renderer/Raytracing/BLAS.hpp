#pragma once
#include "HellCommon.h"
#include "../../Util.hpp"
#include <vector>
#include <algorithm>

#define BVH_BINS 8

struct Bin {
    AABB bounds;
    int triCount = 0;
};

struct BLAS {

    std::vector<BVHNode> bvhNodes;
    std::vector<Triangle> triangles;
    std::vector<unsigned int> triIndices;
    unsigned int nodesUsed = 0;
    unsigned int meshbaseVertex = 0;    // Base vertex into gVertices withing 3D_backend.cpp
    unsigned int meshbaseIndex = 0;     // Base vertex into gIndices withing 3D_backend.cpp
    int rootIndex = 0;                  // Into the global gBlasNodes array within Raytracing.cpp
    int baseTriangleIndex = 0;          // Into the global gTriIndices array within Raytracing.cpp

    void Create(std::vector<CSGVertex>& vertices, std::vector<unsigned int>& indices, int baseVertex, int baseIndex) {

        meshbaseVertex = baseVertex;
        meshbaseIndex = baseIndex;

        int triCount = (int)indices.size() / 3;

        if (indices.size() == 0) {
            std::cout << "ERROR: attempted to create BLAS from mesh with no vertices!\n";
            return;
        }

        // Clear previous data
        bvhNodes.clear();
        triangles.clear();
        triIndices.clear();

        // Allocate memory
        bvhNodes.resize(triCount * 2);
        triangles.resize(triCount);
        triIndices.resize(triCount);

        // Create triangle array
        int j = 0;
        for (int i = 0; i < indices.size(); i += 3) {

            triangles[j].v0 = vertices[indices[i + 0]].position;
            triangles[j].v1 = vertices[indices[i + 1]].position;
            triangles[j].v2 = vertices[indices[i + 2]].position;

            triangles[j].centoid = ((triangles[j].v0 + triangles[j].v1 + triangles[j].v2)) * 0.3333f;
            triangles[j].centoid += triangles[j].v0 * glm::vec3(0.0f);
            float minX = std::min(std::min(triangles[j].v0.x, triangles[j].v1.x), triangles[j].v2.x);
            float minY = std::min(std::min(triangles[j].v0.y, triangles[j].v1.y), triangles[j].v2.y);
            float minZ = std::min(std::min(triangles[j].v0.z, triangles[j].v1.z), triangles[j].v2.z);
            float maxX = std::max(std::max(triangles[j].v0.x, triangles[j].v1.x), triangles[j].v2.x);
            float maxY = std::max(std::max(triangles[j].v0.y, triangles[j].v1.y), triangles[j].v2.y);
            float maxZ = std::max(std::max(triangles[j].v0.z, triangles[j].v1.z), triangles[j].v2.z);
            triangles[j].aabbMin = glm::vec3(minX, minY, minZ);
            triangles[j].aabbMax = glm::vec3(maxX, maxY, maxZ);
            j++;
        }

        // Populate triangle index array
        for (int i = 0; i < triCount; i++) {
            triIndices[i] = i;
        }

        // Assign all triangles to root node
        unsigned int rootNodeIindex = 0;
        nodesUsed = 1;
        BVHNode& root = bvhNodes[rootNodeIindex];
        root.leftFirst = 0;
        root.instanceCount = triCount;

        UpdateNodeBounds(rootNodeIindex);
        Subdivide(rootNodeIindex);
        bvhNodes.shrink_to_fit();
    }

    float FindBestSplitPlane(BVHNode& node, int& axis, float& splitPos) {

        float bestCost = 1e30f;

        for (int a = 0; a < 3; a++) {

            float boundsMin = 1e30f, boundsMax = -1e30f;

            for (int i = 0; i < node.instanceCount; i++) {
                Triangle& triangle = triangles[triIndices[node.leftFirst + i]];
                boundsMin = std::min(boundsMin, triangle.centoid[a]);
                boundsMax = std::max(boundsMax, triangle.centoid[a]);
            }
            if (boundsMin == boundsMax) {
                continue;
            }
            // Populate the bins
            Bin bin[BVH_BINS];
            float scale = BVH_BINS / (boundsMax - boundsMin);

            for (int i = 0; i < node.instanceCount; i++) {

                Triangle& triangle = triangles[triIndices[node.leftFirst + i]];
                int binIdx = std::min(BVH_BINS - 1, (int)((triangle.centoid[a] - boundsMin) * scale));

                if (binIdx >= BVH_BINS || binIdx < 0) {
                    std::cout << "ERROR!\n";
                    std::cout << " binIdx: " << binIdx << "\n";
                    std::cout << " boundsMin: " << boundsMin << "\n";
                    std::cout << " boundsMin: " << boundsMax << "\n";
                    std::cout << " scale: " << scale << "\n";
                    std::cout << " node.instanceCount: " << node.instanceCount << "\n";
                    std::cout << " triangle.centoid: " << Util::Vec3ToString(triangle.centoid) << "\n";
                    std::cout << " triangle.v0: " << Util::Vec3ToString(triangle.v0) << "\n";
                    std::cout << " triangle.v1: " << Util::Vec3ToString(triangle.v1) << "\n";
                    std::cout << " triangle.v2: " << Util::Vec3ToString(triangle.v2) << "\n";
                }

                bin[binIdx].triCount++;
                bin[binIdx].bounds.Grow(triangle.v0);
                bin[binIdx].bounds.Grow(triangle.v1);
                bin[binIdx].bounds.Grow(triangle.v2);
            }

            // Gather data for the 7 planes between the 8 bins
            float leftArea[BVH_BINS - 1], rightArea[BVH_BINS - 1];
            int leftCount[BVH_BINS - 1], rightCount[BVH_BINS - 1];
            AABB leftBox, rightBox;
            int leftSum = 0, rightSum = 0;

            for (int i = 0; i < BVH_BINS - 1; i++) {
                leftSum += bin[i].triCount;
                leftCount[i] = leftSum;
                leftBox.Grow(bin[i].bounds);
                leftArea[i] = leftBox.Area();
                rightSum += bin[BVH_BINS - 1 - i].triCount;
                rightCount[BVH_BINS - 2 - i] = rightSum;
                rightBox.Grow(bin[BVH_BINS - 1 - i].bounds);
                rightArea[BVH_BINS - 2 - i] = rightBox.Area();
            }

            // Calculate SAH cost for the 7 planes
            scale = (boundsMax - boundsMin) / BVH_BINS;
            for (int i = 0; i < BVH_BINS - 1; i++) {
                float planeCost = leftCount[i] * leftArea[i] + rightCount[i] * rightArea[i];
                if (planeCost < bestCost) {
                    axis = a, splitPos = boundsMin + scale * (i + 1), bestCost = planeCost;
                }
            }
        }
        return bestCost;
    }

    float EvaluateSAH(BVHNode& node, int axis, float pos) {

        // Determine triangle counts and bounds for this split candidate
        AABB leftBox, rightBox;
        int leftCount = 0, rightCount = 0;
        for (int i = 0; i < node.instanceCount; i++)
        {
            Triangle& triangle = triangles[triIndices[node.leftFirst + i]];
            if (triangle.centoid[axis] < pos) {
                leftCount++;
                leftBox.Grow(triangle.v0);
                leftBox.Grow(triangle.v1);
                leftBox.Grow(triangle.v2);
            }
            else {
                rightCount++;
                rightBox.Grow(triangle.v0);
                rightBox.Grow(triangle.v1);
                rightBox.Grow(triangle.v2);
            }
        }
        float cost = leftCount * leftBox.Area() + rightCount * rightBox.Area();
        return cost > 0 ? cost : 1e30f;
    }

    void UpdateNodeBounds(unsigned int nodeIndex) {

        BVHNode& node = bvhNodes[nodeIndex];
        node.aabbMin = glm::vec3(1e30f);
        node.aabbMax = glm::vec3(-1e30f);
        for (int first = node.leftFirst, i = 0; i < node.instanceCount; i++) {
            unsigned int leafTriIdx = triIndices[first + i];
            Triangle& leafTri = triangles[leafTriIdx];
            node.aabbMin = Util::Vec3Min(node.aabbMin, leafTri.v0);
            node.aabbMin = Util::Vec3Min(node.aabbMin, leafTri.v1);
            node.aabbMin = Util::Vec3Min(node.aabbMin, leafTri.v2);
            node.aabbMax = Util::Vec3Max(node.aabbMax, leafTri.v0);
            node.aabbMax = Util::Vec3Max(node.aabbMax, leafTri.v1);
            node.aabbMax = Util::Vec3Max(node.aabbMax, leafTri.v2);
        }
    }

    float CalculateNodeCost(BVHNode& node) {

        glm::vec3 e = node.aabbMax - node.aabbMin; // extent of the node
        float surfaceArea = e.x * e.y + e.y * e.z + e.z * e.x;
        return node.instanceCount * surfaceArea;
    }

    void Subdivide(unsigned int nodeIdx) {

        BVHNode& node = bvhNodes[nodeIdx];

        // Terminate recursion if it contains a single triangle
        if (node.instanceCount == 1) {
            return;
        }

        // Determine split axis using SAH
        int axis = 0;
        float splitPos = 0;
        FindBestSplitPlane(node, axis, splitPos);

        //float splitCost = FindBestSplitPlane(node, axis, splitPos);
        //float nosplitCost = CalculateNodeCost(node);    // Need to profile this to see if it is actually quicker, I think not...
        //if (splitCost >= nosplitCost) {
        //    return;
        //}

        // in-place partition
        int i = node.leftFirst;
        int j = i + node.instanceCount - 1;
        while (i <= j) {
            if (triangles[triIndices[i]].centoid[axis] < splitPos) {
                i++;
            }
            else {
                std::swap(triIndices[i], triIndices[j--]);
            }
        }

        // Abort split if one of the sides is empty
        int leftCount = i - node.leftFirst;
        if (leftCount == 0 || leftCount == node.instanceCount) {
            return;
        }

        // Create child nodes
        int leftChildIdx = nodesUsed++;
        int rightChildIdx = nodesUsed++;
        bvhNodes[leftChildIdx].leftFirst = node.leftFirst;
        bvhNodes[leftChildIdx].instanceCount = leftCount;
        bvhNodes[rightChildIdx].leftFirst = i;
        bvhNodes[rightChildIdx].instanceCount = node.instanceCount - leftCount;
        node.leftFirst = leftChildIdx;
        node.instanceCount = 0;

        UpdateNodeBounds(leftChildIdx);
        UpdateNodeBounds(rightChildIdx);

        // Recursively split remaining triangles
        Subdivide(leftChildIdx);
        Subdivide(rightChildIdx);
    }
};