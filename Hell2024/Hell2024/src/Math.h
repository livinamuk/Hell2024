#pragma once
#include <iostream>

struct vec3 {
    float x = 0;
    float y = 0;
    float z = 0;
    vec3() {};
    vec3(float x, float y, float z) {
        this->x = x;
        this->y = y;
        this->z = z;
    }
    vec3 operator+(const vec3& other) const {
        return vec3(x + other.x, y + other.y, z + other.z);
    }
    vec3 operator+=(const vec3& other) const {
        return vec3(x + other.x, y + other.y, z + other.z);
    }
    vec3 operator-(const vec3& other) const {
        return vec3(x - other.x, y - other.y, z - other.z);
    }
    bool operator==(const vec3& other) const {
        return (x == x && y == other.y && z == other.z);
    }
    bool operator!=(const vec3& other) const {
        return !(x == x && y == other.y && z == other.z);
    }
};

struct vec3i {
    int x = 0;
    int y = 0;
    int z = 0;
    vec3i() {};
    vec3i(int x, int y, int z) {
        this->x = x;
        this->y = y;
        this->z = z;
    }
    vec3i operator+(const vec3i& other) const {
        return vec3i(x + other.x, y + other.y, z + other.z);
    }
    vec3i operator+=(const vec3i& other) const {
        return vec3i(x + other.x, y + other.y, z + other.z);
    }
    vec3i operator-(const vec3i& other) const {
        return vec3i(x - other.x, y - other.y, z - other.z);
    }
    bool operator==(const vec3i& other) const {
        return (x == x && y == other.y && z == other.z);
    }
    bool operator!=(const vec3i& other) const {
        return !(x == x && y == other.y && z == other.z);
    }
};

inline  std::ostream& operator<<(std::ostream& os, vec3i obj) {
    os << '(' << obj.x << ", " << obj.y << ", " << obj.z << ')';
    return os;
};