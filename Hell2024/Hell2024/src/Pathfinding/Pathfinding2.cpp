#include "Pathfinding2.h"
#include "recast/DebugDraw.h"
#include "recast/RecastDebugDraw.h"
#include <vector>
#include <iostream>
#include "glm/glm.hpp"
#include "../Timer.hpp"

#include "../Editor/CSG.h"
#include "../Input/Input.h"
#include "../Game/Game.h"
#include "../Core/AssetManager.h"

class MyDebugDraw : public duDebugDraw {
public:
    std::vector<glm::vec3> debugVertices;
    virtual void depthMask(bool state) override {}
    virtual void texture(bool state) override {}
    virtual void begin(duDebugDrawPrimitives prim, float size = 1.0f) override {}
    virtual void vertex(const float x, const float y, const float z, unsigned int color) override {
        debugVertices.push_back(glm::vec3(x, y, z));
    }
    virtual void vertex(const float* pos, unsigned int color, const float* uv) override {
        debugVertices.push_back(glm::vec3(pos[0], pos[1], pos[2]));
    }
    virtual void vertex(const float* pos, unsigned int color) override {
        debugVertices.push_back(glm::vec3(pos[0], pos[1], pos[2]));
    }
    virtual void vertex(const float x, const float y, const float z, unsigned int color, const float u, const float v) override {
        debugVertices.push_back(glm::vec3(x, y, z));
    }
    virtual void end() override {}
};




void DrawNavMesh(MyDebugDraw& dd, NavMesh& navMesh, dtTileCache* tileCache, std::vector<glm::vec3>& vector) {
    if (!navMesh.GetDtNaveMesh() || !tileCache) {
        return;
    }

    vector.clear();
    const dtNavMesh* mesh = navMesh.GetDtNaveMesh();

    for (int i = 0; i < mesh->getMaxTiles(); ++i) {
        const dtMeshTile* tile = mesh->getTile(i);
        if (!tile || !tile->header) {
            continue;
        }

        for (int j = 0; j < tile->header->polyCount; ++j) {
            const dtPoly* poly = &tile->polys[j];
            if (poly->getType() == DT_POLYTYPE_OFFMESH_CONNECTION) {
                continue;
            }

            for (int k = 0; k < poly->vertCount; ++k) {
                const float* v0 = &tile->verts[poly->verts[k] * 3];
                const float* v1 = &tile->verts[poly->verts[(k + 1) % poly->vertCount] * 3];

                vector.push_back(glm::vec3(v0[0], v0[1], v0[2]));
                vector.push_back(glm::vec3(v1[0], v1[1], v1[2]));
            }
        }
    }
}




void DrawNavMesh(MyDebugDraw& dd, NavMesh& navMesh, std::vector<glm::vec3>& vector) {

    //Timer timer("DrawNavMesh()");

    if (!navMesh.GetDtNaveMesh()) {
        return;
    }
    vector.clear();
    const dtNavMesh* mesh = navMesh.GetDtNaveMesh();
    for (int i = 0; i < mesh->getMaxTiles(); ++i) {
        const dtMeshTile* tile = mesh->getTile(i);
        if (!tile || !tile->header) {
            continue;
        }
        for (int j = 0; j < tile->header->polyCount; ++j) {
            const dtPoly* poly = &tile->polys[j];
            if (poly->getType() == DT_POLYTYPE_OFFMESH_CONNECTION) {
                continue;
            }
            for (int k = 0; k < poly->vertCount; ++k) {
                const float* v0 = &tile->verts[poly->verts[k] * 3];
                const float* v1 = &tile->verts[poly->verts[(k + 1) % poly->vertCount] * 3];
                vector.push_back(glm::vec3(v0[0], v0[1], v0[2]));
                vector.push_back(glm::vec3(v1[0], v1[1], v1[2]));
            }
        }
    }

    const int maxTiles = navMesh.GetTileCache()->getTileCount();

    for (int i = 0; i < maxTiles; ++i) {
        const dtCompressedTile* tile = navMesh.GetTileCache()->getTile(i);
        if (!tile) {
            continue;
        }

        const dtTileCacheObstacle* obstacle = navMesh.GetTileCache()->getObstacleByRef(tile->salt);
        while (obstacle) {
            if (obstacle->state == DT_OBSTACLE_EMPTY) {
                obstacle = obstacle->next;
                continue;
            }

            std::cout << "found obstacle " << i << "\n";

            // Get the obstacle's bounds (AABB)
            const float* bmin = obstacle->box.bmin;
            const float* bmax = obstacle->box.bmax;

            // Define the 8 corners of the AABB
            glm::vec3 corners[8];
            corners[0] = glm::vec3(bmin[0], bmin[1], bmin[2]);
            corners[1] = glm::vec3(bmax[0], bmin[1], bmin[2]);
            corners[2] = glm::vec3(bmax[0], bmax[1], bmin[2]);
            corners[3] = glm::vec3(bmin[0], bmax[1], bmin[2]);
            corners[4] = glm::vec3(bmin[0], bmin[1], bmax[2]);
            corners[5] = glm::vec3(bmax[0], bmin[1], bmax[2]);
            corners[6] = glm::vec3(bmax[0], bmax[1], bmax[2]);
            corners[7] = glm::vec3(bmin[0], bmax[1], bmax[2]);

            // Add the edges of the AABB to the debug vector
            vector.push_back(corners[0]); vector.push_back(corners[1]);
            vector.push_back(corners[1]); vector.push_back(corners[2]);
            vector.push_back(corners[2]); vector.push_back(corners[3]);
            vector.push_back(corners[3]); vector.push_back(corners[0]);

            vector.push_back(corners[4]); vector.push_back(corners[5]);
            vector.push_back(corners[5]); vector.push_back(corners[6]);
            vector.push_back(corners[6]); vector.push_back(corners[7]);
            vector.push_back(corners[7]); vector.push_back(corners[4]);

            vector.push_back(corners[0]); vector.push_back(corners[4]);
            vector.push_back(corners[1]); vector.push_back(corners[5]);
            vector.push_back(corners[2]); vector.push_back(corners[6]);
            vector.push_back(corners[3]); vector.push_back(corners[7]);

            obstacle = obstacle->next;  // Move to the next obstacle
        }
    }
}

