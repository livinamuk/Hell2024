#include "NavMesh.h"
#include <iostream>


struct FastLZCompressor : public dtTileCacheCompressor {
    virtual ~FastLZCompressor();

    virtual int maxCompressedSize(const int bufferSize) {
        return (int)(bufferSize * 1.05f);
    }
    virtual dtStatus compress(const unsigned char* buffer, const int bufferSize, unsigned char* compressed, const int /*maxCompressedSize*/, int* compressedSize) {
        *compressedSize = fastlz_compress((const void* const)buffer, bufferSize, compressed);
        return DT_SUCCESS;
    }
    virtual dtStatus decompress(const unsigned char* compressed, const int compressedSize, unsigned char* buffer, const int maxBufferSize, int* bufferSize) {
        *bufferSize = fastlz_decompress(compressed, compressedSize, buffer, maxBufferSize);
        return *bufferSize < 0 ? DT_FAILURE : DT_SUCCESS;
    }
};

FastLZCompressor::~FastLZCompressor() {
    // Defined out of line to fix the weak v-tables warning
}


struct LinearAllocator : public dtTileCacheAlloc {
    unsigned char* buffer;
    size_t capacity;
    size_t top;
    size_t high;

    LinearAllocator() = default;
    LinearAllocator(const size_t cap) : buffer(0), capacity(0), top(0), high(0) {
        resize(cap);
    }

    virtual ~LinearAllocator();

    void resize(const size_t cap) {
        if (buffer) dtFree(buffer);
        buffer = (unsigned char*)dtAlloc(cap, DT_ALLOC_PERM);
        capacity = cap;
    }

    virtual void reset() {
        high = dtMax(high, top);
        top = 0;
    }

    virtual void* alloc(const size_t size) {
        if (!buffer)
            return 0;
        if (top + size > capacity)
            return 0;
        unsigned char* mem = &buffer[top];
        top += size;
        return mem;
    }

    virtual void free(void* /*ptr*/) {
        // Empty
    }
};

LinearAllocator::~LinearAllocator() {
    // Defined out of line to fix the weak v-tables warning
    dtFree(buffer);
}



struct MeshProcess : public dtTileCacheMeshProcess {
    inline MeshProcess() {}
    virtual ~MeshProcess();

    virtual void process(struct dtNavMeshCreateParams* params, unsigned char* polyAreas, unsigned short* polyFlags) {
        for (int i = 0; i < params->polyCount; ++i) {
            polyAreas[i] = 1;
            polyFlags[i] = 1;
        }
    }
};

MeshProcess::~MeshProcess() {
    // Defined out of line to fix the weak v-tables warning
}

static const int MAX_LAYERS = 32;

struct TileCacheData {
    unsigned char* data;
    int dataSize;
};






void NavMesh::CleanUp() {

    if (m_heightField) {
        rcFreeHeightField(m_heightField);
        m_heightField = nullptr;
    }
    if (m_compactHeightField) {
        rcFreeCompactHeightfield(m_compactHeightField);
        m_compactHeightField = nullptr;
    }
    if (m_contourSet) {
        rcFreeContourSet(m_contourSet);
        m_contourSet = nullptr;
    }
    if (m_polyMesh) {
        rcFreePolyMesh(m_polyMesh);
        m_polyMesh = nullptr;
    }
    if (m_polyMeshDetail) {
        rcFreePolyMeshDetail(m_polyMeshDetail);
        m_polyMeshDetail = nullptr;
    }
    if (m_navMesh) {
        dtFreeNavMesh(m_navMesh);
        m_navMesh = nullptr;
    }
    if (m_navQuery) {
        dtFreeNavMeshQuery(m_navQuery);
        m_navQuery = nullptr;
    }
    if (m_tileCache) {
        dtFreeTileCache(m_tileCache);
        m_tileCache = nullptr;
    }
    if (m_talloc) {
        delete m_talloc;
        m_talloc = nullptr;
    }
    if (m_tcomp) {
        delete m_tcomp;
        m_tcomp = nullptr;
    }
    if (m_tmproc) {
        delete m_tmproc;
        m_tmproc = nullptr;
    }
}

dtNavMesh* NavMesh::GetDtNaveMesh() {
    return m_navMesh;
}

rcPolyMesh* NavMesh::GetPolyMesh() {
    return m_polyMesh;
}

dtTileCache* NavMesh::GetTileCache() {
    return m_tileCache;
}

int calcLayerBufferSize(const int gridWidth, const int gridHeight) {
    const int headerSize = dtAlign4(sizeof(dtTileCacheLayerHeader));
    const int gridSize = gridWidth * gridHeight;
    return headerSize + gridSize * 4;
}

