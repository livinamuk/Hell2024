#pragma once
#include "HellCommon.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include "glm/gtx/intersect.hpp"
#include <random>
#include <format>
#include <filesystem>
#include <assimp/matrix3x3.h>
#include <assimp/matrix4x4.h>
#include "Physics/Physics.h"
#include "rapidjson/document.h"
#include <rapidjson/filereadstream.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/prettywriter.h>
#include "RendererCommon.h"

namespace Util {

    inline glm::dvec3 Rot3D(glm::dvec3 v, glm::dvec2 rot) {
        glm::vec2 c = cos(rot);
        glm::vec2 s = sin(rot);
        glm::dmat3 rm = glm::dmat3(c.x, c.x * s.y, s.x * c.y, 0.0, c.y, s.y, -c.x, s.y * c.x, c.y * c.x);
        return v * rm;
    }

  //  long MapRange(long x, long in_min, long in_max, long out_min, long out_max) {
   //     return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
   // }
    
    inline std::string EditorModeToString(EditorMode editormode) {
        if (editormode == EditorMode::MAP) {
            return "MAP";
        }
        else if (editormode == EditorMode::CHRISTMAS) {
            return "CHRISTMAS";
        }
        else if (editormode == EditorMode::SHARK_PATH) {
            return "SHARK_PATH";
        }
    };
     
    inline std::string MenuTypeToString(MenuType menuType) {
        if (menuType == MenuType::NONE) {
            return "NONE";
        }
        else if (menuType == MenuType::INSERT) {
            return "INSERT";
        }
        else if (menuType == MenuType::FILE) {
            return "FILE";
        }
        else if (menuType == MenuType::SELECTED_OBJECT) {
            return "SELECTED_OBJECT";
        }
        else if (menuType == MenuType::MISC) {
            return "MISC";
        }
        else {
            return "UNDEFINED";
        }
    }
    

    inline std::string DobermannStateToString(DobermannState& state) {
        if (state == DobermannState::LAY) {
            return "LAY";
        }
        else if (state == DobermannState::PATROL) {
            return "PATROL";
        }
        else if (state == DobermannState::KAMAKAZI) {
            return "KAMAKAZI";
        }
        else if (state == DobermannState::DOG_SHAPED_PIECE_OF_MEAT) {
            return "DOG_SHAPED_PIECE_OF_MEAT";
        }
        else if (state == DobermannState::RETURN_TO_ORIGIN) {
            return "RETURN_TO_ORIGIN";
        }
        else {
            return "UNDEFINED";
        }
    }

    inline std::string ObjectTypeToString(ObjectType& type) {

        if (type == ObjectType::GAME_OBJECT) {
            return "GAME_OBJECT";
        }
        else if (type == ObjectType::LADDER) {
            return "LADDER";
        }
        else if (type == ObjectType::GLASS) {
            return "GLASS";
        }
        else if (type == ObjectType::DOOR) {
            return "DOOR";
        }
        else if (type == ObjectType::WINDOW) {
            return "WINDOW";
        }
        else if (type == ObjectType::SCENE_MESH) {
            return "SCENE_MESH";
        }
        else if (type == ObjectType::RAGDOLL_RIGID) {
            return "RAGDOLL_RIGID";
        }
        else if (type == ObjectType::CSG_OBJECT_ADDITIVE_CUBE) {
            return "CSG_OBJECT_ADDITIVE_CUBE";
        }
        else if (type == ObjectType::CSG_OBJECT_ADDITIVE_WALL_PLANE) {
            return "CSG_OBJECT_ADDITIVE_WALL_PLANE";
        }
        else if (type == ObjectType::CSG_OBJECT_ADDITIVE_FLOOR_PLANE) {
            return "CSG_OBJECT_ADDITIVE_FLOOR_PLANE";
        }
        else if (type == ObjectType::CSG_OBJECT_ADDITIVE_CEILING_PLANE) {
            return "CSG_OBJECT_ADDITIVE_CEILING_PLANE";
        }
        else if (type == ObjectType::CSG_OBJECT_SUBTRACTIVE) {
            return "CSG_OBJECT_SUBTRACTIVE";
        }
        else if (type == ObjectType::HEIGHT_MAP) {
            return "HEIGHT_MAP";
        }
        else if (type == ObjectType::UNDEFINED) {
            return "UNDEFINED";
        }
        else  {
            return "U N K N O W N";
        }
    }

    inline glm::vec3 CalculateUniformScaleRelativeToCamera(glm::vec3 viewPos, glm::vec3 objectPos) {
        float magic = 10;
        float distance = glm::length(viewPos - objectPos);
        float scale = (distance) / (magic * std::tan(glm::radians(1.0f)));
        return glm::vec3(scale);
    }

    inline glm::ivec2 CalculateScreenSpaceCoordinates(const glm::vec3& worldPos, const glm::mat4& mvpMatrix, int screenWidth, int screenHeight, bool flipY = false) {
        glm::vec4 clipCoords = mvpMatrix * glm::vec4(worldPos, 1.0f);
        glm::vec3 ndcCoords = glm::vec3(clipCoords) / clipCoords.w;
        glm::ivec2 screenCoords;
        screenCoords.x = (ndcCoords.x + 1.0f) * 0.5f * screenWidth;
        if (flipY) {
            screenCoords.y = screenHeight - (1.0f - ndcCoords.y) * 0.5f * screenHeight;
        }
        else {
            screenCoords.y = (1.0f - ndcCoords.y) * 0.5f * screenHeight;
        }
        return screenCoords;
    }

