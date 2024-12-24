#pragma once
#include "nlohmann/json.hpp"
#include <fstream>
#include <string>
#include <iostream>
#include <glm/glm.hpp>

struct JSONObject {

    nlohmann::json data;

    void WriteString(std::string elementName, std::string value) {
        data[elementName] = value;
    }
    void WriteInt(std::string elementName, int value) {
        data[elementName] = value;
    }
    void WriteFloat(std::string elementName, float value) {
        data[elementName] = value;
    }
    void WriteArray(std::string elementName, std::vector<std::string> value) {
        data[elementName] = value;
    }
    void WriteFloatArray(std::string elementName, std::vector<float> array) {
        data[elementName] = array;
    }

    std::string GetJSONAsString() {
        int indent = 4;
        return data.dump(indent);
    }

    void SaveToFile(std::string filepath) {
        std::ofstream out(filepath);
        out << GetJSONAsString();
        out.close();
    }

    void test() {

        std::vector<int> arr = { 1, 2, 3 };

        nlohmann::json data;
        data["string"] = "hello cruel world";
        data["int"] = 666;
        data["float"] = 4.26;
        data["array"] = arr;
        data["bool"] = true;

        // Custom objects
        struct Object {
            glm::vec3 a;
            glm::vec3 b;
        };
        std::vector<Object> objects = {
            { {1.0f, 2.0f, 3.0f}, {0.0f, 0.0f, 0.0f} },
            { {4.0f, 5.0f, 6.0f}, {0.1f, 0.1f, 0.1f} }
        };
        nlohmann::json jsonObjects = nlohmann::json::array();
        for (const auto& object : objects) {
            nlohmann::json jsonObject;
            jsonObject["a"] = { {"x", object.a.x}, {"y", object.a.y}, {"z", object.a.z} };
            jsonObject["b"] = { {"x", object.b.x}, {"y", object.b.y}, {"z", object.b.z} };
            jsonObjects.push_back(jsonObject);
        }
        data["objects"] = jsonObjects;

        // Print it
        int indent = 4;
        std::string text = data.dump(indent);
        std::cout << text << "\n\n";

        // Save to file
        std::ofstream out("output.txt");
        out << text;
        out.close();

        // Load file
        std::ifstream file("output.txt");
        std::stringstream buffer;
        buffer << file.rdbuf();

        // Parse file
        nlohmann::json data2 = nlohmann::json::parse(buffer.str());

        // Read from it
        std::cout << "array: ";
        std::vector<int> arr2 = data2["array"];
        for (int i : arr2) {
            std::cout << i << " ";
        }
        int i = data2["int"];
        bool b = data2["bool"];
        float f = data2["float"];
        std::string s = data2["string"];
        std::cout << "\n";
        std::cout << "bool: " << b << "\n";
        std::cout << "int: " << i << "\n";
        std::cout << "float: " << f << "\n";
        std::cout << "string: " << s << "\n";

        // Read custom objects
        std::vector<Object> loadedObjects;
        for (const auto& jsonObject : data["objects"]) {
            Object object;
            object.a = { jsonObject["a"]["x"], jsonObject["a"]["y"], jsonObject["a"]["z"] };
            object.b = { jsonObject["b"]["x"], jsonObject["b"]["y"], jsonObject["b"]["z"] };
            loadedObjects.push_back(object);
        }
        for (const Object& object : loadedObjects) {
            std::cout << "a: (" << object.a.x << ", " << object.a.y << ", " << object.a.z << ") ";
            std::cout << "b: (" << object.b.x << ", " << object.b.y << ", " << object.b.z << ")\n";
        }

        // Error handling
        std::cout << "\n";
        try {
            int test = data2["doesntexist"];
        }
        catch (const nlohmann::json::exception& e) {
            std::cout << e.what() << '\n';
        }
        try {
            int test = data2["string"];
        }
        catch (const nlohmann::json::exception& e) {
            std::cout << e.what() << '\n';
        }
    }
};