/*void NavMesh::Create2(rcContext* context, std::vector<glm::vec3>& vertices, NavMeshRegionMode regionMode) {

    //CleanUp();

    m_heightField = rcAllocHeightfield();
    m_compactHeightField = rcAllocCompactHeightfield();
    m_polyMesh = rcAllocPolyMesh();
    m_contourSet = rcAllocContourSet();
    m_polyMesh = rcAllocPolyMesh();
    m_polyMeshDetail = rcAllocPolyMeshDetail();
    m_navMesh = dtAllocNavMesh();
    m_navQuery = dtAllocNavMeshQuery();
    m_tileCache = dtAllocTileCache();

    m_talloc = new LinearAllocator(32000);
    m_tcomp = new FastLZCompressor;
    m_tmproc = new MeshProcess;

    m_vertices = vertices;
    m_indices.clear();

    for (size_t i = 0; i < vertices.size(); i++) {
        m_indices.push_back(i);
    }
    m_boundsMin[0] = std::numeric_limits<float>::max();
    m_boundsMin[1] = std::numeric_limits<float>::max();
    m_boundsMin[2] = std::numeric_limits<float>::max();
    m_boundsMax[0] = -std::numeric_limits<float>::max();
    m_boundsMax[1] = -std::numeric_limits<float>::max();
    m_boundsMax[2] = -std::numeric_limits<float>::max();

    for (glm::vec3& vertex : vertices) {
        m_boundsMin[0] = std::min(m_boundsMin[0], vertex.x);
        m_boundsMin[1] = std::min(m_boundsMin[1], vertex.y);
        m_boundsMin[2] = std::min(m_boundsMin[2], vertex.z);
        m_boundsMax[0] = std::max(m_boundsMax[0], vertex.x);
        m_boundsMax[1] = std::max(m_boundsMax[1], vertex.y);
        m_boundsMax[2] = std::max(m_boundsMax[2], vertex.z);
    }

    float m_cellSize = 0.075;
    float m_tileSize = 1.0f;
    float m_cellHeight = 0.2f;
    float m_agentMaxSlope = 45.0f;
    float m_agentHeight = 1.0f;
    float m_agentRadius = 1.0f;
    float m_agentMaxClimb = 1.0f;
    float m_edgeMaxLen = 12.0f;
    float m_edgeMaxError = 1.0f;
    float m_regionMinSize = 4.0f;
    float m_regionMergeSize = 10.0f;
    int m_vertsPerPoly = 6;
    float m_detailSampleDist = 1.0f;
    float m_detailSampleMaxError = 0.25f;

    // Init cache
    const float* bmin = m_boundsMin;
    const float* bmax = m_boundsMax;
    int gw = 0, gh = 0;
    rcCalcGridSize(bmin, bmax, m_cellSize, &gw, &gh);
    const int ts = (int)m_tileSize;
    const int tw = (gw + ts - 1) / ts;
    const int th = (gh + ts - 1) / ts;

    int tileBits = rcMin((int)dtIlog2(dtNextPow2(tw * th * EXPECTED_LAYERS_PER_TILE)), 14);
    if (tileBits > 14) {
        tileBits = 14;
    }
    int polyBits = 22 - tileBits;

    int m_maxTiles = 1 << tileBits;
    int m_maxPolysPerTile = 1 << polyBits;

    // Generation params.
    rcConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.cs = m_cellSize;
    cfg.ch = m_cellHeight;
    cfg.walkableSlopeAngle = m_agentMaxSlope;
    cfg.walkableHeight = (int)ceilf(m_agentHeight / cfg.ch);
    cfg.walkableClimb = (int)floorf(m_agentMaxClimb / cfg.ch);
    cfg.walkableRadius = (int)ceilf(m_agentRadius / cfg.cs);
    cfg.maxEdgeLen = (int)(m_edgeMaxLen / m_cellSize);
    cfg.maxSimplificationError = m_edgeMaxError;
    cfg.minRegionArea = (int)rcSqr(m_regionMinSize);		// Note: area = size*size
    cfg.mergeRegionArea = (int)rcSqr(m_regionMergeSize);	// Note: area = size*size
    cfg.maxVertsPerPoly = (int)m_vertsPerPoly;
    cfg.tileSize = (int)m_tileSize;
    cfg.borderSize = cfg.walkableRadius + 3; // Reserve enough padding.
    cfg.width = cfg.tileSize + cfg.borderSize * 2;
    cfg.height = cfg.tileSize + cfg.borderSize * 2;
    cfg.detailSampleDist = m_detailSampleDist < 0.9f ? 0 : m_cellSize * m_detailSampleDist;
    cfg.detailSampleMaxError = m_cellHeight * m_detailSampleMaxError;
    rcVcopy(cfg.bmin, bmin);
    rcVcopy(cfg.bmax, bmax);

    // Tile cache params.
    dtTileCacheParams tcparams;
    memset(&tcparams, 0, sizeof(tcparams));
    rcVcopy(tcparams.orig, bmin);
    tcparams.cs = m_cellSize;
    tcparams.ch = m_cellHeight;
    tcparams.width = (int)m_tileSize;
    tcparams.height = (int)m_tileSize;
    tcparams.walkableHeight = m_agentHeight;
    tcparams.walkableRadius = m_agentRadius;
    tcparams.walkableClimb = m_agentMaxClimb;
    tcparams.maxSimplificationError = m_edgeMaxError;
    tcparams.maxTiles = tw * th * EXPECTED_LAYERS_PER_TILE;
    tcparams.maxObstacles = 128;

    dtFreeTileCache(m_tileCache);
    m_tileCache = dtAllocTileCache();
    if (!m_tileCache) {
        std::cout << "buildTiledNavigation: Could not allocate tile cache.\n";
    }
    dtStatus status = m_tileCache->init(&tcparams, m_talloc, m_tcomp, m_tmproc);
    if (dtStatusFailed(status)) {
        std::cout << "buildTiledNavigation: Could not init tile cache.\n";
    }
    dtFreeNavMesh(m_navMesh);
    m_navMesh = dtAllocNavMesh();
    if (!m_navMesh) {
        std::cout << "buildTiledNavigation: Could not allocate navmesh.\n";
    }

    dtNavMeshParams params;
    memset(&params, 0, sizeof(params));
    rcVcopy(params.orig, bmin);
    params.tileWidth = m_tileSize * m_cellSize;
    params.tileHeight = m_tileSize * m_cellSize;
    params.maxTiles = m_maxTiles;
    params.maxPolys = m_maxPolysPerTile;

    status = m_navMesh->init(&params);
    if (dtStatusFailed(status)) {
        std::cout << "buildTiledNavigation: Could not init navmesh.\n";
    }

    status = m_navQuery->init(m_navMesh, 2048);
    if (dtStatusFailed(status)) {
        std::cout << "buildTiledNavigation: Could not init Detour navmesh query\n";
    }


    // Pre-process tiles.

    context->resetTimers();

    m_cacheLayerCount = 0;
    m_cacheCompressedSize = 0;
    m_cacheRawSize = 0;

    for (int y = 0; y < th; ++y)
    {
        for (int x = 0; x < tw; ++x)
        {
            TileCacheData tiles[MAX_LAYERS];
            memset(tiles, 0, sizeof(tiles));
            int ntiles = rasterizeTileLayers(context, vertices, regionMode, x, y, cfg, tiles, MAX_LAYERS);

            for (int i = 0; i < ntiles; ++i)
            {
                TileCacheData* tile = &tiles[i];
                status = m_tileCache->addTile(tile->data, tile->dataSize, DT_COMPRESSEDTILE_FREE_DATA, 0);
                if (dtStatusFailed(status))
                {
                    dtFree(tile->data);
                    tile->data = 0;
                    continue;
                }

                m_cacheLayerCount++;
                m_cacheCompressedSize += tile->dataSize;
                m_cacheRawSize += calcLayerBufferSize(tcparams.width, tcparams.height);
            }
        }
    }

    // Build initial meshes
    context->startTimer(RC_TIMER_TOTAL);
    for (int y = 0; y < th; ++y)
        for (int x = 0; x < tw; ++x)
            m_tileCache->buildNavMeshTilesAt(x, y, m_navMesh);
    context->stopTimer(RC_TIMER_TOTAL);

    //m_cacheBuildTimeMs = m_ctx->getAccumulatedTime(RC_TIMER_TOTAL) / 1000.0f;
    //m_cacheBuildMemUsage = static_cast<unsigned int>(m_talloc->high);


    const dtNavMesh* nav = m_navMesh;
    int navmeshMemUsage = 0;
    for (int i = 0; i < nav->getMaxTiles(); ++i)
    {
        const dtMeshTile* tile = nav->getTile(i);
        if (tile->header)
            navmeshMemUsage += tile->dataSize;
    }
    printf("navmeshMemUsage = %.1f kB", navmeshMemUsage / 1024.0f);


    //if (m_tool)
    //    m_tool->init(this);
    //initToolStates(this);

    //return true;

    
   // const dtNavMesh* mesh = GetDtNaveMesh();
   // for (int i = 0; i < mesh->getMaxTiles(); ++i) {
   //     const dtMeshTile* tile = mesh->getTile(i);
   //     if (!tile || !tile->header) {
   //         std::cout << "Tile " << i << " is invalid or has no header." << std::endl;
   //         continue;
   //     }
   //     std::cout << "Tile " << i << " is valid with " << tile->header->polyCount << " polygons." << std::endl;
   //
   //
   //     for (int j = 0; j < tile->header->polyCount; ++j) {
   //         const dtPoly* poly = &tile->polys[j];
   //         std::cout << "Poly " << j << " has " << poly->vertCount << " vertices." << std::endl;
   //         for (int k = 0; k < poly->vertCount; ++k) {
   //             const float* v = &tile->verts[poly->verts[k] * 3];
   //             std::cout << "Vertex " << k << ": (" << v[0] << ", " << v[1] << ", " << v[2] << ")" << std::endl;
   //         }
   //     }
   // }
   //
   // for (int i = 0; i < GetTileCache()->getTileCount(); ++i) {
   //     const dtCompressedTile* tile = GetTileCache()->getTile(i);
   //     if (tile && tile->header) {
   //         std::cout << "TileCache Tile " << i << " is valid." << std::endl;
   //     }
   //     else {
   //         std::cout << "TileCache Tile " << i << " is invalid." << std::endl;
   //     }
   // }
    
}*/
#include "Timer.hpp"