    inline glm::vec3 Vec3Min(const glm::vec3& a, const glm::vec3& b) {
        return glm::vec3(std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z));
    }

    inline glm::vec3 Vec3Max(const glm::vec3& a, const glm::vec3& b) {
        return glm::vec3(std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z));
    }

    inline std::string Lowercase(std::string& str) {
        std::string result = "";
        for (auto& c : str) {
            result += std::tolower(c);
        }
        return result;
    }

    inline std::string Uppercase(std::string& str) {
        std::string result = "";
        for (auto& c : str) {
            result += std::toupper(c);
        }
        return result;
    }

    inline std::string DebugLineRenderModeToString(DebugLineRenderMode mode) {

        if (mode == DebugLineRenderMode::SHOW_NO_LINES) {
            return "NONE";
        }
        else if (mode == DebugLineRenderMode::PHYSX_ALL) {
            return "PHYSX_ALL";
        }
        else if (mode == DebugLineRenderMode::PHYSX_RAYCAST) {
            return "PHYSX_RAYCAST";
        }
        else if (mode == DebugLineRenderMode::PHYSX_COLLISION) {
            return "Collision";
        }
        else if (mode == DebugLineRenderMode::RAYTRACE_LAND) {
            return "RAYTRACE_LAND";
        }
        else if (mode == DebugLineRenderMode::PHYSX_EDITOR) {
            return "PHYSX_EDITOR";
        }
        else if (mode == DebugLineRenderMode::BOUNDING_BOXES) {
            return "BOUNDING_BOXES";
        }
        else if (mode == DebugLineRenderMode::RTX_LAND_AABBS) {
            return "RTX_LAND_AABBS";
        }
        else if (mode == DebugLineRenderMode::RTX_LAND_TRIS) {
            return "RTX_LAND_TRIS";
        }
        else if (mode == DebugLineRenderMode::RTX_LAND_TOP_LEVEL_ACCELERATION_STRUCTURE) {
            return "Top Level Acceleration Structure";
        }
        else if (mode == DebugLineRenderMode::RTX_LAND_BOTTOM_LEVEL_ACCELERATION_STRUCTURES) {
            return "Bottom Level Acceleration Structures";
        }
        else if (mode == DebugLineRenderMode::RTX_LAND_TOP_AND_BOTTOM_LEVEL_ACCELERATION_STRUCTURES) {
            return "RTX_LAND_TOP_AND_BOTTOM_LEVEL_ACCELERATION_STRUCTURES";
        }
        else if (mode == DebugLineRenderMode::PATHFINDING_RECAST) {
            return "Pathfinding Recast";
        }
        else {
            return "UNDEFINED";
        }
    }

    inline std::string SplitscreenModeToString(SplitscreenMode mode) {
        if (mode == SplitscreenMode::NONE) {
            return "NONE";
        }
        else if (mode == SplitscreenMode::TWO_PLAYER) {
            return "TWO_PLAYER";
        }
        else if (mode == SplitscreenMode::FOUR_PLAYER) {
            return "FOUR_PLAYER";
        }
        else {
            return "UNDEFINED";
        }
    }

    inline std::string RenderModeToString(RenderMode mode) {
        if (mode == RenderMode::COMPOSITE) {
            return "Direct + Indirect Light";
        }
        else if (mode == RenderMode::POINT_CLOUD) {
            return "Point Cloud";
        }
        else if (mode == RenderMode::DIRECT_LIGHT) {
            return "Direct Light";
        }
        else if (mode == RenderMode::COMPOSITE_PLUS_POINT_CLOUD) {
            return "Direct + Indirect Light + Point cloud";
        }
        else if (mode == RenderMode::TILE_HEATMAP) {
            return "Tile heat map";
        }
        else if (mode == RenderMode::LIGHTS_PER_PIXEL) {
            return "Lights Per Pixel";
        }
        else if (mode == RenderMode::LIGHTS_PER_TILE) {
            return "Lights Per Tile";
        }
        else {
            return "UNDEFINED";
        }
    }

    inline std::vector<Vertex> GetAABBVertices(AABB& aabb, glm::vec3 color) {
        std::vector<Vertex> vertices;
        glm::vec4 frontTopLeft = glm::vec4(aabb.boundsMin.x, aabb.boundsMax.y, aabb.boundsMax.z, 1.0f);
        glm::vec4 frontTopRight = glm::vec4(aabb.boundsMax.x, aabb.boundsMax.y, aabb.boundsMax.z, 1.0f);
        glm::vec4 frontBottomLeft = glm::vec4(aabb.boundsMin.x, aabb.boundsMin.y, aabb.boundsMax.z, 1.0f);
        glm::vec4 frontBottomRight = glm::vec4(aabb.boundsMax.x, aabb.boundsMin.y, aabb.boundsMax.z, 1.0f);
        glm::vec4 backTopLeft = glm::vec4(aabb.boundsMin.x, aabb.boundsMax.y, aabb.boundsMin.z, 1.0f);
        glm::vec4 backTopRight = glm::vec4(aabb.boundsMax.x, aabb.boundsMax.y, aabb.boundsMin.z, 1.0f);
        glm::vec4 backBottomLeft = glm::vec4(aabb.boundsMin.x, aabb.boundsMin.y, aabb.boundsMin.z, 1.0f);
        glm::vec4 backBottomRight = glm::vec4(aabb.boundsMax.x, aabb.boundsMin.y, aabb.boundsMin.z, 1.0f);
        vertices.push_back(Vertex(glm::vec3(frontBottomLeft), color));
        vertices.push_back(Vertex(glm::vec3(frontBottomRight), color));
        vertices.push_back(Vertex(glm::vec3(frontTopLeft), color));
        vertices.push_back(Vertex(glm::vec3(frontTopRight), color));
        vertices.push_back(Vertex(glm::vec3(frontBottomLeft), color));
        vertices.push_back(Vertex(glm::vec3(frontTopLeft), color));
        vertices.push_back(Vertex(glm::vec3(frontBottomRight), color));
        vertices.push_back(Vertex(glm::vec3(frontTopRight), color));
        vertices.push_back(Vertex(glm::vec3(backBottomLeft), color));
        vertices.push_back(Vertex(glm::vec3(backBottomRight), color));
        vertices.push_back(Vertex(glm::vec3(backTopLeft), color));
        vertices.push_back(Vertex(glm::vec3(backTopRight), color));
        vertices.push_back(Vertex(glm::vec3(backBottomLeft), color));
        vertices.push_back(Vertex(glm::vec3(backTopLeft), color));
        vertices.push_back(Vertex(glm::vec3(backBottomRight), color));
        vertices.push_back(Vertex(glm::vec3(backTopRight), color));
        vertices.push_back(Vertex(glm::vec3(frontBottomLeft), color));
        vertices.push_back(Vertex(glm::vec3(backBottomLeft), color));
        vertices.push_back(Vertex(glm::vec3(frontBottomRight), color));
        vertices.push_back(Vertex(glm::vec3(backBottomRight), color));
        vertices.push_back(Vertex(glm::vec3(frontTopLeft), color));
        vertices.push_back(Vertex(glm::vec3(backTopLeft), color));
        vertices.push_back(Vertex(glm::vec3(frontTopRight), color));
        vertices.push_back(Vertex(glm::vec3(backTopRight), color));
        return vertices;
    }

    inline float SquaredDistPointAABB(const glm::vec3 p, const AABB& aabb) {
        auto check = [&](
            const double pn,
            const double bmin,
            const double bmax) -> double {
            double out = 0;
            double v = pn;
            if (v < bmin) {
                double val = (bmin - v);
                out += val * val;
            }
            if (v > bmax) {
                double val = (v - bmax);
                out += val * val;
            }
            return out;
        };
        // Squared distance
        double sq = 0.0;
        sq += check(p.x, aabb.boundsMin.x, aabb.boundsMax.x);
        sq += check(p.y, aabb.boundsMin.y, aabb.boundsMax.y);
        sq += check(p.z, aabb.boundsMin.z, aabb.boundsMax.z);
        return (float)sq;
    }

    inline bool AABBInSphere(AABB& aabb, glm::vec3 spherePosition, float sphereRadius) {
        float squaredDistance = SquaredDistPointAABB(spherePosition, aabb);
        return squaredDistance <= (sphereRadius * sphereRadius);
    }

    inline std::string WeaponActionToString(WeaponAction weaponAction) {

        if (weaponAction == IDLE) {
            return "IDLE";
        }
        else if (weaponAction == FIRE) {
            return "FIRE";
        }
        else if (weaponAction == RELOAD) {
            return "RELOAD";
        }
        else if (weaponAction == RELOAD_FROM_EMPTY) {
            return "RELOAD_FROM_EMPTY";
        }
        else if (weaponAction == DRAW_BEGIN) {
            return "DRAW_BEGIN";
        }
        else if (weaponAction == DRAWING) {
            return "DRAWING";
        }
        else if (weaponAction == SPAWNING) {
            return "SPAWNING";
        }
        else if (weaponAction == RELOAD_SHOTGUN_BEGIN) {
            return "RELOAD_SHOTGUN_BEGIN";
        }
        else if (weaponAction == RELOAD_SHOTGUN_SINGLE_SHELL) {
            return "RELOAD_SHOTGUN_SINGLE_SHELL";
        }
        else if (weaponAction == RELOAD_SHOTGUN_DOUBLE_SHELL) {
            return "RELOAD_SHOTGUN_DOUBLE_SHELL";
        }
        else if (weaponAction == RELOAD_SHOTGUN_END) {
            return "RELOAD_SHOTGUN_END";
        }
        else if (weaponAction == ADS_IN) {
            return "ADS_IN";
        }
        else if (weaponAction == ADS_OUT) {
            return "ADS_OUT";
        }
        else if (weaponAction == ADS_IDLE) {
            return "ADS_IDLE";
        }
        else if (weaponAction == ADS_FIRE) {
            return "ADS_FIRE";
        }
        else {
            return "UNKNOWN WEAPON ACTION";
        }
    }



	inline glm::vec3 PxVec3toGlmVec3(PxVec3 vec) {
		return { vec.x, vec.y, vec.z };
	}

	inline glm::vec3 PxVec3toGlmVec3(PxExtendedVec3 vec) {
		return { vec.x, vec.y, vec.z };
	}

	inline PxVec3 GlmVec3toPxVec3(glm::vec3 vec) {
		return { vec.x, vec.y, vec.z };
	}

	inline PxQuat GlmQuatToPxQuat(glm::quat quat) {
		return { quat.x, quat.y, quat.z, quat.w };
	}

    inline glm::quat PxQuatToGlmQuat(PxQuat quat) {
        return { quat.x, quat.y, quat.z, quat.w };
    }

    //inline long MapRange(long x, long in_min, long in_max, long out_min, long out_max) {
    //    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    //}

    inline glm::vec3 GetMouseRay(glm::mat4 projection, glm::mat4 view, int windowWidth, int windowHeight, int mouseX, int mouseY) {
		float x = (2.0f * mouseX) / (float)windowWidth - 1.0f;
		float y = 1.0f - (2.0f * mouseY) / (float)windowHeight;
		float z = 1.0f;
		glm::vec3 ray_nds = glm::vec3(x, y, z);
		glm::vec4 ray_clip = glm::vec4(ray_nds.x, ray_nds.y, ray_nds.z, 1.0f);
		glm::vec4 ray_eye = glm::inverse(projection) * ray_clip;
		ray_eye = glm::vec4(ray_eye.x, ray_eye.y, ray_eye.z, 0.0f);
		glm::vec4 inv_ray_wor = (inverse(view) * ray_eye);
		glm::vec3 ray_wor = glm::vec3(inv_ray_wor.x, inv_ray_wor.y, inv_ray_wor.z);
		ray_wor = normalize(ray_wor);
		return ray_wor;
    }

    inline PhysXRayResult CastPhysXRay(glm::vec3 rayOrigin, glm::vec3 rayDirection, float rayLength, PxU32 collisionFlags, bool cullBackFacing = false) {

        PxScene* scene = Physics::GetScene();

        PxVec3 origin = PxVec3(rayOrigin.x, rayOrigin.y, rayOrigin.z);
        PxVec3 unitDir = PxVec3(rayDirection.x, rayDirection.y, rayDirection.z);
        PxReal maxDistance = rayLength;
        PxRaycastBuffer hit;
        // [in] Define what parts of PxRaycastHit we're interested in
     //   const PxHitFlags outputFlags = PxHitFlag::ePOSITION | PxHitFlag::eNORMAL | ~PxHitFlag::eMESH_BOTH_SIDES;

        PxHitFlags outputFlags = PxHitFlag::ePOSITION | PxHitFlag::eNORMAL | PxHitFlag::eMESH_BOTH_SIDES;
        if (cullBackFacing) {
            outputFlags = PxHitFlag::ePOSITION | PxHitFlag::eNORMAL;
        }

     // Only ray cast against objects with the GROUP_RAYCAST flag
        PxQueryFilterData filterData = PxQueryFilterData();
        filterData.data.word0 = collisionFlags;

        // Defaults
        PhysXRayResult result;
        result.hitObjectName = "NO_HIT";
        result.hitPosition = glm::vec3(0, 0, 0);
        result.surfaceNormal = glm::vec3(0, 0, 0);
        result.rayDirection = rayDirection;
        result.hitFound = false;
        result.hitActor = nullptr;
        result.parent = nullptr;

        // Cast the ray
        bool status = scene->raycast(origin, unitDir, maxDistance, hit, outputFlags, filterData);

        if (status) {
            if (hit.block.actor->getName()) {
                result.hitObjectName = hit.block.actor->getName();
            }
            else
                result.hitObjectName = "HIT OBJECT HAS NO ACTOR NAME";

            result.hitPosition = glm::vec3(hit.block.position.x, hit.block.position.y, hit.block.position.z);
            result.surfaceNormal = glm::vec3(hit.block.normal.x, hit.block.normal.y, hit.block.normal.z);
            result.hitFound = true;
            result.hitActor = hit.block.actor;

            if (hit.block.actor->userData) {
                PhysicsObjectData* physicsObjectData = (PhysicsObjectData*)hit.block.actor->userData;
                result.objectType = physicsObjectData->type;
                result.parent = physicsObjectData->parent;
            }
            else {
                result.objectType = ObjectType::UNDEFINED;
                result.hitFound = false;
                std::cout << "no user data found on ray hit\n";
            }

            /*EntityData* hitEntityData = (EntityData*)hit.block.actor->userData;
             m_parent = hitEntityData->parent;
             m_physicsObjectType = hitEntityData->type;*/

        }
        return result;
    }


    /*inline PxMat44 GlmMat4ToPxMat44(glm::mat4 matrix) {
        PxMat44 pxMatrix;
        for (int x = 0; x < 4; x++)
            for (int y = 0; y < 4; y++)
                pxMatrix[x][y] = matrix[x][y];
        return pxMatrix;
    }

    inline physx::PxMat44 TransformToPxMaQt44(Transform transform) {
        return GlmMat4ToPxMat44(transform.to_mat4());
    }*/

    inline void DrawFrontFacingPlane(int instanceCount) {
        static unsigned int frontFacingPlaneVAO = 0;
        float offset = 0.1f;
        if (frontFacingPlaneVAO == 0) {
            Vertex vert0, vert1, vert2, vert3;
            vert0.position = glm::vec3(-0.5, 0.5, offset);
            vert1.position = glm::vec3(0.5, 0.5f, offset);
            vert2.position = glm::vec3(0.5, -0.5, offset);
            vert3.position = glm::vec3(-0.5, -0.5, offset);
            vert0.uv = glm::vec2(0, 1);
            vert1.uv = glm::vec2(1, 1);
            vert2.uv = glm::vec2(1, 0);
            vert3.uv = glm::vec2(0, 0);
            vert0.normal = glm::vec3(0, 0, 1);
            vert1.normal = glm::vec3(0, 0, 1);
            vert2.normal = glm::vec3(0, 0, 1);
            vert3.normal = glm::vec3(0, 0, 1);
            vert0.tangent = glm::vec3(1, 0, 0);
            vert1.tangent = glm::vec3(1, 0, 0);
            vert2.tangent = glm::vec3(1, 0, 0);
            vert3.tangent = glm::vec3(1, 0, 0);
            std::vector<Vertex> vertices;
            std::vector<unsigned int> indices;
            unsigned int i = (unsigned int)vertices.size();
            indices.push_back(i + 2);
            indices.push_back(i + 1);
            indices.push_back(i + 0);
            indices.push_back(i + 0);
            indices.push_back(i + 3);
            indices.push_back(i + 2);
            vertices.push_back(vert0);
            vertices.push_back(vert1);
            vertices.push_back(vert2);
            vertices.push_back(vert3);
            unsigned int VBO;
            unsigned int EBO;
            glGenVertexArrays(1, &frontFacingPlaneVAO);
            glGenBuffers(1, &VBO);
            glGenBuffers(1, &EBO);
            glBindVertexArray(frontFacingPlaneVAO);
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
            glEnableVertexAttribArray(3);
            glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));
        }
        glBindVertexArray(frontFacingPlaneVAO);
        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, instanceCount);
    }

    inline glm::mat4 PxMat44ToGlmMat4(physx::PxMat44 pxMatrix) {
        glm::mat4 matrix;
        for (int x = 0; x < 4; x++)
            for (int y = 0; y < 4; y++)
                matrix[x][y] = pxMatrix[x][y];
        return matrix;
    }

    inline physx::PxMat44 GlmMat4ToPxMat44(glm::mat4 glmMatrix) {
        physx::PxMat44 matrix;
        for (int x = 0; x < 4; x++)
            for (int y = 0; y < 4; y++)
                matrix[x][y] = glmMatrix[x][y];
        return matrix;
    }

    inline glm::vec3 ClosestPointOnLine(glm::vec3 point, glm::vec3 start, glm::vec3 end) {
        glm::vec2 p(point.x, point.z);
        glm::vec2 v(start.x, start.z);
        glm::vec2 w(end.x, end.z);
        const float l2 = ((v.x - w.x) * (v.x - w.x)) + ((v.y - w.y) * (v.y - w.y));
        if (l2 == 0.0)
            return glm::vec3(0);
        const float t = std::max(0.0f, std::min(1.0f, dot(p - v, w - v) / l2));
        const glm::vec2 projection = v + t * (w - v);
        return glm::vec3(projection.x, 0, projection.y);
    }

    inline float DistanceSquared(glm::vec3 A, glm::vec3 B) {
        glm::vec3 C = A - B;
        return glm::dot(C, C);
    }

    inline glm::vec3 Translate(glm::mat4& translation, glm::vec3 position) {
        return glm::vec3(translation * glm::vec4(position, 1.0));
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

    inline std::string FloatToString(float value, int precision = 2) {
        return std::format("{:.{}f}", value, precision);
    }

    inline std::string BoolToString(bool value) {
        return value ? "True" : "False";
    }

    inline std::string Vec3ToString(glm::vec3 v) {
        return std::string("(" + std::format("{:.2f}", v.x) + ", " + std::format("{:.2f}", v.y) + ", " + std::format("{:.2f}", v.z) + ")");
    }
    inline std::string Vec3ToString10(glm::vec3 v) {
        return std::string("(" + std::format("{:.10f}", v.x) + ", " + std::format("{:.10f}", v.y) + ", " + std::format("{:.10f}", v.z) + ")");
    }

    inline std::string QuatToString(glm::quat q) {
        return std::string("(" + std::format("{:.2f}", q.x) + ", " + std::format("{:.2f}", q.y) + ", " + std::format("{:.2f}", q.z) + ", " + std::format("{:.2f}", q.w) + ")");
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

    inline std::string Mat4ToString10(glm::mat4 m) {
        std::string result;
        /* result += std::format("{:.2f}", m[0][0]) + ", " + std::format("{:.2f}", m[1][0]) + ", " + std::format("{:.2f}", m[2][0]) + ", " + std::format("{:.2f}", m[3][0]) + "\n";
         result += std::format("{:.2f}", m[0][1]) + ", " + std::format("{:.2f}", m[1][1]) + ", " + std::format("{:.2f}", m[2][1]) + ", " + std::format("{:.2f}", m[3][1]) + "\n";
         result += std::format("{:.2f}", m[0][2]) + ", " + std::format("{:.2f}", m[1][2]) + ", " + std::format("{:.2f}", m[2][2]) + ", " + std::format("{:.2f}", m[3][2]) + "\n";
         result += std::format("{:.2f}", m[0][3]) + ", " + std::format("{:.2f}", m[1][3]) + ", " + std::format("{:.2f}", m[2][3]) + ", " + std::format("{:.2f}", m[3][3]);
        */
        result += std::format("{:.10f}", m[0][0]) + ", " + std::format("{:.10f}", m[1][0]) + ", " + std::format("{:.10f}", m[2][0]) + ", " + std::format("{:.10f}", m[3][0]) + "\n";
        result += std::format("{:.10f}", m[0][1]) + ", " + std::format("{:.10f}", m[1][1]) + ", " + std::format("{:.10f}", m[2][1]) + ", " + std::format("{:.10f}", m[3][1]) + "\n";
        result += std::format("{:.10f}", m[0][2]) + ", " + std::format("{:.10f}", m[1][2]) + ", " + std::format("{:.10f}", m[2][2]) + ", " + std::format("{:.10f}", m[3][2]) + "\n";
        result += std::format("{:.10f}", m[0][3]) + ", " + std::format("{:.10f}", m[1][3]) + ", " + std::format("{:.10f}", m[2][3]) + ", " + std::format("{:.10f}", m[3][3]);
        return result;
    }

    inline bool FileExists(const std::string_view name) {
        struct stat buffer;
        return (stat(name.data(), &buffer) == 0);
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

    inline glm::vec3 FInterpTo(const glm::vec3& current, const glm::vec3& target, float deltaTime, float interpSpeed) {
        // If no interp speed, jump to target value
        if (interpSpeed <= 0.f)
            return target;
        // Distance to reach
        glm::vec3 Dist = target - current;
        // If distance is too small, just set the desired location
        if (glm::dot(Dist, Dist) < SMALL_NUMBER)
            return target;
        // Delta Move, Clamp so we do not overshoot
        glm::vec3 DeltaMove = Dist * glm::clamp(deltaTime * interpSpeed, 0.0f, 1.0f);
        return current + DeltaMove;
    }


    inline bool AreNormalsAligned(const glm::vec3& normal1, const glm::vec3& normal2, float threshold = 0.999f) {
        float dotProduct = glm::dot(normal1, normal2);
        return dotProduct * dotProduct > threshold * threshold;
    }

    inline glm::vec3 NormalFromTriangle(glm::vec3 pos0, glm::vec3 pos1, glm::vec3 pos2) {
        return glm::normalize(glm::cross(pos1 - pos0, pos2 - pos0));
    }

    inline glm::vec3 NormalFromTriangle(Triangle triangle) {
        glm::vec3 pos0 = triangle.v0;
        glm::vec3 pos1 = triangle.v1;
        glm::vec3 pos2 = triangle.v2;
        return glm::normalize(glm::cross(pos1 - pos0, pos2 - pos0));
    }

    inline void SetNormalsAndTangentsFromVertices(Vertex* vert0, Vertex* vert1, Vertex* vert2) {
        // Shortcuts for positions and UVs
        glm::vec3& v0 = vert0->position;
        glm::vec3& v1 = vert1->position;
        glm::vec3& v2 = vert2->position;

        glm::vec2& uv0 = vert0->uv;
        glm::vec2& uv1 = vert1->uv;
        glm::vec2& uv2 = vert2->uv;

        // Compute edges of the triangle
        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;

        glm::vec2 deltaUV1 = uv1 - uv0;
        glm::vec2 deltaUV2 = uv2 - uv0;

        // Compute the normal
        glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

        // Compute the tangent and bitangent
        float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
        glm::vec3 tangent = (edge1 * deltaUV2.y - edge2 * deltaUV1.y) * r;
        glm::vec3 bitangent = (edge2 * deltaUV1.x - edge1 * deltaUV2.x) * r;

        // Normalize tangent and bitangent
        tangent = glm::normalize(tangent);
        bitangent = glm::normalize(bitangent);

        // Assign the normal, tangent, and bitangent to the vertices
        //vert0->normal = normal;
        //vert1->normal = normal;
        //vert2->normal = normal;

        vert0->tangent = tangent;
        vert1->tangent = tangent;
        vert2->tangent = tangent;
    }

    inline void SetNormalsAndTangentsFromVertices(CSGVertex* vert0, CSGVertex* vert1, CSGVertex* vert2) {
        // Shortcuts for UVs
        glm::vec3& v0 = vert0->position;
        glm::vec3& v1 = vert1->position;
        glm::vec3& v2 = vert2->position;
        glm::vec2& uv0 = vert0->uv;
        glm::vec2& uv1 = vert1->uv;
        glm::vec2& uv2 = vert2->uv;
        // Edges of the triangle : position delta. UV delta
        glm::vec3 deltaPos1 = v1 - v0;
        glm::vec3 deltaPos2 = v2 - v0;
        glm::vec2 deltaUV1 = uv1 - uv0;
        glm::vec2 deltaUV2 = uv2 - uv0;
        float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
        glm::vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
        glm::vec3 bitangent = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r;
        glm::vec3 normal = glm::normalize(glm::cross(deltaPos1, deltaPos2));
        vert0->normal = normal;
        vert1->normal = normal;
        vert2->normal = normal;
        vert0->tangent = tangent;
        vert1->tangent = tangent;
        vert2->tangent = tangent;
    }
    /*
    inline float GetMaxXPointOfTri(Triangle& tri) {
        return std::max(std::max(tri.v0.x, tri.p2.x), tri.p3.x);
    }
    inline float GetMaxYPointOfTri(Triangle& tri) {
        return std::max(std::max(tri.v0.y, tri.p2.y), tri.p3.y);
    }
    inline float GetMaxZPointOfTri(Triangle& tri) {
        return std::max(std::max(tri.v0.z, tri.p2.z), tri.p3.z);
    }
    inline float GetMinXPointOfTri(Triangle& tri) {
        return std::min(std::min(tri.v0.x, tri.p2.x), tri.p3.x);
    }
    inline float GetMinYPointOfTri(Triangle& tri) {
        return std::min(std::min(tri.v0.y, tri.p2.y), tri.p3.y);
    }
    inline float GetMinZPointOfTri(Triangle& tri) {
        return std::min(std::min(tri.v0.z, tri.p2.z), tri.p3.z);
    }*/

   /* namespace RayTracing {

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
    }*/

    inline float MapRange(float inValue, float minInRange, float maxInRange, float minOutRange, float maxOutRange) {
        float x = (inValue - minInRange) / (maxInRange - minInRange);
        return minOutRange + (maxOutRange - minOutRange) * x;
    }

    /*inline int MapRange(int inValue, int minInRange, int maxInRange, int minOutRange, int maxOutRange) {
        float x = (inValue - minInRange) / (float)(maxInRange - minInRange);
        return minOutRange + (maxOutRange - minOutRange) * x;
    }*/

    /*
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
    }*/

    /*
    inline void EvaluateRaycasts(glm::vec3 rayOrigin, glm::vec3 rayDirection, float maxDistance, std::vector<Triangle>& triangles, RigidStaticType parentType, glm::mat4 parentMatrix, RayCastResult& out, void* parent, bool ignoreBackFacing = true) {
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
                    out.RigidStaticType = parentType;
                    out.triangeleModelMatrix = parentMatrix;
                    out.baryPosition = result.baryPosition;
                    out.found = true;
                    out.parent = parent;
                }
            }
            out.rayCount++;
        }
    }*/

    inline const std::string GetFilename(const std::string& filepath) {
        std::string result = filepath.substr(filepath.rfind("/") + 1);
        result = result.substr(0, result.length() - 4);
        return result;
    }

    inline FileInfoOLD GetFileInfo(std::string filepath) {
        // isolate name
        std::string filename = filepath.substr(filepath.rfind("/") + 1);
        filename = filename.substr(0, filename.length() - 4);
        // isolate filetype
        std::string filetype = filepath.substr(filepath.length() - 3);
        // isolate directory
        std::string directory = filepath.substr(0, filepath.rfind("/") + 1);
        // material name
        std::string materialType = "NONE";
        if (filename.length() > 5) {
            std::string query = filename.substr(filename.length() - 3);
            if (query == "ALB" || query == "RMA" || query == "NRM")
                materialType = query;
        }
        // RETURN IT
        FileInfoOLD info;
        info.fullpath = filepath;
        info.filename = filename;
        info.filetype = filetype;
        info.directory = directory;
        info.materialType = materialType;
        return info;
    }

    inline FileInfoOLD GetFileInfo(const std::filesystem::directory_entry &filepath)
    {
        const auto &path{ filepath.path() };

        static const auto get_material_type{ [] (std::string_view filename) {
			if (filename.size() > 5) {
				filename.remove_prefix(filename.size() - 3);
				if (filename == "ALB" || filename == "RMA" || filename == "NRM") {
					return std::string{ filename };
				}
			}
            return std::string{ "NONE" };
        }};

		const auto stem{ path.has_stem() ? path.stem().string() : "" };

        std::string filetype = path.has_extension() ? path.extension().string().substr(1) : "";  // remove dot


        return FileInfoOLD{
            path.string(),
            path.parent_path().string(),
            stem,
            Lowercase(filetype),
            get_material_type(stem)
        };
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

    inline glm::vec3 GetRotationFromMatrix(glm::mat4 matrix) {
        glm::mat3 rotationMatrix(
            glm::normalize(glm::vec3(matrix[0])),
            glm::normalize(glm::vec3(matrix[1])),
            glm::normalize(glm::vec3(matrix[2]))
        );
        return glm::eulerAngles(glm::quat_cast(rotationMatrix));
    }

    inline glm::vec3 GetScaleFromMatrix(glm::mat4 matrix) {
        return glm::vec3(
            glm::length(glm::vec3(matrix[0])),
            glm::length(glm::vec3(matrix[1])),
            glm::length(glm::vec3(matrix[2]))
        );
    }

    inline glm::vec3 GetForwardVectorFromMatrix(const glm::mat4& matrix) {
        return glm::normalize(glm::vec3(matrix[2]));
    }

	inline void RemoveScaleFromMatrix(glm::mat4& matrix) {
		matrix[0][3] = 1.0f;
		matrix[1][3] = 1.0f;
		matrix[2][3] = 1.0f;
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
            float aa1 = y4 - y3;
            float bb1 = x3 - x4;
            float cc1 = aa1 * (x3)+bb1 * (y3);
            float det = a * bb1 - aa1 * b;
            if (det == 0) {

                return false;
            }
            else {
                float x = (bb1 * c - b * cc1) / det;
                float y = (a * cc1 - aa1 * c) / det;
                result.x = x;
                result.y = y;
                return true;
            }
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

    inline PxMat44 PxMat4FromJSONArray(rapidjson::GenericArray<false, rapidjson::Value> const arr) {
        PxMat44 m;
        m[0][0] = arr[0].GetFloat();
        m[0][1] = arr[1].GetFloat();
        m[0][2] = arr[2].GetFloat();
        m[0][3] = arr[3].GetFloat();
        m[1][0] = arr[4].GetFloat();
        m[1][1] = arr[5].GetFloat();
        m[1][2] = arr[6].GetFloat();
        m[1][3] = arr[7].GetFloat();
        m[2][0] = arr[8].GetFloat();
        m[2][1] = arr[9].GetFloat();
        m[2][2] = arr[10].GetFloat();
        m[2][3] = arr[11].GetFloat();
        m[3][0] = arr[12].GetFloat();
        m[3][1] = arr[13].GetFloat();
        m[3][2] = arr[14].GetFloat();
        m[3][3] = arr[15].GetFloat();
        return m;
    }

    inline glm::mat4 Mat4FromJSONArray(rapidjson::GenericArray<false, rapidjson::Value> const arr) {
        glm::mat4 m;
        m[0][0] = arr[0].GetFloat();
        m[0][1] = arr[1].GetFloat();
        m[0][2] = arr[2].GetFloat();
        m[0][3] = arr[3].GetFloat();
        m[1][0] = arr[4].GetFloat();
        m[1][1] = arr[5].GetFloat();
        m[1][2] = arr[6].GetFloat();
        m[1][3] = arr[7].GetFloat();
        m[2][0] = arr[8].GetFloat();
        m[2][1] = arr[9].GetFloat();
        m[2][2] = arr[10].GetFloat();
        m[2][3] = arr[11].GetFloat();
        m[3][0] = arr[12].GetFloat();
        m[3][1] = arr[13].GetFloat();
        m[3][2] = arr[14].GetFloat();
        m[3][3] = arr[15].GetFloat();
        return m;
    }

    inline glm::vec3 Vec3FromJSONArray(rapidjson::GenericArray<false, rapidjson::Value> const arr) {
        return glm::vec3(arr[0].GetFloat(), arr[1].GetFloat(), arr[2].GetFloat());
    }

    inline PxVec3 PxVec3FromJSONArray(rapidjson::GenericArray<false, rapidjson::Value> const arr) {
        return PxVec3(arr[0].GetFloat(), arr[1].GetFloat(), arr[2].GetFloat());
    }

    inline PxQuat PxQuatFromJSONArray(rapidjson::GenericArray<false, rapidjson::Value> const arr) {
        return PxQuat(arr[0].GetFloat(), arr[1].GetFloat(), arr[2].GetFloat(), arr[3].GetFloat());
    }

    inline float Cross(const glm::vec2& v1, const glm::vec2& v2) {
        return v1.x * v2.y - v1.y * v2.x;
    }

    inline bool IsPointInTriangle2D(const glm::vec2& pt, const glm::vec2& v0, const glm::vec2& v1, const glm::vec2& v2) {
        glm::vec2 v0v1 = v1 - v0;
        glm::vec2 v0v2 = v2 - v0;
        glm::vec2 v0pt = pt - v0;
        // Precompute the determinant to avoid division in each barycentric calculation
        float denom = v0v1.x * v0v2.y - v0v1.y * v0v2.x;
        // Early out if the denominator is zero (i.e., the triangle is degenerate)
        if (denom == 0.0f) return false;
        float invDenom = 1.0f / denom;
        // Compute the barycentric coordinates (u, v, w) directly
        float v = (v0pt.x * v0v2.y - v0pt.y * v0v2.x) * invDenom;
        float w = (v0v1.x * v0pt.y - v0v1.y * v0pt.x) * invDenom;
        float u = 1.0f - v - w;
        // Check if point is inside the triangle
        return (u >= 0.0f) && (v >= 0.0f) && (w >= 0.0f);
    }

    inline bool IsPointInCircle(const glm::vec2& point, const glm::vec2& circleCenter, float radius) {
        return glm::distance(point, circleCenter) <= radius;
    }

    inline bool IsPointInTriangle(const glm::vec2& pt, const glm::vec2& v1, const glm::vec2& v2, const glm::vec2& v3) {
        float d1 = Cross(v2 - v1, pt - v1);
        float d2 = Cross(v3 - v2, pt - v2);
        float d3 = Cross(v1 - v3, pt - v3);
        bool has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
        bool has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);
        return !(has_neg && has_pos);
    }

    inline bool DoesLineIntersectCircle(const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& circleCenter, float radius) {
        glm::vec2 d = p2 - p1;
        glm::vec2 f = p1 - circleCenter;

        float a = glm::dot(d, d);
        float b = 2.0f * glm::dot(f, d);
        float c = glm::dot(f, f) - radius * radius;

        float discriminant = b * b - 4 * a * c;
        if (discriminant < 0) {
            return false; // No intersection
        }

        discriminant = sqrt(discriminant);
        float t1 = (-b - discriminant) / (2.0f * a);
        float t2 = (-b + discriminant) / (2.0f * a);

        if (t1 >= 0 && t1 <= 1) return true;
        if (t2 >= 0 && t2 <= 1) return true;

        return false;
    }

    inline bool IsTriangleOverlappingCircle(const glm::vec2& v1, const glm::vec2& v2, const glm::vec2& v3, const glm::vec2& circleCenter, float radius) {
        // Check if any of the triangle's vertices are inside the circle
        if (IsPointInCircle(v1, circleCenter, radius)) return true;
        if (IsPointInCircle(v2, circleCenter, radius)) return true;
        if (IsPointInCircle(v3, circleCenter, radius)) return true;
        // Check if the circle's center is inside the triangle
        if (IsPointInTriangle(circleCenter, v1, v2, v3)) return true;
        // Check if any of the triangle's edges intersect with the circle
        if (DoesLineIntersectCircle(v1, v2, circleCenter, radius)) return true;
        if (DoesLineIntersectCircle(v2, v3, circleCenter, radius)) return true;
        if (DoesLineIntersectCircle(v3, v1, circleCenter, radius)) return true;
        // No overlap
        return false;
    }

    inline glm::vec2 PixelToNDC(const glm::vec2& pixelPos, float viewportWidth, float viewportHeight) {
        float x = (2.0f * pixelPos.x) / viewportWidth - 1.0f;
        float y = 1.0f - (2.0f * pixelPos.y) / viewportHeight;
        return glm::vec2(x, y);
    }

    inline std::vector<glm::vec2> Generate2DVerticesFromPixelCoords(const std::vector<glm::vec2>& pixelCoords, float viewportWidth, float viewportHeight) {
        std::vector<glm::vec2> vertices;
        for (const auto& pixelPos : pixelCoords) {
            vertices.push_back(PixelToNDC(pixelPos, viewportWidth, viewportHeight));
        }
        return vertices;
    }

    inline float CalculateAngle(const glm::vec3& from, const glm::vec3& to) {
        return atan2(to.x - from.x, to.z - from.z);
    }

    inline float NormalizeAngle(float angle) {
        angle = fmod(angle + HELL_PI, 2 * HELL_PI);
        if (angle < 0) angle += 2 * HELL_PI;
        return angle - HELL_PI;
        //while (angle > HELL_PI) angle -= 2 * HELL_PI;
        //while (angle < -HELL_PI) angle += 2 * HELL_PI;
        //return angle;
    }

    inline void RotateYTowardsTarget(glm::vec3& objectPosition, float& objectYRotation, const glm::vec3& targetPosition, float rotationSpeed) {
        float desiredAngle = CalculateAngle(objectPosition, targetPosition);
        float angleDifference = NormalizeAngle(desiredAngle - objectYRotation);
        // Determine the shortest direction to rotate
        if (fabs(angleDifference) < rotationSpeed) {
            // If the difference is small, set the rotation directly
            objectYRotation = desiredAngle;
        }
        else {
            // Rotate towards the target with a maximum of rotationSpeed
            if (angleDifference > 0) {
                objectYRotation += rotationSpeed;
            }
            else {
                objectYRotation -= rotationSpeed;
            }
        }
        // Keep the rotation within the range [0, 2*pi]
        objectYRotation = fmod(objectYRotation, 2 * HELL_PI);
        if (objectYRotation < 0) {
            objectYRotation += 2 * HELL_PI;
        }
    }

    inline FacingDirection DetermineFacingDirection(const glm::vec3& forwardVector, const glm::vec3& point, const glm::vec3& origin) {
        glm::vec3 pointVector = point - origin;
        glm::vec3 forwardNormalized = glm::normalize(forwardVector);
        glm::vec3 pointNormalized = glm::normalize(pointVector);
        glm::vec3 rightVector = glm::vec3(forwardNormalized.z, 0.0f, -forwardNormalized.x);
        float dotProduct = glm::dot(pointNormalized, rightVector);
        if (dotProduct > 0) {
            return FacingDirection::LEFT;
        }
        else if (dotProduct < 0) {
            return FacingDirection::RIGHT;
        }
        // The point is directly in front or behind
        return FacingDirection::ALIGNED;
    }

    /*
    inline FacingDirection DetermineFacingDirection(glm::vec3& forwardA, const glm::vec3& forwardB) {
        glm::vec3 xzForwardA = glm::normalize(glm::vec3(forwardA.x, 0, forwardA.z));
        glm::vec3 xzForwardB = glm::normalize(glm::vec3(forwardB.x, 0, forwardB.z));
        glm::vec3 rightB = glm::vec3(xzForwardB.z, 0.0f, -xzForwardB.x);
        float dotProduct = glm::dot(xzForwardA, rightB);
        if (dotProduct > 0) {
            return FacingDirection::RIGHT;
        }
        else if (dotProduct < 0) {
            return FacingDirection::LEFT;
        }
        return FacingDirection::ALIGNED;
    }*/

    inline int RoundDownToMultiple(int inputValue, int roundingValue) {
        if (roundingValue == 0) {
            return inputValue; // Avoid division by zero
        }
        return (inputValue / roundingValue) * roundingValue;
    }

    inline void CalculateAABB(const std::vector<Vertex>& vertices, glm::vec3& aabbMin, glm::vec3& aabbMax) {
        if (vertices.empty()) {
            return;
        }
        aabbMin = vertices[0].position;
        aabbMax = vertices[0].position;
        for (const auto& vertex : vertices) {
            aabbMin = glm::min(aabbMin, vertex.position);
            aabbMax = glm::max(aabbMax, vertex.position);
        }
    }
    /*
    inline AABB GetWorldSpaceAABBFromMeshSpaceOBB(const glm::vec3& obbMin, const glm::vec3& obbMax, const glm::mat4& modelMatrix) {
        // Calculate the center of the OBB in local space
        glm::vec3 obbCenter = (obbMin + obbMax) * 0.5f;
        // Calculate the extents of the OBB (half-sizes)
        glm::vec3 obbExtent = (obbMax - obbMin) * 0.5f;
        // Extract the 3 basis vectors from the model matrix (the rotational part)
        glm::vec3 right = glm::vec3(modelMatrix[0]);   // X-axis (right)
        glm::vec3 up = glm::vec3(modelMatrix[1]);      // Y-axis (up)
        glm::vec3 forward = glm::vec3(modelMatrix[2]); // Z-axis (forward)
        // Transform the OBB center to world space
        glm::vec3 worldCenter = glm::vec3(modelMatrix * glm::vec4(obbCenter, 1.0f));
        // Calculate the new AABB extents by projecting the OBB extents along the basis vectors
        glm::vec3 worldExtent = glm::abs(obbExtent.x * right) + glm::abs(obbExtent.y * up) + glm::abs(obbExtent.z * forward);
        // The world AABB min and max
        glm::vec3 aabbMin = worldCenter - worldExtent;
        glm::vec3 aabbMax = worldCenter + worldExtent;
        return AABB(aabbMin, aabbMax);
    }*/

    inline void CreateSolidSphereVerticsAndIndices(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, float radius, int segments) {
        segments = std::max(segments, 4);
        float theta_step = glm::two_pi<float>() / segments;
        float phi_step = glm::pi<float>() / segments;
        for (int i = 0; i <= segments; ++i) {
            float phi = i * phi_step;
            for (int j = 0; j <= segments; ++j) {
                float theta = j * theta_step;
                glm::vec3 point_on_sphere(
                    radius * sin(phi) * cos(theta),
                    radius * cos(phi),
                    radius * sin(phi) * sin(theta)
                );
                glm::vec3 current_point = point_on_sphere;
                glm::vec3 normal = glm::normalize(point_on_sphere);
                glm::vec2 uv(theta / glm::two_pi<float>(), phi / glm::pi<float>());
                glm::vec3 tangent = glm::normalize(glm::vec3(-radius * sin(phi) * sin(theta), 0, radius * sin(phi) * cos(theta)));
                vertices.emplace_back(current_point, normal);
                vertices.back().uv = uv;
                vertices.back().tangent = tangent;
            }
        }
        for (int i = 0; i < segments; ++i) {
            for (int j = 0; j < segments; ++j) {
                int current = i * (segments + 1) + j;
                int next = current + segments + 1;
                indices.push_back(current);
                indices.push_back(next);
                indices.push_back(current + 1);
                indices.push_back(current + 1);
                indices.push_back(next);
                indices.push_back(next + 1);
            }
        }
    }

    inline glm::vec3 GetEulerAnglesFromForwardVector(const glm::vec3& forward) {
        glm::vec3 eulerAngles;
        // Calculate the yaw (rotation around the y-axis)
        eulerAngles.y = glm::atan(forward.x, forward.z);
        // Calculate the pitch (rotation around the x-axis)
        eulerAngles.x = glm::atan(-forward.y, glm::length(glm::vec2(forward.x, forward.z)));
        // Roll is typically not necessary unless you're dealing with banking/tilting, so set it to 0.
        eulerAngles.z = 0.0f;
        return eulerAngles;
    }

    inline float CalculateAngleDifference(float angle1, float angle2) {
        angle1 = fmod(angle1, 2 * HELL_PI);
        angle2 = fmod(angle2, 2 * HELL_PI);
        float difference = angle1 - angle2;
        if (difference > HELL_PI) {
            difference -= 2 * HELL_PI;
        }
        else if (difference < -HELL_PI) {
            difference += 2 * HELL_PI;
        }
        return difference;
    }

    inline glm::vec2 GetVogelDiskSample(int sampleIndex, int sampleCount, float spacingFactor, float phi) {
        const float goldenAngle = glm::pi<float>() * (3.0f - std::sqrt(5.0f));
        float sampleIndexF = static_cast<float>(sampleIndex);
        float sampleCountF = static_cast<float>(sampleCount);
        //float r = std::sqrt((sampleIndexF + 0.5f) / sampleCountF);
        float r = spacingFactor * std::sqrt((sampleIndexF + 0.5f) / sampleCountF);
        float theta = sampleIndexF * goldenAngle + phi;
        return glm::vec2(std::cos(theta), std::sin(theta)) * r;
    }


    inline TriangleIntersectionResult IntersectLineTriangle(const glm::vec3& orig, const glm::vec3& dir, const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2) {
        TriangleIntersectionResult result;
        const float EPSILON = 1e-6f;
        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 h = glm::cross(dir, edge2);
        float a = glm::dot(edge1, h);
        if (a > -EPSILON && a < EPSILON) {
            return result; // The line is parallel to the triangle
        }
        float f = 1.0f / a;
        glm::vec3 s = orig - v0;
        float u = f * glm::dot(s, h);
        if (u < 0.0f || u > 1.0f) {
            return result; // not sure what this is
        }
        glm::vec3 q = glm::cross(s, edge1);
        float v = f * glm::dot(dir, q);
        if (v < 0.0f || u + v > 1.0f) {
            return result; // not sure what this is
        }
        // At this stage, we can compute t to find out where the intersection point is on the line.
        float t = f * glm::dot(edge2, q);
        if (t > EPSILON) {
            result.hitFound = true;
            result.hitPosition = orig + dir * t;
            return result;
        }
        else {            
            return result;// This means that there is a line intersection, but not a ray intersection. 
        }
    }

    inline bool FloatWithinRange(float a, float b, float threshold) {
        return (a < b + threshold && a > b - threshold);
    }

    inline std::string GetFileName(const std::string& filepath) {
        // Find the last slash or backslash to remove directory paths
        std::string::size_type pos = filepath.find_last_of("/\\");
        std::string filename = (pos == std::string::npos) ? filepath : filepath.substr(pos + 1);
        // Find the last dot to remove the file extension
        pos = filename.find_last_of('.');
        if (pos != std::string::npos) {
            filename = filename.substr(0, pos);
        }
        return filename;
    }

    inline std::string RemoveFileExtension(const std::string& filename) {
        size_t pos = filename.find_last_of('.');
        if (pos != std::string::npos) {
            return filename.substr(0, pos);
        }
        return filename;
    }

    inline std::string GetFullPath(const std::filesystem::directory_entry& entry) {
        return entry.path().string();
    }

    inline std::string GetFileName(const std::filesystem::directory_entry& entry) {
        return entry.path().filename().string();
    }

    inline std::string GetFileNameWithoutExtension(const std::filesystem::directory_entry& entry) {
        return entry.path().stem().string(); // stem() removes the extension
    }

    inline std::string GetFileExtension(const std::filesystem::directory_entry& entry) {
        return entry.path().extension().string().substr(1);
    }

    inline std::vector<FileInfo> IterateDirectory(const std::string& directory, std::vector<std::string> extensions = std::vector<std::string>()) {
        std::vector<FileInfo> fileInfos;
        auto entries = std::filesystem::directory_iterator(directory);
        for (const auto& entry : entries) {
            // Skip directories
            if (!std::filesystem::is_regular_file(entry)) {
                continue;
            }
            FileInfo fileInfo;
            fileInfo.path = Util::GetFullPath(entry);
            fileInfo.name = Util::GetFileNameWithoutExtension(entry);
            fileInfo.ext = Util::GetFileExtension(entry);
            fileInfo.dir = directory;
            if (extensions.empty()) {
                fileInfos.push_back(fileInfo);
            }
            else {
                for (std::string& ext : extensions) {
                    if (ext == fileInfo.ext) {
                        fileInfos.push_back(fileInfo);
                        break;
                    }
                }
            }
        }
        return fileInfos;
    }

    inline FileInfo GetFileInfoFromPath(const std::string& filepath) {
        FileInfo fileInfo;
        std::filesystem::path path(filepath);
        if (!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path)) {
            throw std::runtime_error("Invalid file path or not a regular file: " + filepath);
        }
        fileInfo.path = path.string();
        fileInfo.name = path.stem().string();
        fileInfo.ext = path.has_extension() ? path.extension().string().substr(1) : "";
        fileInfo.dir = path.parent_path().string();

        return fileInfo;
    }

    inline AABB GetAABBFromVertices(std::vector<Vertex>& vertices) {
        glm::vec3 aabbMin = glm::vec3(std::numeric_limits<float>::max());
        glm::vec3 aabbMax = glm::vec3(-std::numeric_limits<float>::max());
        for (Vertex& vertex : vertices) {
            aabbMin = Util::Vec3Min(vertex.position, aabbMin);
            aabbMax = Util::Vec3Max(vertex.position, aabbMax);
        }
        return AABB(aabbMin, aabbMax);
    }


}

