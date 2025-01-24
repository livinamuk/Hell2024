#pragma once
#include <vector>
#include "glm/glm.hpp"
#include "recast/Recast.h"
#include "recast/RecastAlloc.h"
#include "recast/RecastAssert.h"
#include "detour/DetourNavMesh.h"
#include "detour/DetourNavMeshQuery.h"
#include "detour/DetourNavMeshBuilder.h"
#include "detour/DetourTileCache.h"
#include "detour/DetourTileCacheBuilder.h"
#include "detour/DetourAlloc.h"
#include "detour/DetourCommon.h"
#include "detour/fastlz.h"

#define MAX_POLYS 256
#define EXPECTED_LAYERS_PER_TILE 4

enum class NavMeshRegionMode {
    WATER_SHED,
    MONOTONE
};

/*
class MyTileCacheAllocator : public dtTileCacheAlloc {
public:
    virtual void* alloc(const size_t size) override {
        return dtAlloc(size, DT_ALLOC_PERM);
    }

    virtual void free(void* ptr) override {
        dtFree(ptr);
    }
*/

/*
class MyTileCacheCompressor : public dtTileCacheCompressor {
public:
    virtual dtStatus compress(const unsigned char* buffer, const int bufferSize,
        unsigned char* compressed, const int maxCompressedSize,
        int* compressedSize) override {
        // Example: Simple copy compression (not real compression)
        if (bufferSize > maxCompressedSize) return DT_FAILURE;
        memcpy(compressed, buffer, bufferSize);
        *compressedSize = bufferSize;
        return DT_SUCCESS;
    }

    virtual dtStatus decompress(const unsigned char* compressed, const int compressedSize,
        unsigned char* buffer, const int maxBufferSize,
        int* bufferSize) override {
        // Example: Simple copy decompression (not real decompression)
        if (compressedSize > maxBufferSize) return DT_FAILURE;
        memcpy(buffer, compressed, compressedSize);
        *bufferSize = compressedSize;
        return DT_SUCCESS;
    }
    virtual int maxCompressedSize(const int bufferSize) override {
        // Example: Return buffer size as the "maximum" compressed size
        return bufferSize;
    }
};*/

/*
struct MeshProcess : public dtTileCacheMeshProcess {
    InputGeom* m_geom;

    inline MeshProcess() : m_geom(0)
    {
    }

    virtual ~MeshProcess();

    inline void init(InputGeom* geom)
    {
        m_geom = geom;
    }

    virtual void process(struct dtNavMeshCreateParams* params,
        unsigned char* polyAreas, unsigned short* polyFlags)
    {
        // Update poly flags from areas.
        for (int i = 0; i < params->polyCount; ++i)
        {
            if (polyAreas[i] == DT_TILECACHE_WALKABLE_AREA)
                polyAreas[i] = SAMPLE_POLYAREA_GROUND;

            if (polyAreas[i] == SAMPLE_POLYAREA_GROUND ||
                polyAreas[i] == SAMPLE_POLYAREA_GRASS ||
                polyAreas[i] == SAMPLE_POLYAREA_ROAD)
            {
                polyFlags[i] = SAMPLE_POLYFLAGS_WALK;
            }
            else if (polyAreas[i] == SAMPLE_POLYAREA_WATER)
            {
                polyFlags[i] = SAMPLE_POLYFLAGS_SWIM;
            }
            else if (polyAreas[i] == SAMPLE_POLYAREA_DOOR)
            {
                polyFlags[i] = SAMPLE_POLYFLAGS_WALK | SAMPLE_POLYFLAGS_DOOR;
            }
        }

        // Pass in off-mesh connections.
        if (m_geom)
        {
            params->offMeshConVerts = m_geom->getOffMeshConnectionVerts();
            params->offMeshConRad = m_geom->getOffMeshConnectionRads();
            params->offMeshConDir = m_geom->getOffMeshConnectionDirs();
            params->offMeshConAreas = m_geom->getOffMeshConnectionAreas();
            params->offMeshConFlags = m_geom->getOffMeshConnectionFlags();
            params->offMeshConUserID = m_geom->getOffMeshConnectionId();
            params->offMeshConCount = m_geom->getOffMeshConnectionCount();
        }
    }
};*/


/*
class MyTileCacheMeshProcess : public dtTileCacheMeshProcess {
public:
    virtual void process(dtNavMeshCreateParams* params,
        unsigned char* polyAreas, unsigned short* polyFlags) override {
        for (int i = 0; i < params->polyCount; ++i) {
            polyAreas[i] = 0;
            polyFlags[i] = 0;
        }
    }
};*/

struct NavMesh {

    void Create(rcContext* context, std::vector<glm::vec3>& vertices, NavMeshRegionMode regionMode);
    //void Create2(rcContext* context, std::vector<glm::vec3>& vertices, NavMeshRegionMode regionMode);
    std::vector<glm::vec3> FindPath(glm::vec3 startPos, glm::vec3 endPos);
    rcPolyMesh* GetPolyMesh();
    dtNavMesh* GetDtNaveMesh();
    dtTileCache* GetTileCache();

private:
    void CleanUp();
    float GetSizeX();
    float GetSizeY();
    float GetSizeZ();
    float GetBoundsMinX();
    float GetBoundsMinY();
    float GetBoundsMinZ();
    float GetBoundsMaxX();
    float GetBoundsMaxY();
    float GetBoundsMaxZ();
    const float* GetBoundsMin() const;
    const float* GetBoundsMax() const;
    float* GetVertices();
    int* GetTriangles();
    int GetVertexCount();
    int GetTriCount();
    void AllocateMemory();

    struct LinearAllocator* m_talloc;
    struct FastLZCompressor* m_tcomp;
    struct MeshProcess* m_tmproc;

    bool m_memoryAllocated = false;

    //LinearAllocator* m_talloc;
    //FastLZCompressor* m_tcomp;
    //MeshProcess* m_tmproc;
    int m_cacheCompressedSize;
    int m_cacheRawSize;
    int m_cacheLayerCount;

    int rasterizeTileLayers(rcContext* context, std::vector<glm::vec3>& vertices, NavMeshRegionMode regionMode, const int tx, const int ty, const rcConfig& cfg, struct TileCacheData* tiles, const int maxTiles);

    //LinearAllocator myTileCacheAllocator;
    //FastLZCompressor myTileCacheCompressor;
    //MyTileCacheMeshProcess myTileCacheMeshProcess;

private:
    rcHeightfield* m_heightField = nullptr;
    rcCompactHeightfield* m_compactHeightField = nullptr;
    rcContourSet* m_contourSet = nullptr;
    rcPolyMesh* m_polyMesh = nullptr;
    rcPolyMeshDetail* m_polyMeshDetail = nullptr;
    dtNavMesh* m_navMesh = nullptr;
    dtNavMeshQuery* m_navQuery = nullptr;
    dtTileCache* m_tileCache = nullptr;
    std::vector<glm::vec3> m_vertices;
    std::vector<int> m_indices;
    float m_boundsMin[3] = { 0, 0, 0 };
    float m_boundsMax[3] = { 0, 0, 0 };
};