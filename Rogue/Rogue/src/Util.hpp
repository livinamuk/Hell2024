#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>
//#include <glm/glm.hpp>
#include "Common.h"
#include "glm/gtx/intersect.hpp"
#include <random>
#include <format>

namespace Util {

    /*inline std::string Vec3ToString(glm::vec3 vec) {
        std::string str = "(";
        str += vec.x;
        str += ", ";
        str += vec.y;
        str += ", ";
        str += vec.z;
        str += ")";
    }*/

    inline int RandomInt(int min, int max) {
        static std::random_device dev;
        static std::mt19937 rng(dev());
        std::uniform_int_distribution<std::mt19937::result_type> dist6(min, max);
        return dist6(rng);
    }

    inline std::string ReadTextFromFile(std::string path) {
        std::ifstream file(path);
        std::string str;
        std::string line;
        while (std::getline(file, line)) {
            str += line + "\n";
        }
        return str;
    }

    inline std::string Vec3ToString(glm::vec3 v) {
        return std::string("(" + std::format("{:.2f}", v.x) + ", " + std::format("{:.2f}", v.y) + ", " + std::format("{:.2f}", v.z) + ")");
    }


    inline std::string Mat4ToString(glm::mat4 m) {
        std::string result;
       /* result += std::format("{:.2f}", m[0][0]) + ", " + std::format("{:.2f}", m[1][0]) + ", " + std::format("{:.2f}", m[2][0]) + ", " + std::format("{:.2f}", m[3][0]) + "\n";
        result += std::format("{:.2f}", m[0][1]) + ", " + std::format("{:.2f}", m[1][1]) + ", " + std::format("{:.2f}", m[2][1]) + ", " + std::format("{:.2f}", m[3][1]) + "\n";
        result += std::format("{:.2f}", m[0][2]) + ", " + std::format("{:.2f}", m[1][2]) + ", " + std::format("{:.2f}", m[2][2]) + ", " + std::format("{:.2f}", m[3][2]) + "\n";
        result += std::format("{:.2f}", m[0][3]) + ", " + std::format("{:.2f}", m[1][3]) + ", " + std::format("{:.2f}", m[2][3]) + ", " + std::format("{:.2f}", m[3][3]);
       */ 
        result += std::format("{:.2f}", m[0][0]) + ", " + std::format("{:.2f}", m[1][0]) + ", " + std::format("{:.2f}", m[2][0]) + ", " + std::format("{:.2f}", m[3][0]) + "\n";
        result += std::format("{:.2f}", m[0][1]) + ", " + std::format("{:.2f}", m[1][1]) + ", " + std::format("{:.2f}", m[2][1]) + ", " + std::format("{:.2f}", m[3][1]) + "\n";
        result += std::format("{:.2f}", m[0][2]) + ", " + std::format("{:.2f}", m[1][2]) + ", " + std::format("{:.2f}", m[2][2]) + ", " + std::format("{:.2f}", m[3][2]) + "\n";
        result += std::format("{:.2f}", m[0][3]) + ", " + std::format("{:.2f}", m[1][3]) + ", " + std::format("{:.2f}", m[2][3]) + ", " + std::format("{:.2f}", m[3][3]);
        return result;
    }

    inline bool FileExists(const std::string& name) {
        struct stat buffer;
        return (stat(name.c_str(), &buffer) == 0);
    }

    inline glm::mat4 GetVoxelModelMatrix(VoxelFace& voxel, float voxelSize) {
        float x = voxel.x * voxelSize;
        float y = voxel.y * voxelSize;
        float z = voxel.z * voxelSize;
        Transform transform;
        transform.scale = glm::vec3(voxelSize);
        transform.position = glm::vec3(x, y, z);
        return transform.to_mat4();
    }

    inline glm::mat4 GetVoxelModelMatrix(int x, int y, int z, float voxelSize) {
        Transform transform;
        transform.scale = glm::vec3(voxelSize);
        transform.position = glm::vec3(x, y, z) * voxelSize;
        return transform.to_mat4();
    }


