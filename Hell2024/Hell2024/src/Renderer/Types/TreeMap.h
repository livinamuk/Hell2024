#pragma once
#include <string>
#include <vector>

struct TreeMap {
    int m_width = 0;
    int m_depth = 0;
    std::vector<int> m_data;

    std::vector<std::vector<int>> m_array;

    void Load(const std::string& filepath);
};