void NavMesh::AllocateMemory() {

    m_talloc = new LinearAllocator(32000);
    m_tcomp = new FastLZCompressor;
    m_tmproc = new MeshProcess;

    m_heightField = rcAllocHeightfield();
    m_compactHeightField = rcAllocCompactHeightfield();
    m_polyMesh = rcAllocPolyMesh();
    m_contourSet = rcAllocContourSet();
    m_polyMesh = rcAllocPolyMesh();
    m_polyMeshDetail = rcAllocPolyMeshDetail();
    m_navMesh = dtAllocNavMesh();
    m_navQuery = dtAllocNavMeshQuery();
    m_tileCache = dtAllocTileCache();

    m_memoryAllocated = true;
    //std::cout << "Navmesh memory allocated\n";
}

void NavMesh::Create(rcContext* context, std::vector<glm::vec3>& vertices, NavMeshRegionMode regionMode) {

    //Timer timer("NavMesh::Create");

    //CleanUp();

    if (!m_memoryAllocated) {
        AllocateMemory();
    }

    m_vertices = vertices;
    m_indices.clear();
    m_indices.reserve(vertices.size());

    for (size_t i = 0; i < vertices.size(); i++) {
        m_indices.push_back(i);
    }
    m_boundsMin[0] = std::numeric_limits<float>::max();
    m_boundsMin[1] = std::numeric_limits<float>::max();
    m_boundsMin[2] = std::numeric_limits<float>::max();
    m_boundsMax[0] = -std::numeric_limits<float>::max();
    m_boundsMax[1] = -std::numeric_limits<float>::max();
    m_boundsMax[2] = -std::numeric_limits<float>::max();

    for (glm::vec3& vertex : vertices) {
        m_boundsMin[0] = std::min(m_boundsMin[0], vertex.x);
        m_boundsMin[1] = std::min(m_boundsMin[1], vertex.y);
        m_boundsMin[2] = std::min(m_boundsMin[2], vertex.z);
        m_boundsMax[0] = std::max(m_boundsMax[0], vertex.x);
        m_boundsMax[1] = std::max(m_boundsMax[1], vertex.y);
        m_boundsMax[2] = std::max(m_boundsMax[2], vertex.z);
    }
    rcConfig cfg = {};
    cfg.cs = 0.075;
    cfg.ch = 0.4;
    cfg.walkableSlopeAngle = 65.0f;
    cfg.walkableHeight = 1;
    cfg.walkableClimb = 1;
    cfg.walkableRadius = 1;
    cfg.maxEdgeLen = 12.0f;
    cfg.maxSimplificationError = 1;
    cfg.minRegionArea = 4;
    cfg.mergeRegionArea = 10;
    cfg.maxVertsPerPoly = (int)6;
    cfg.detailSampleDist = 1.0f;
    cfg.detailSampleMaxError = 0.25f;
    cfg.tileSize = 1.0f;
    cfg.bmin[0] = GetBoundsMinX();
    cfg.bmin[1] = GetBoundsMinY();
    cfg.bmin[2] = GetBoundsMinZ();
    cfg.bmax[0] = GetBoundsMaxX();
    cfg.bmax[1] = GetBoundsMaxY();
    cfg.bmax[2] = GetBoundsMaxZ();
    rcCalcGridSize(cfg.bmin, cfg.bmax, cfg.cs, &cfg.width, &cfg.height);

    unsigned char* triareas = new unsigned char[GetTriCount()];
    memset(triareas, 0, GetTriCount() * sizeof(unsigned char));

    if (!rcCreateHeightfield(context, *m_heightField, cfg.width, cfg.height, GetBoundsMin(), GetBoundsMax(), cfg.cs, cfg.ch)) {
        std::cout << "Heightfield creation failed!\n";
    }
    rcMarkWalkableTriangles(context, cfg.walkableSlopeAngle, GetVertices(), GetVertexCount(), GetTriangles(), GetTriCount(), triareas);
    if (!rcRasterizeTriangles(context, GetVertices(), GetVertexCount(), GetTriangles(), triareas, GetTriCount(), *m_heightField, cfg.walkableClimb)) {
        std::cout << "Triangle rasterization failed!\n";
    }
    rcFilterLowHangingWalkableObstacles(context, cfg.walkableClimb, *m_heightField);
    rcFilterLedgeSpans(context, cfg.walkableHeight, cfg.walkableClimb, *m_heightField);
    rcFilterWalkableLowHeightSpans(context, cfg.walkableHeight, *m_heightField);
    if (!rcBuildCompactHeightfield(context, cfg.walkableHeight, cfg.walkableClimb, *m_heightField, *m_compactHeightField)) {
        std::cout << "Compact Heightfield creation failed!\n";
    }
    if (!rcErodeWalkableArea(context, cfg.walkableRadius, *m_compactHeightField)) {
        std::cout << "Walkable area erosion failed!\n";
    }
    if (regionMode == NavMeshRegionMode::WATER_SHED) {
        if (!rcBuildDistanceField(context, *m_compactHeightField)) {
            std::cout << "Could not build distance field.\n";
        }
        if (!rcBuildRegions(context, *m_compactHeightField, 0, cfg.minRegionArea, cfg.mergeRegionArea)) {
            std::cout << "Could not build watershed regions.\n";
        }
    }
    else if (regionMode == NavMeshRegionMode::MONOTONE) {
        if (!rcBuildRegionsMonotone(context, *m_compactHeightField, 0, cfg.minRegionArea, cfg.mergeRegionArea)) {
            std::cout << "Build regions monotone failed!\n";
        }
    }
    if (!rcBuildContours(context, *m_compactHeightField, cfg.maxSimplificationError, cfg.maxEdgeLen, *m_contourSet)) {
        std::cout << "Build contours failed!\n";
    }
    if (!rcBuildPolyMesh(context, *m_contourSet, cfg.maxVertsPerPoly, *m_polyMesh)) {
        std::cout << "Build poly mesh failed!\n";
    }
    if (!rcBuildPolyMeshDetail(context, *m_polyMesh, *m_compactHeightField, cfg.detailSampleDist, cfg.detailSampleMaxError, *m_polyMeshDetail)) {
        std::cout << "Build poly mesh detail failed!\n";
    }
    // Mark walkable tiles
    for (int i = 0; i < m_polyMesh->npolys; i++) {
        m_polyMesh->flags[i] = 1;
    }
    /*
    const int tw = (cfg.width + cfg.tileSize - 1) / cfg.tileSize;
    const int th = (cfg.height + cfg.tileSize - 1) / cfg.tileSize;

    // Tile cache params.
    dtTileCacheParams tcparams;
    memset(&tcparams, 0, sizeof(tcparams));
    rcVcopy(tcparams.orig, m_polyMesh->bmin);
    tcparams.cs = cfg.ch;
    tcparams.ch = cfg.cs;
    tcparams.width = cfg.tileSize;
    tcparams.height = cfg.tileSize;
    tcparams.walkableHeight = cfg.walkableHeight * cfg.ch;
    tcparams.walkableRadius = cfg.walkableRadius * cfg.cs;
    tcparams.walkableClimb = cfg.walkableClimb * cfg.ch;
    tcparams.maxSimplificationError = cfg.maxSimplificationError;
    tcparams.maxTiles = tw * th * EXPECTED_LAYERS_PER_TILE;
    tcparams.maxObstacles = 128;
    m_tileCache->init(&tcparams, m_talloc, m_tcomp, m_tmproc);
    */

    // Detour
    dtNavMeshCreateParams params;
    memset(&params, 0, sizeof(params));
    params.verts = m_polyMesh->verts;
    params.vertCount = m_polyMesh->nverts;
    params.polys = m_polyMesh->polys;
    params.polyAreas = m_polyMesh->areas;
    params.polyFlags = m_polyMesh->flags;
    params.polyCount = m_polyMesh->npolys;
    params.nvp = m_polyMesh->nvp;
    params.detailMeshes = m_polyMeshDetail->meshes;
    params.detailVerts = m_polyMeshDetail->verts;
    params.detailVertsCount = m_polyMeshDetail->nverts;
    params.detailTris = m_polyMeshDetail->tris;
    params.detailTriCount = m_polyMeshDetail->ntris;
    rcVcopy(params.bmin, m_polyMesh->bmin);
    rcVcopy(params.bmax, m_polyMesh->bmax);
    params.cs = cfg.cs;
    params.ch = cfg.ch;
    params.buildBvTree = true;
    params.walkableHeight = 0.1f;
    params.walkableRadius = 0.05;
    params.walkableClimb = 0.1;
    unsigned char* navData = 0;
    int navDataSize = 0;
    if (!dtCreateNavMeshData(&params, &navData, &navDataSize)) {
        std::cout << "Build detour nav mesh failed!\n";
    }
    dtStatus status = m_navMesh->init(navData, navDataSize, DT_TILE_FREE_DATA);
    if (dtStatusFailed(status)) {
        std::cout << "Detour nav mesh init failed!\n";
    }
    status = m_navQuery->init(m_navMesh, 2048);
    if (dtStatusFailed(status)) {
        std::cout << "Detour nav mesh query failed!\n";
    }

    /*
    float aabbMin[3] = { -1, -1, -1 };
    float aabbMax[3] = { 1, 1, 1 };

    static dtObstacleRef obstacleRef;
    dtStatus status2 = m_tileCache->addBoxObstacle(aabbMin, aabbMax, &obstacleRef);
    if (dtStatusFailed(status2)) {
        std::cout << "addBoxObstacle() FAILED!\n";
    }
    else {
        std::cout << "O B S T A C L E ADDED!\n";
    }

    int count = m_tileCache->getObstacleCount();
    std::cout << "obstacle count: " << count << "\n";*/
}