namespace Pathfinding2 {

    rcContext* g_ctx;
    MyDebugDraw g_debugDraw;
    NavMesh g_navMesh;
    std::vector<glm::vec3> g_debugVertices;

    void Init() {
        g_ctx = new rcContext();
        std::cout << "Recast initialized\n";
    }

    void Update(float deltaTime) {

        // do door shit here
        //g_navMesh.GetTileCache()->update(deltaTime, g_navMesh.GetDtNaveMesh());

        //Timer timer("navmesh construction");
        //UpdateNavMesh(CSG::GetNavMeshVertices());
    }

    void CalculateNavMesh() {

        //Timer timer("Pathfinding::UpdateNavMesh()");

        std::vector<glm::vec3> vertices = CSG::GetNavMeshVertices();
        std::cout << "CSG vertices: " << vertices.size() << "\n";

        /*      
        // Add heightmap
        HeightMap& heightMap = AssetManager::g_heightMap;
        vertices.reserve(vertices.size() + heightMap.m_indices.size());
        for (int i = 0; i < 66; i++) {
        //for (int i = 0; i < heightMap.m_indices.size(); i++) {
            int vertexIndex = heightMap.m_indices[i];           
            Vertex& vertex = heightMap.m_vertices[vertexIndex];
            glm::vec3 position = heightMap.m_transform.to_mat4() * glm::vec4(vertex.position, 1.0);
            vertices.push_back(position);
        }
        */


        // Hack to add height map to nav mesh
         float y = -1.75f;
         float size = 50;
        
         glm::vec3 cornerA = glm::vec3(-size, y, -size);
         glm::vec3 cornerB = glm::vec3(size, y, size);
         glm::vec3 cornerC = glm::vec3(-size, y, size);
         glm::vec3 cornerD = glm::vec3(size, y, -size);
        
         vertices.push_back(cornerC);
         vertices.push_back(cornerB);
         vertices.push_back(cornerA);
        
         vertices.push_back(cornerB);
         vertices.push_back(cornerD);
         vertices.push_back(cornerA);

        g_navMesh.Create(g_ctx, vertices, NavMeshRegionMode::MONOTONE);
        DrawNavMesh(g_debugDraw, g_navMesh, g_debugVertices);
    }

    Path FindPath(glm::vec3 startPos, glm::vec3 endPos) {

        Path path;
        path.start = startPos;
        path.end = endPos;
        path.points = g_navMesh.FindPath(startPos, endPos);
        return path;
    }

    std::vector<glm::vec3>& GetDebugVertices() {
        return g_debugVertices;
    }

    rcContext* GetRecastContext() {
        return g_ctx;
    }
}