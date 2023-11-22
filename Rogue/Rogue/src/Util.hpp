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
#include <filesystem>
#include <assimp/matrix3x3.h>
#include <assimp/matrix4x4.h>

namespace Util {

    inline float DistanceSquared(glm::vec3 A, glm::vec3 B) {
        glm::vec3 C = A - B;
        return glm::dot(C, C);
    }

    inline glm::vec3 Translate(glm::mat4& translation, glm::vec3 position) {
        return translation * glm::vec4(position, 1.0);
    }

    inline float RandomFloat(float min, float max) {
        return min + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (max - min)));
    }

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
        glm::vec3 lightCenter = light.position;
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

    inline void AnyHit(glm::vec3 rayOrigin, glm::vec3 rayDirection, float maxDistance, std::vector<Triangle>& triangles, RayCastResult& out, bool ignoreBackFacing = true) {

        for (Triangle& triangle : triangles) {

            // Skip if triangle is facing away from light direction
            float vdotn = dot(rayDirection, triangle.normal);
            if (ignoreBackFacing && vdotn > 0) {
                continue;
            }

            IntersectionResult result = RayTriangleIntersectTest(triangle.p1, triangle.p2, triangle.p3, rayOrigin, rayDirection);
            float minDist = 0.01f;
            if (result.found && result.distance > minDist && result.distance < maxDistance) {
                if (result.distance < out.distanceToHit) {
                    out.triangle = triangle;
                    out.distanceToHit = result.distance;
                    out.baryPosition = result.baryPosition;
                    out.found = true;
                    out.rayCount++;
                    return;
                }
            }
            out.rayCount++;
        }
    }


    inline void EvaluateRaycasts(glm::vec3 rayOrigin, glm::vec3 rayDirection, float maxDistance, std::vector<Triangle>& triangles, RaycastObjectType parentType, glm::mat4 parentMatrix, RayCastResult& out, void* parent, bool ignoreBackFacing = true) {
        glm::mat4 inverseTransform = glm::inverse(parentMatrix);

        glm::vec3 origin = inverseTransform * glm::vec4(rayOrigin, 1);
        glm::vec3 direction = inverseTransform * glm::vec4(rayDirection, 0);

        for (Triangle& triangle : triangles) {

            // Skip if triangle is facing away from light direction
            float vdotn = dot(rayDirection, triangle.normal);
            if (ignoreBackFacing && vdotn > 0) {
                continue;
            }

            IntersectionResult result = RayTriangleIntersectTest(triangle.p1, triangle.p2, triangle.p3, origin, direction);
            float minDist = 0.01f;
            if (result.found && result.distance > minDist && result.distance < maxDistance) {
                if (result.distance < out.distanceToHit) {
                    out.triangle = triangle;
                    out.distanceToHit = result.distance;
                    out.raycastObjectType = parentType;
                    out.triangeleModelMatrix = parentMatrix;
                    out.baryPosition = result.baryPosition;
                    out.found = true;
                    out.parent = parent;
                }
            }
            out.rayCount++;
        }
    }

    inline FileInfo GetFileInfo(std::string filepath) {
        // isolate name
        std::string filename = filepath.substr(filepath.rfind("/") + 1);
        filename = filename.substr(0, filename.length() - 4);
        // isolate filetype
        std::string filetype = filepath.substr(filepath.length() - 3);
        // isolate direcetory
        std::string directory = filepath.substr(0, filepath.rfind("/") + 1);
        // material name
        std::string materialType = "NONE";
        if (filename.length() > 5) {
            std::string query = filename.substr(filename.length() - 3);
            if (query == "ALB" || query == "RMA" || query == "NRM")
                materialType = query;
        }
        // RETURN IT
        FileInfo info;
        info.fullpath = filepath;
        info.filename = filename;
        info.filetype = filetype;
        info.directory = directory;
        info.materialType = materialType;
        return info;
    }

    inline FileInfo GetFileInfo(const std::filesystem::directory_entry filepath)
    {
        std::stringstream ss;
        ss << filepath.path();
        std::string fullpath = ss.str();
        // remove quotes at beginning and end
        fullpath = fullpath.substr(1);
        fullpath = fullpath.substr(0, fullpath.length() - 1);
        // isolate name
        std::string filename = fullpath.substr(fullpath.rfind("/") + 1);
        filename = filename.substr(0, filename.length() - 4);
        // isolate filetype
        std::string filetype = fullpath.substr(fullpath.length() - 3);
        // isolate direcetory
        std::string directory = fullpath.substr(0, fullpath.rfind("/") + 1);
        // material name
        std::string materialType = "NONE";
        if (filename.length() > 5) {
            std::string query = filename.substr(filename.length() - 3);
            if (query == "ALB" || query == "RMA" || query == "NRM")
                materialType = query;
        }
        // RETURN IT
        FileInfo info;
        info.fullpath = fullpath;
        info.filename = filename;
        info.filetype = filetype;
        info.directory = directory;
        info.materialType = materialType;
        return info;
    }

    inline float YRotationBetweenTwoPoints(glm::vec3 a, glm::vec3 b) {
        float delta_x = b.x - a.x;
        float delta_y = b.z - a.z;
        float theta_radians = atan2(delta_y, delta_x);
        return -theta_radians;
    }

    inline glm::vec3 GetTranslationFromMatrix(glm::mat4 matrix) {
        return glm::vec3(matrix[3][0], matrix[3][1], matrix[3][2]);
    }

    inline void InterpolateQuaternion(glm::quat& Out, const glm::quat& Start, const glm::quat& End, float pFactor) {
        // calc cosine theta
        float cosom = Start.x * End.x + Start.y * End.y + Start.z * End.z + Start.w * End.w;
        // adjust signs (if necessary)
        glm::quat end = End;
        if (cosom < static_cast<float>(0.0)) {
            cosom = -cosom;
            end.x = -end.x;   // Reverse all signs
            end.y = -end.y;
            end.z = -end.z;
            end.w = -end.w;
        }
        // Calculate coefficients
        float sclp, sclq;
        if ((static_cast<float>(1.0) - cosom) > static_cast<float>(0.0001)) // 0.0001 -> some epsillon
        {
            // Standard case (slerp)
            float omega, sinom;
            omega = std::acos(cosom); // extract theta from dot product's cos theta
            sinom = std::sin(omega);
            sclp = std::sin((static_cast<float>(1.0) - pFactor) * omega) / sinom;
            sclq = std::sin(pFactor * omega) / sinom;
        }
        else {
            // Very close, do linear interp (because it's faster)
            sclp = static_cast<float>(1.0) - pFactor;
            sclq = pFactor;
        }
        Out.x = sclp * Start.x + sclq * end.x;
        Out.y = sclp * Start.y + sclq * end.y;
        Out.z = sclp * Start.z + sclq * end.z;
        Out.w = sclp * Start.w + sclq * end.w;
    }

    inline glm::mat4 Mat4InitScaleTransform(float ScaleX, float ScaleY, float ScaleZ) {
        /*	glm::mat4 m = glm::mat4(1);
            m[0][0] = ScaleX; m[0][1] = 0.0f;   m[0][2] = 0.0f;   m[0][3] = 0.0f;
            m[1][0] = 0.0f;   m[1][1] = ScaleY; m[1][2] = 0.0f;   m[1][3] = 0.0f;
            m[2][0] = 0.0f;   m[2][1] = 0.0f;   m[2][2] = ScaleZ; m[2][3] = 0.0f;
            m[3][0] = 0.0f;   m[3][1] = 0.0f;   m[3][2] = 0.0f;   m[3][3] = 1.0f;
            return m;*/

        return glm::scale(glm::mat4(1.0), glm::vec3(ScaleX, ScaleY, ScaleZ));
    }

    inline glm::mat4 Mat4InitRotateTransform(float RotateX, float RotateY, float RotateZ) {
        glm::mat4 rx = glm::mat4(1);
        glm::mat4 ry = glm::mat4(1);
        glm::mat4 rz = glm::mat4(1);

        const float x = ToRadian(RotateX);
        const float y = ToRadian(RotateY);
        const float z = ToRadian(RotateZ);

        rx[0][0] = 1.0f; rx[0][1] = 0.0f; rx[0][2] = 0.0f; rx[0][3] = 0.0f;
        rx[1][0] = 0.0f; rx[1][1] = cosf(x); rx[1][2] = -sinf(x); rx[1][3] = 0.0f;
        rx[2][0] = 0.0f; rx[2][1] = sinf(x); rx[2][2] = cosf(x); rx[2][3] = 0.0f;
        rx[3][0] = 0.0f; rx[3][1] = 0.0f; rx[3][2] = 0.0f; rx[3][3] = 1.0f;

        ry[0][0] = cosf(y); ry[0][1] = 0.0f; ry[0][2] = -sinf(y); ry[0][3] = 0.0f;
        ry[1][0] = 0.0f; ry[1][1] = 1.0f; ry[1][2] = 0.0f; ry[1][3] = 0.0f;
        ry[2][0] = sinf(y); ry[2][1] = 0.0f; ry[2][2] = cosf(y); ry[2][3] = 0.0f;
        ry[3][0] = 0.0f; ry[3][1] = 0.0f; ry[3][2] = 0.0f; ry[3][3] = 1.0f;

        rz[0][0] = cosf(z); rz[0][1] = -sinf(z); rz[0][2] = 0.0f; rz[0][3] = 0.0f;
        rz[1][0] = sinf(z); rz[1][1] = cosf(z); rz[1][2] = 0.0f; rz[1][3] = 0.0f;
        rz[2][0] = 0.0f; rz[2][1] = 0.0f; rz[2][2] = 1.0f; rz[2][3] = 0.0f;
        rz[3][0] = 0.0f; rz[3][1] = 0.0f; rz[3][2] = 0.0f; rz[3][3] = 1.0f;

        return rz * ry * rx;
    }

    inline glm::mat4 Mat4InitTranslationTransform(float x, float y, float z) {
        return  glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
    }

    inline glm::mat4 aiMatrix4x4ToGlm(const aiMatrix4x4& from) {
        glm::mat4 to;
        //the a,b,c,d in assimp is the row ; the 1,2,3,4 is the column
        to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
        to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
        to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
        to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
        return to;
    }

    inline glm::mat4 aiMatrix3x3ToGlm(const aiMatrix3x3& from) {
        glm::mat4 to;
        to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = 0.0;
        to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = 0.0;
        to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = 0.0;
        to[0][3] = 0.0; to[1][3] = 0.0; to[2][3] = 0.0; to[3][3] = 1.0;
        return to;
    }

    inline bool StrCmp(const char* queryA, const char* queryB) {
        if (strcmp(queryA, queryB) == 0)
            return true;
        else
            return false;
    }

    inline const char* CopyConstChar(const char* text) {
        char* b = new char[strlen(text) + 1] {};
        std::copy(text, text + strlen(text), b);
        return b;
    }

    inline bool LineIntersects(glm::vec2 begin_A, glm::vec2 end_A, glm::vec2 begin_B, glm::vec2 end_B, glm::vec2& result)
    {
        static const auto SameSign = [](float a, float b) -> bool {
            return ((a * b) >= 0);
        };
        // a
        float x1 = begin_A.x;
        float y1 = begin_A.y;
        float x2 = end_A.x;
        float y2 = end_A.y;
        // b
        float x3 = begin_B.x;
        float y3 = begin_B.y;
        float x4 = end_B.x;
        float y4 = end_B.y;
        float a1, a2, b1, b2, c1, c2;
        float r1, r2, r3, r4;
        float denom;
        a1 = y2 - y1;
        b1 = x1 - x2;
        c1 = (x2 * y1) - (x1 * y2);
        r3 = ((a1 * x3) + (b1 * y3) + c1);
        r4 = ((a1 * x4) + (b1 * y4) + c1);
        if ((r3 != 0) && (r4 != 0) && SameSign(r3, r4))
            return false;
        a2 = y4 - y3; // Compute a2, b2, c2
        b2 = x3 - x4;
        c2 = (x4 * y3) - (x3 * y4);
        r1 = (a2 * x1) + (b2 * y1) + c2; // Compute r1 and r2
        r2 = (a2 * x2) + (b2 * y2) + c2;
        if ((r1 != 0) && (r2 != 0) && (SameSign(r1, r2)))
            return false;
        denom = (a1 * b2) - (a2 * b1); //Line segments intersect: compute intersection point.
        if (denom == 0)
            return false;// COLLINEAR;
        // FIND THAT INTERSECTION POINT ALREADY
        {
            // Line AB represented as a1x + b1y = c1
            float a = y2 - y1;
            float b = x1 - x2;
            float c = a * (x1)+b * (y1);
            // Line CD represented as a2x + b2y = c2
            float a1 = y4 - y3;
            float b1 = x3 - x4;
            float c1 = a1 * (x3)+b1 * (y3);
            float det = a * b1 - a1 * b;
            if (det == 0) {

                return false;
            }
            else {
                float x = (b1 * c - b * c1) / det;
                float y = (a * c1 - a1 * c) / det;
                result.x = x;
                result.y = y;
                return true;
            }
            return false;
        }
    }

    inline bool LineIntersects(glm::vec3 begin_A, glm::vec3 end_A, glm::vec3 begin_B, glm::vec3 end_B, glm::vec3& result) {
        // wow this is ugly
        glm::vec2 temp;
        bool i = LineIntersects(glm::vec2(begin_A.x, begin_A.z), glm::vec2(end_A.x, end_A.z), glm::vec2(begin_B.x, begin_B.z), glm::vec2(end_B.x, end_B.z), temp);
        result.x = temp.x;
        result.y = begin_A.y;
        result.z = temp.y;
        return i;
    }

    inline float sign(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3) {
        return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
    }

    inline bool PointIn2DTriangle(glm::vec2 pt, glm::vec2 v1, glm::vec2 v2, glm::vec2 v3) {
        float d1, d2, d3;
        bool has_neg, has_pos;
        d1 = sign(pt, v1, v2);
        d2 = sign(pt, v2, v3);
        d3 = sign(pt, v3, v1);
        has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
        has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);
        return !(has_neg && has_pos);
    }
}

