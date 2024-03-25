#pragma once
#include <string>
#include <vector>

struct Model {

    void AddMeshIndex(uint32_t index) {
        meshIndices.push_back(index);
    }
    std::vector<uint32_t>& GetMeshIndices() {
        return meshIndices;
    }
    void SetName(std::string name) {
        this->name = name;
    }    
    const std::string& GetName() {
        return name;
    }
private:
    std::string name = "undefined";
    std::vector<uint32_t> meshIndices;
};