struct RasterizationContext {

    RasterizationContext() : solid(0), triareas(0), lset(0), chf(0), ntiles(0) {
        memset(tiles, 0, sizeof(TileCacheData) * MAX_LAYERS);
    }

    ~RasterizationContext() {
        rcFreeHeightField(solid);
        delete[] triareas;
        rcFreeHeightfieldLayerSet(lset);
        rcFreeCompactHeightfield(chf);
        for (int i = 0; i < MAX_LAYERS; ++i)
        {
            dtFree(tiles[i].data);
            tiles[i].data = 0;
        }
    }

    rcHeightfield* solid;
    unsigned char* triareas;
    rcHeightfieldLayerSet* lset;
    rcCompactHeightfield* chf;
    TileCacheData tiles[MAX_LAYERS];
    int ntiles;
};

int NavMesh::rasterizeTileLayers(rcContext* context, std::vector<glm::vec3>& vertices, NavMeshRegionMode regionMode, const int tx, const int ty, const rcConfig& cfg, TileCacheData* tiles, const int maxTiles) {

    FastLZCompressor comp;
    RasterizationContext rc;

    //const float* verts = m_geom->getMesh()->getVerts();
    //const int nverts = m_geom->getMesh()->getVertCount();
    //const rcChunkyTriMesh* chunkyMesh = m_geom->getChunkyMesh();

    // Tile bounds.
    const float tcs = cfg.tileSize * cfg.cs;

    rcConfig tcfg;
    memcpy(&tcfg, &cfg, sizeof(tcfg));
    tcfg.bmin[0] = cfg.bmin[0] + tx * tcs;
    tcfg.bmin[1] = cfg.bmin[1];
    tcfg.bmin[2] = cfg.bmin[2] + ty * tcs;
    tcfg.bmax[0] = cfg.bmin[0] + (tx + 1) * tcs;
    tcfg.bmax[1] = cfg.bmax[1];
    tcfg.bmax[2] = cfg.bmin[2] + (ty + 1) * tcs;
    tcfg.bmin[0] -= tcfg.borderSize * tcfg.cs;
    tcfg.bmin[2] -= tcfg.borderSize * tcfg.cs;
    tcfg.bmax[0] += tcfg.borderSize * tcfg.cs;
    tcfg.bmax[2] += tcfg.borderSize * tcfg.cs;

    // Allocate voxel heightfield where we rasterize our input data to.
    rc.solid = rcAllocHeightfield();
    if (!rc.solid) {
        std::cout << "buildNavigation: Out of memory 'solid'.\n";
        return 0;
    }
    if (!rcCreateHeightfield(context, *rc.solid, tcfg.width, tcfg.height, tcfg.bmin, tcfg.bmax, tcfg.cs, tcfg.ch)) {
        std::cout << "buildNavigation: Could not create solid heightfield.\n";
        return 0;
    }

    // Allocate array that can hold triangle flags.
    // If you have multiple meshes you need to process, allocate
    // and array which can hold the max number of triangles you need to process.
    //rc.triareas = new unsigned char[chunkyMesh->maxTrisPerChunk];
    //if (!rc.triareas){
    //    std::cout << "buildNavigation: Out of memory 'm_triareas' (%d).", chunkyMesh->maxTrisPerChunk);
    //    return 0;
    //}
    unsigned char* triareas = new unsigned char[GetTriCount()];
    memset(triareas, 0, GetTriCount() * sizeof(unsigned char));

    /*
    float tbmin[2], tbmax[2];
    tbmin[0] = tcfg.bmin[0];
    tbmin[1] = tcfg.bmin[2];
    tbmax[0] = tcfg.bmax[0];
    tbmax[1] = tcfg.bmax[2];
    int cid[512];// TODO: Make grow when returning too many items.
    const int ncid = rcGetChunksOverlappingRect(chunkyMesh, tbmin, tbmax, cid, 512);
    if (!ncid) {
        return 0; // empty
    }

    for (int i = 0; i < ncid; ++i) {
        const rcChunkyTriMeshNode& node = chunkyMesh->nodes[cid[i]];
        const int* tris = &chunkyMesh->tris[node.i * 3];
        const int ntris = node.n;

        memset(rc.triareas, 0, ntris * sizeof(unsigned char));

        rcMarkWalkableTriangles(m_ctx, tcfg.walkableSlopeAngle, verts, nverts, tris, ntris, rc.triareas);
        if (!rcRasterizeTriangles(m_ctx, verts, nverts, tris, rc.triareas, ntris, *rc.solid, tcfg.walkableClimb)) {
            return 0;
        }
    }*/

    rcMarkWalkableTriangles(context, cfg.walkableSlopeAngle, GetVertices(), GetVertexCount(), GetTriangles(), GetTriCount(), triareas);
    if (!rcRasterizeTriangles(context, GetVertices(), GetVertexCount(), GetTriangles(), triareas, GetTriCount(), *m_heightField, cfg.walkableClimb)) {
        std::cout << "Triangle rasterization failed!\n";
    }


        // Once all geometry is rasterized, we do initial pass of filtering to
    // remove unwanted overhangs caused by the conservative rasterization
    // as well as filter spans where the character cannot possibly stand.
    //if (m_filterLowHangingObstacles)
        rcFilterLowHangingWalkableObstacles(context, tcfg.walkableClimb, *rc.solid);
    //if (m_filterLedgeSpans)
        rcFilterLedgeSpans(context, tcfg.walkableHeight, tcfg.walkableClimb, *rc.solid);
    //if (m_filterWalkableLowHeightSpans)
        rcFilterWalkableLowHeightSpans(context, tcfg.walkableHeight, *rc.solid);


    rc.chf = rcAllocCompactHeightfield();
    if (!rc.chf) {
        std::cout << "buildNavigation: Out of memory 'chf'.\n";
        return 0;
    }
    if (!rcBuildCompactHeightfield(context, tcfg.walkableHeight, tcfg.walkableClimb, *rc.solid, *rc.chf)) {
        std::cout << "buildNavigation: Could not build compact data.\n";
        return 0;
    }
    // Erode the walkable area by agent radius.
    if (!rcErodeWalkableArea(context, tcfg.walkableRadius, *rc.chf)) {
        std::cout << "buildNavigation: Could not erode.\n";
        return 0;
    }
    // (Optional) Mark areas.
    //const ConvexVolume* vols = m_geom->getConvexVolumes();
    //for (int i = 0; i < m_geom->getConvexVolumeCount(); ++i) {
    //    rcMarkConvexPolyArea(context, vols[i].verts, vols[i].nverts,
    //        vols[i].hmin, vols[i].hmax,
    //        (unsigned char)vols[i].area, *rc.chf);
    //}

    rc.lset = rcAllocHeightfieldLayerSet();
    if (!rc.lset) {
        std::cout << "buildNavigation: Out of memory 'lset'.\n";
        return 0;
    }
    if (!rcBuildHeightfieldLayers(context, *rc.chf, tcfg.borderSize, tcfg.walkableHeight, *rc.lset)) {
        std::cout << "buildNavigation: Could not build heighfield layers.\n";
        return 0;
    }

    rc.ntiles = 0;
    for (int i = 0; i < rcMin(rc.lset->nlayers, MAX_LAYERS); ++i) {
        TileCacheData* tile = &rc.tiles[rc.ntiles++];
        const rcHeightfieldLayer* layer = &rc.lset->layers[i];

        // Store header
        dtTileCacheLayerHeader header;
        header.magic = DT_TILECACHE_MAGIC;
        header.version = DT_TILECACHE_VERSION;

        // Tile layer location in the navmesh.
        header.tx = tx;
        header.ty = ty;
        header.tlayer = i;
        dtVcopy(header.bmin, layer->bmin);
        dtVcopy(header.bmax, layer->bmax);

        // Tile info.
        header.width = (unsigned char)layer->width;
        header.height = (unsigned char)layer->height;
        header.minx = (unsigned char)layer->minx;
        header.maxx = (unsigned char)layer->maxx;
        header.miny = (unsigned char)layer->miny;
        header.maxy = (unsigned char)layer->maxy;
        header.hmin = (unsigned short)layer->hmin;
        header.hmax = (unsigned short)layer->hmax;

        dtStatus status = dtBuildTileCacheLayer(&comp, &header, layer->heights, layer->areas, layer->cons, &tile->data, &tile->dataSize);
        if (dtStatusFailed(status)) {
            return 0;
        }
    }

    // Transfer ownership of tile data from build context to the caller.
    int n = 0;
    for (int i = 0; i < rcMin(rc.ntiles, maxTiles); ++i) {
        tiles[n++] = rc.tiles[i];
        rc.tiles[i].data = 0;
        rc.tiles[i].dataSize = 0;
    }

    return n;
}



