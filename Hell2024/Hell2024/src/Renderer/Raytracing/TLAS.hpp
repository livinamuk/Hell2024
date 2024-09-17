#pragma once
#include "HellCommon.h"
#include "../../Util.hpp"
#include <vector>
#include <algorithm>

struct TLAS {

private:
    std::vector<BVHNode> nodes;
    std::vector<AABB> instanceAABBs;
    std::vector<unsigned int> sortedBLASInstanceIndices; // These range from 0 to n instances
    std::vector<unsigned int> instanceBLASIndices;       // These are the actual indices into the gBottomLevelAccelerationStructures vector in raytracing.cpp for each BLAS instance
    std::vector<glm::mat4> instanceWorldTransforms;
    unsigned int nodesUsed = 0;

public:

    void AddInstance(glm::mat4& worldTransform, unsigned int blasIndex, AABB& aabb) {
        instanceWorldTransforms.push_back(worldTransform);
        instanceBLASIndices.push_back(blasIndex);
        sortedBLASInstanceIndices.push_back((unsigned int)sortedBLASInstanceIndices.size());
        instanceAABBs.push_back(aabb);
    }

    void BuildBVH() {

        if (!instanceAABBs.size()) {
            return;
        }


        unsigned int tlasCount = (unsigned int)instanceAABBs.size();
        nodes.clear();
        nodes.resize(tlasCount * 2);

        unsigned int rootTlasNodeIindex = 0;
        nodesUsed = 1;

        // assign all instances to root node
        BVHNode& root = nodes[rootTlasNodeIindex];
        root.leftFirst = 0;
        root.instanceCount = tlasCount;

        UpdateNodeBounds(rootTlasNodeIindex);
        SubdivideNode(rootTlasNodeIindex);

        nodes.resize(nodesUsed);
    }

    void UpdateNodeBounds(unsigned int nodeIndex) {
        BVHNode& node = nodes[nodeIndex];
        node.aabbMin = glm::vec3(1e30f);
        node.aabbMax = glm::vec3(-1e30f);
        for (int first = node.leftFirst, i = 0; i < node.instanceCount; i++) {
            unsigned int leafIndex = sortedBLASInstanceIndices[first + i];
            AABB& aabb = instanceAABBs[leafIndex];
            node.aabbMin = Util::Vec3Min(node.aabbMin, aabb.GetBoundsMin());
            node.aabbMax = Util::Vec3Max(node.aabbMax, aabb.GetBoundsMax());
        }
    }

    float EvaluateTLASSAH(BVHNode& node, int axis, float candidatePos) {

        AABB leftBox, rightBox;
        for (int i = 0; i < node.instanceCount; i++) {

            AABB& aabb = instanceAABBs[node.leftFirst + i];
            if (aabb.GetCenter()[axis] < candidatePos) {
                leftBox.Grow(aabb.GetCenter());
            }
            else {
                rightBox.Grow(aabb.GetCenter());
            }
        }
        float cost = leftBox.Area() + rightBox.Area();
        return cost;
    }

    struct sort_aabb_x {
        explicit sort_aabb_x(std::vector<AABB>& container_) : container(container_) {}
        bool operator ()(const unsigned int& indexA, const unsigned int& indexB) const {
            return container[indexA].GetCenter().x < container[indexB].GetCenter().x;
        }
        std::vector<AABB>& container;
    };
    struct sort_aabb_y {
        explicit sort_aabb_y(std::vector<AABB>& container_) : container(container_) {}
        bool operator ()(const unsigned int& indexA, const unsigned int& indexB) const {
            return container[indexA].GetCenter().y < container[indexB].GetCenter().y;
        }
        std::vector<AABB>& container;
    };

    struct sort_aabb_z {
        explicit sort_aabb_z(std::vector<AABB>& container_) : container(container_) {}
        bool operator ()(const unsigned int& indexA, const unsigned int& indexB) const {
            return container[indexA].GetCenter().z < container[indexB].GetCenter().z;
        }
        std::vector<AABB>& container;
    };


    void SubdivideNode(unsigned int nodeIndex) {

        BVHNode& node = nodes[nodeIndex];

        // Determine split axis based upon longest side
        glm::vec3 extent = node.aabbMax - node.aabbMin;
        int axis = 0;
        if (extent.y > extent.x) {
            axis = 1;
        }
        if (extent.z > extent[axis]) {
            axis = 2;
        }

        // Gather child TLAS indices
        std::vector<unsigned int> indices;
        for (int i = node.leftFirst; i < node.leftFirst + node.instanceCount; i++) {
            indices.push_back(sortedBLASInstanceIndices[i]);
        }

        // Sort the the based on either their x, y, or z value inside the array of AABBs
        if (axis == 0) {
            std::sort(indices.begin(), indices.end(), sort_aabb_x(instanceAABBs));
        }
        else if (axis == 1) {
            std::sort(indices.begin(), indices.end(), sort_aabb_y(instanceAABBs));
        }
        else {
            std::sort(indices.begin(), indices.end(), sort_aabb_z(instanceAABBs));
        }

        // Replace the original indices with the sorted indices
        int j = 0;
        for (int i = node.leftFirst; i < node.leftFirst + node.instanceCount; i++) {
            sortedBLASInstanceIndices[i] = indices[j];
            j++;
        }

        // End recursion if one or both child nodes will be empty
        int leftCount = (int)indices.size() / 2;
        int rightCount = (int)indices.size() - leftCount;
        if (leftCount == 0 || rightCount == 0) {
            return;
        }

        // Update child node AABBs
        int leftChildIdx = nodesUsed++;
        int rightChildIdx = nodesUsed++;
        nodes[leftChildIdx].leftFirst = node.leftFirst;
        nodes[leftChildIdx].instanceCount = leftCount;
        nodes[rightChildIdx].leftFirst = node.leftFirst + leftCount;
        nodes[rightChildIdx].instanceCount = rightCount;
        node.leftFirst = leftChildIdx;
        node.instanceCount = 0;
        UpdateNodeBounds(leftChildIdx);
        UpdateNodeBounds(rightChildIdx);

        // Keep recursively subdividing the instances until each node contains a single child
        SubdivideNode(leftChildIdx);
        SubdivideNode(rightChildIdx);
    }

    std::vector<BVHNode>& GetNodes() {
        return nodes;
    }

    std::vector<unsigned int>& GetSortedBLASInstanceIndices() {
        return sortedBLASInstanceIndices;
    }

    unsigned int GetNodeCount() {
        return nodesUsed;
    }

    unsigned int GetInstanceCount() {
        return (int)sortedBLASInstanceIndices.size();
    }

    glm::mat4 GetInstanceWorldTransformByInstanceIndex(int index) {
        if (index >= 0 && index < (int)GetInstanceCount()) {
            return instanceWorldTransforms[index];
        }
        else {
            return glm::mat4(1);
        }
    }

    unsigned int GetInstanceBLASIndexByInstanceIndex(int index) {
        if (index >= 0 && index < (int)GetInstanceCount()) {
            return instanceBLASIndices[index];
        }
        else {
            return 0;
        }
    }
};