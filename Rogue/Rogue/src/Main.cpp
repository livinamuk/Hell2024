
#include "Engine.h"
#include "common.h"
#include "Util.hpp"

int main() {

   /* struct CPoint {
        glm::vec3 position;
    };

    float _mapWidth = 16 / 2;
    float _mapHeight = 8 / 2;
    float _mapDepth = 16 / 2;

    std::vector<int> _dirtyPointCloudIndices;
    std::vector<glm::uvec4> _dirtyGridChunks;
    std::vector<CPoint> _cloudPoints;

    float _propogationGridSpacing = 0.4f;
    float _maxPropogationDistance = 2.6f;

    // Find which probe co-orindates are within range of cloud points that just changed    
    int textureWidth = _mapWidth / _propogationGridSpacing;
    int textureHeight = _mapHeight / _propogationGridSpacing;
    int textureDepth = _mapDepth / _propogationGridSpacing;

    float texelsPerChunk = 4.0f;
    int xChunksCount = textureWidth / texelsPerChunk;
    int yChunksCount = textureHeight / texelsPerChunk;
    int zChunksCount = textureDepth / texelsPerChunk;

    _dirtyGridChunks.clear();
    _dirtyGridChunks.reserve(xChunksCount * yChunksCount * zChunksCount);

    glm::vec3 chunkHalfSize = glm::vec3(texelsPerChunk * _propogationGridSpacing * 0.5f);
    float adjustedDistance = _maxPropogationDistance + chunkHalfSize.x; // your searching from the center of a chunk, so you add the half chunk distance
    float maxDistSquared = adjustedDistance * adjustedDistance;

    CPoint p, p2, p3, p4, p5;
    p.position = { 1, 1, 1 };
    p2.position = { 1.4, 0, 0 };
    p3.position = { 1.8, 0, 0 };
    p4.position = { 2.2, 0, 0 };
    p5.position = { 2.6, 0, 0 };
    _cloudPoints.push_back(p);
    _cloudPoints.push_back(p2);
    _cloudPoints.push_back(p3);
    _cloudPoints.push_back(p4);
    _cloudPoints.push_back(p5);

    _dirtyPointCloudIndices.push_back(0);
    _dirtyPointCloudIndices.push_back(1);
    _dirtyPointCloudIndices.push_back(2);
    _dirtyPointCloudIndices.push_back(3);
    _dirtyPointCloudIndices.push_back(4);

    for (int x = 0; x < xChunksCount; x++) {
        for (int y = 0; y < yChunksCount; y++) {
            for (int z = 0; z < zChunksCount; z++) {
                glm::vec3 chunkCenter = (glm::vec3(x, y, z) * texelsPerChunk * _propogationGridSpacing) + chunkHalfSize;                   
                for (int& index : _dirtyPointCloudIndices) {
                    glm::vec3 pointPosition = _cloudPoints[index].position;       
                    if (Util::DistanceSquared(pointPosition, chunkCenter) < maxDistSquared) {
                        _dirtyGridChunks.push_back(glm::uvec4(x, y, z, 0));
                        break;
                    }
                }
            }
        }
    }

    std::cout << "\chunkHalfSize: " << Util::Vec3ToString(chunkHalfSize) << "\n";

    std::cout << "\n_xChunksCount:    " << xChunksCount << "\n";
    std::cout << "yChunksCount:    " << yChunksCount << "\n";
    std::cout << "zChunksCount:    " << zChunksCount << "\n";
    std::cout << "\n_mapWidth:     " << _mapWidth << "\n";
    std::cout << "_mapHeight:    " << _mapHeight << "\n";
    std::cout << "_mapDepth:     " << _mapDepth << "\n";
    std::cout << "\ntextureWidth:  " << textureWidth << "\n";
    std::cout << "textureHeight: " << textureHeight << "\n";
    std::cout << "textureDepth:  " << textureDepth << "\n";    int texelCount = textureWidth * textureHeight * textureDepth;
    std::cout << "texelCount:    " << texelCount << "\n";


    std::cout << "\n_dirtyPointCloudIndices.size(): " << _dirtyPointCloudIndices.size() << "\n";
    std::cout << "_dirtyGridChunks.size(): " << _dirtyGridChunks.size() << "\n";
    int iterationCount = xChunksCount * yChunksCount * zChunksCount;
    std::cout << "total iterations over 3d tex: " << iterationCount << "\n";


    std::cout << "\n";

    for (int i = 0; i < _dirtyGridChunks.size(); i++) {
        std::cout << i << ": " << _dirtyGridChunks[i].x << ", " << _dirtyGridChunks[i].y << ", " << _dirtyGridChunks[i].z << "\n";
    }

	return 0;*/

   Engine::Run();
   return 0;
}