std::vector<glm::vec3> NavMesh::FindPath(glm::vec3 startPos, glm::vec3 endPos) {
    std::vector<glm::vec3> finalPath;
    float spos[3] = { startPos.x, startPos.y, startPos.z };
    float epos[3] = { endPos.x, endPos.y, endPos.z };
    float nspos[3], nepos[3];
    const float polyPickExt[3] = { 1, 1, 1 };
    dtQueryFilter filter;
    filter.setIncludeFlags(1);
    filter.setExcludeFlags(0);
    dtPolyRef startRef, endRef;
    dtStatus status = m_navQuery->findNearestPoly(spos, polyPickExt, &filter, &startRef, nspos);

    if (dtStatusFailed(status) || !startRef) {
        //printf("Failed to find nearest poly for start position. Status: %u, startRef: %u\n", status, startRef);
        return finalPath;
    }
    status = m_navQuery->findNearestPoly(epos, polyPickExt, &filter, &endRef, nepos);
    if (dtStatusFailed(status) || !endRef) {
        //printf("Failed to find nearest poly for end position. Status: %u, endRef: %u\n", status, endRef);
        return finalPath;
    }


    // Allocate memory for the polys array
    dtPolyRef polys[MAX_POLYS];
    int npolys;
    // Find path
    status = m_navQuery->findPath(startRef, endRef, spos, epos, &filter, polys, &npolys, MAX_POLYS);
    if (dtStatusFailed(status)) {
        //std::cout << "findPath() failed with status: " << status << "\n";
    }
    else {
        // Allocate memory for straight path
        float straightPath[MAX_POLYS * 3];
        dtPolyRef straightPathPolys[MAX_POLYS];
        unsigned char straightPathFlags[MAX_POLYS];
        int nstraightPath;

        // Find straight path
        status = m_navQuery->findStraightPath(spos, epos, polys, npolys, straightPath, straightPathFlags, straightPathPolys, &nstraightPath, MAX_POLYS);
        if (dtStatusFailed(status)) {
            //printf("Failed to find straight path. Status: %u\n", status);
        }
        else {
            for (int i = 0; i < nstraightPath; ++i) {
                float* v = &straightPath[i * 3];
                finalPath.push_back({ v[0], v[1], v[2] });
            }
        }
    }
    return finalPath;
}

