#pragma once
#include "TreeMap.h"
#include <stb_image.h>
#include <iostream>

void TreeMap::Load(const std::string& filepath) {
    int channels;
    unsigned char* data = stbi_load(filepath.c_str(), &m_width, &m_depth, &channels, 0);
    if (!data) {
        std::cout << "Failed to load treemap image\n";
        return;
    }
    m_data.reserve(m_width * m_depth);
    m_array = std::vector<std::vector<int>>(m_width,std::vector<int>(m_depth));

    for (int z = 0; z < m_depth; z ++) {
        for (int x = 0; x < m_width; x++) { 
            int index = (z * m_width + x) * channels;
            unsigned char value = data[index];
            float heightValue = static_cast<float>(value) / 255.0f;
            int finalValue = (heightValue > 0.1) ? 1 : 0;
            m_array[x][z] = finalValue;
            //m_data.push_back(finalValue);
        }
    }
    stbi_image_free(data);
    //std::cout << "Treemap loaded: " << m_data.size() << " values\n";
}