    inline float FInterpTo(float current, float target, float deltaTime, float interpSpeed) {
        // If no interp speed, jump to target value
        if (interpSpeed <= 0.f)
            return target;
        // Distance to reach
        const float Dist = target - current;
        // If distance is too small, just set the desired location
        if (Dist * Dist < SMALL_NUMBER)
            return target;
        // Delta Move, Clamp so we do not over shoot.
        const float DeltaMove = Dist * glm::clamp(deltaTime * interpSpeed, 0.0f, 1.0f);
        return current + DeltaMove;
    }

    inline glm::vec3 GetDirectLightAtAPoint(Light& light, glm::vec3 voxelFaceCenter, glm::vec3 voxelNormal, float _voxelSize) {
        glm::vec3 lightCenter = glm::vec3(light.x * _voxelSize, light.y * _voxelSize, light.z * _voxelSize);
        float dist = glm::distance(voxelFaceCenter, lightCenter);
        //float att = 1.0 / (1.0 + 0.1 * dist + 0.01 * dist * dist);
        float att = glm::smoothstep(light.radius, 0.0f, glm::length(lightCenter - voxelFaceCenter));
        glm::vec3 n = glm::normalize(voxelNormal);
        glm::vec3 l = glm::normalize(lightCenter - voxelFaceCenter);
        float ndotl = glm::clamp(glm::dot(n, l), 0.0f, 1.0f);
        return glm::vec3(light.color) * att * light.strength * ndotl;
    }

    inline IntersectionResult RayTriangleIntersectTest(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, glm::vec3 o, glm::vec3 n) {

        IntersectionResult result;
        result.found = glm::intersectRayTriangle(o, n, p1, p2, p3, result.baryPosition, result.distance);
        //result.distance *= -1;
        return result;
    }

    inline glm::vec3 NormalFromTriangle(glm::vec3 pos0, glm::vec3 pos1, glm::vec3 pos2) {
        return glm::normalize(glm::cross(pos1 - pos0, pos2 - pos0));
    }

    inline glm::vec3 NormalFromTriangle(Triangle triangle) {
        glm::vec3 pos0 = triangle.p1;
        glm::vec3 pos1 = triangle.p2;
        glm::vec3 pos2 = triangle.p3;
        return glm::normalize(glm::cross(pos1 - pos0, pos2 - pos0));
    }

    inline float GetMaxXPointOfTri(Triangle& tri) {
        return std::max(std::max(tri.p1.x, tri.p2.x), tri.p3.x);
    }
    inline float GetMaxYPointOfTri(Triangle& tri) {
        return std::max(std::max(tri.p1.y, tri.p2.y), tri.p3.y);
    }
    inline float GetMaxZPointOfTri(Triangle& tri) {
        return std::max(std::max(tri.p1.z, tri.p2.z), tri.p3.z);
    }
    inline float GetMinXPointOfTri(Triangle& tri) {
        return std::min(std::min(tri.p1.x, tri.p2.x), tri.p3.x);
    }
    inline float GetMinYPointOfTri(Triangle& tri) {
        return std::min(std::min(tri.p1.y, tri.p2.y), tri.p3.y);
    }
    inline float GetMinZPointOfTri(Triangle& tri) {
        return std::min(std::min(tri.p1.z, tri.p2.z), tri.p3.z);
    }

    namespace RayTracing {

        inline bool AnyHit(std::vector<Triangle>& triangles, glm::vec3 rayOrign, glm::vec3 rayDir, float minDist, float maxDist) {
            
            for (Triangle& tri : triangles) {

                // Skip back facing
                if (glm::dot(rayDir, tri.normal) < 0) {
                    continue;
                }
                // Cast ray
                IntersectionResult result = Util::RayTriangleIntersectTest(tri.p1, tri.p2, tri.p3, rayOrign, rayDir);
                if (result.found && result.distance < maxDist && result.distance > minDist) {
                    return true;
                }
            }
            // No hit found
            return false;
        }
    }

    inline float MapRange(float inValue, float minInRange, float maxInRange, float minOutRange, float maxOutRange) {
        float x = (inValue - minInRange) / (maxInRange - minInRange);
        return minOutRange + (maxOutRange - minOutRange) * x;
    }

    inline int MapRange(int inValue, int minInRange, int maxInRange, int minOutRange, int maxOutRange) {
        float x = (inValue - minInRange) / (float)(maxInRange - minInRange);
        return minOutRange + (maxOutRange - minOutRange) * x;
    }
}