const float* NavMesh::GetBoundsMin() const {
    return m_boundsMin;
}

const float* NavMesh::GetBoundsMax() const {
    return m_boundsMax;
}

float* NavMesh::GetVertices() {
    if (!m_vertices.empty()) {
        return reinterpret_cast<float*>(m_vertices.data());
    }
    return nullptr;
}

int* NavMesh::GetTriangles() {
    if (!m_indices.empty()) {
        return reinterpret_cast<int*>(m_indices.data());
    }
    return nullptr;
}

int NavMesh::GetVertexCount() {
    return m_vertices.size();
}

int NavMesh::GetTriCount() {
    return m_indices.size() / 3;
}

float NavMesh::GetSizeX() {
    return m_boundsMax[0] - m_boundsMin[0];
}

float NavMesh::GetSizeY() {
    return m_boundsMax[1] - m_boundsMin[1];
}

float NavMesh::GetSizeZ() {
    return m_boundsMax[2] - m_boundsMin[2];
}

float NavMesh::GetBoundsMinX() {
    return m_boundsMin[0];
}

float NavMesh::GetBoundsMinY() {
    return m_boundsMin[1];
}

float NavMesh::GetBoundsMinZ() {
    return m_boundsMin[2];
}

float NavMesh::GetBoundsMaxX() {
    return m_boundsMax[0];
}

float NavMesh::GetBoundsMaxY() {
    return m_boundsMax[1];
}

float NavMesh::GetBoundsMaxZ() {
    return m_boundsMax[2];
}