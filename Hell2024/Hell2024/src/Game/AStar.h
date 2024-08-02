#pragma once
#include <vector>
#include <list>
#include "../Common.h"

#define ORTHOGONAL_COST 10
#define DIAGONAL_COST 14

struct Cell {
    int x, y;
    bool obstacle;
    int g = 99999;  // G cost: distance from starting node
    int h = -1;     // H cost: distance from end node. Aka the heuristic.
    int f = -1;     // F cost: g + f
    std::vector<Cell*> neighbours;
    Cell* parent = nullptr;
    int heapIndex = -1;
    float GetF(Cell* destination);
    float GetH(Cell* destination);
};

struct MinHeap {
    std::vector<Cell*> items;
    int currentItemCount = 0;
    void AllocateSpace(int size);
    void AddItem(Cell* item);
    void Update(Cell* cell);
    bool Contains(Cell* cell);
    bool IsEmpty();
    int Size();
    void Clear();
    void SortUp(Cell* item);
    void Swap(Cell* cellA, Cell* cellB);
    Cell* RemoveFirst();
    void SortDown(Cell* cell);
};

struct AStar {
    void InitSearch(std::vector<std::vector<bool>>& map, int startX, int startY, int destinationX, int destinationY);
    void FindPath();
    void FindSmoothPath();
    void ClearData();
    bool GridPathFound();
    bool SmoothPathFound();
    bool SearchInitilized();
    std::list<Cell*>& GetClosedList();
    std::vector<Cell*>& GetPath();
    MinHeap& GetOpenList();

    Cell* m_start;
    Cell* m_destination;
    Cell* m_current;
    MinHeap m_openList;
    std::vector<std::vector<Cell>> m_cells;
    std::list<Cell*> m_closedList;
    std::vector<Cell*> m_finalPath;
    std::vector<glm::vec2> m_finalPathPoints;

private:
    bool IsDestination(Cell* cell);
    void BuildFinalPath();
    void AddIfUnique(std::list<Cell*>* list, Cell* cell);
    bool IsOrthogonal(Cell* cellA, Cell* cellB);
    bool IsInClosedList(Cell* cell);
    void FindNeighbours(Cell* cell);

    int m_smoothSearchIndex = 0;
    bool m_gridPathFound = false;
    bool m_smoothPathFound = false;
    bool m_searchInitilized = false;
};