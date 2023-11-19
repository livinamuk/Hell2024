#include "File.h"
#include "../Common.h"
#include "rapidjson/document.h"
#include <rapidjson/filereadstream.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/prettywriter.h>
#include <filesystem>
#include <sstream> 
#include <fstream>
#include <string>
#include <iostream>
#include "../Core/Scene.h"
#include "../Core/AssetManager.h"

namespace File {
	void SaveVec2(rapidjson::Value* object, std::string elementName, glm::vec2 vector, rapidjson::Document::AllocatorType& allocator);
	void SaveVec3(rapidjson::Value* object, std::string elementName, glm::vec3 vector, rapidjson::Document::AllocatorType& allocator);
	void SaveString(rapidjson::Value* object, std::string elementName, std::string string, rapidjson::Document::AllocatorType& allocator);
	void SaveBool(rapidjson::Value* object, std::string elementName, bool boolean, rapidjson::Document::AllocatorType& allocator);
	void SaveFloat(rapidjson::Value* object, std::string elementName, float number, rapidjson::Document::AllocatorType& allocator);
	void SaveInt(rapidjson::Value* object, std::string elementName, int number, rapidjson::Document::AllocatorType& allocator);
	glm::vec2 ReadVec2(const rapidjson::Value& value, std::string name);
	glm::vec3 ReadVec3(const rapidjson::Value& value, std::string name);
	std::string ReadString(const rapidjson::Value& value, std::string name);
	//char* ReadText(const rapidjson::Value& value, std::string name);
	bool ReadBool(const rapidjson::Value& value, std::string name);
	float ReadFloat(const rapidjson::Value& value, std::string name);
	int ReadInt(const rapidjson::Value& value, std::string name);
}

void File::LoadMap(std::string mapName) {

}

void File::SaveMap(std::string mapName) {

	rapidjson::Document document;
	rapidjson::Document::AllocatorType& allocator = document.GetAllocator();
	rapidjson::Value walls(rapidjson::kArrayType);
	rapidjson::Value lights(rapidjson::kArrayType);
	rapidjson::Value gameObjects(rapidjson::kArrayType);
	document.SetObject();

	for (Wall& wall : Scene::_walls) {
		rapidjson::Value object(rapidjson::kObjectType);
		SaveVec3(&object, "begin", wall.begin, allocator);
		SaveVec3(&object, "end", wall.begin, allocator);
		SaveFloat(&object, "height", wall.height, allocator);
		SaveBool(&object, "hasTopTrim", wall.height, allocator);
		SaveBool(&object, "hasBottomTrim", wall.height, allocator);
		SaveString(&object, "materialName", AssetManager::GetMaterialNameByIndex(wall.materialIndex), allocator);
		walls.PushBack(object, allocator);
	}

	document.AddMember("Walls", walls, allocator);

	rapidjson::StringBuffer strbuf;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(strbuf);
	document.Accept(writer);

	std::string data = strbuf.GetString();
	std::ofstream out("res/maps/" + mapName);
	out << data;
	out.close();

	std::cout << "Saved res/maps/" << mapName << "\n";
}

void File::SaveVec3(rapidjson::Value* object, std::string elementName, glm::vec3 vector, rapidjson::Document::AllocatorType& allocator) {
	rapidjson::Value array(rapidjson::kArrayType);
	rapidjson::Value name(elementName.c_str(), allocator);
	array.PushBack(rapidjson::Value().SetFloat(vector.x), allocator);
	array.PushBack(rapidjson::Value().SetFloat(vector.y), allocator);
	array.PushBack(rapidjson::Value().SetFloat(vector.z), allocator);
	object->AddMember(name, array, allocator);
}

void File::SaveVec2(rapidjson::Value* object, std::string elementName, glm::vec2 vector, rapidjson::Document::AllocatorType& allocator) {
	rapidjson::Value array(rapidjson::kArrayType);
	rapidjson::Value name(elementName.c_str(), allocator);
	array.PushBack(rapidjson::Value().SetFloat(vector.x), allocator);
	array.PushBack(rapidjson::Value().SetFloat(vector.y), allocator);
	object->AddMember(name, array, allocator);
}

void File::SaveString(rapidjson::Value* object, std::string elementName, std::string string, rapidjson::Document::AllocatorType& allocator) {
	rapidjson::Value name(elementName.c_str(), allocator);
	rapidjson::Value value(rapidjson::kObjectType);
	value.SetString(string.c_str(), static_cast<rapidjson::SizeType>(string.length()), allocator);
	object->AddMember(name, value, allocator);
}

void File::SaveBool(rapidjson::Value* object, std::string elementName, bool boolean, rapidjson::Document::AllocatorType& allocator) {
	rapidjson::Value name(elementName.c_str(), allocator);
	rapidjson::Value value(rapidjson::kObjectType);
	value.SetBool(boolean);
	object->AddMember(name, value, allocator);
}

void File::SaveFloat(rapidjson::Value* object, std::string elementName, float number, rapidjson::Document::AllocatorType& allocator) {
	rapidjson::Value name(elementName.c_str(), allocator);
	rapidjson::Value value(rapidjson::kObjectType);
	value.SetFloat(number);
	object->AddMember(name, value, allocator);
}

void File::SaveInt(rapidjson::Value* object, std::string elementName, int number, rapidjson::Document::AllocatorType& allocator) {
	rapidjson::Value name(elementName.c_str(), allocator);
	rapidjson::Value value(rapidjson::kObjectType);
	value.SetInt(number);
	object->AddMember(name, value, allocator);
}


glm::vec3 File::ReadVec3(const rapidjson::Value& value, std::string name) {
	glm::vec3 v = glm::vec3(0, 0, 0);
	if (value.HasMember(name.c_str())) {
		const rapidjson::Value& element = value[name.c_str()];
		v.x = element[0].GetFloat();
		v.y = element[1].GetFloat();
		v.z = element[2].GetFloat();
	}
	else
		std::cout << "'" << name << "' NOT FOUND IN MAP FILE\n";
	return v;
}

glm::vec2 File::ReadVec2(const rapidjson::Value& value, std::string name) {
	glm::vec2 v = glm::vec2(0, 0);
	if (value.HasMember(name.c_str())) {
		const rapidjson::Value& element = value[name.c_str()];
		v.x = element[0].GetFloat();
		v.y = element[1].GetFloat();
	}
	else
		std::cout << "'" << name << "' NOT FOUND IN MAP FILE\n";
	return v;
}

std::string File::ReadString(const rapidjson::Value& value, std::string name) {
	std::string s = "NOT FOUND";
	if (value.HasMember(name.c_str()))
		s = value[name.c_str()].GetString();
	else
		std::cout << "'" << name << "' NOT FOUND IN MAP FILE\n";
	return s;
}

/*char* File::ReadText(const rapidjson::Value& value, std::string name) {
	std::string s = "NOT FOUND";
	if (value.HasMember(name.c_str())) {
		s = value[name.c_str()].GetString();
		return s;
	}
	else
		std::cout << "'" << name << "' NOT FOUND IN MAP FILE\n";
	char* cstr = new char[s.length() + 1];
	strcpy_s(cstr, s.length() + 1, s.c_str());

	strcpy(cstr, s.c_str());
	// do stuff
	//delete[] cstr;		
	return cstr;
}*/

bool File::ReadBool(const rapidjson::Value& value, std::string name) {
	bool b = false;
	if (value.HasMember(name.c_str()))
		b = value[name.c_str()].GetBool();
	else
		std::cout << "'" << name << "' NOT FOUND IN MAP FILE\n";
	return b;
}

float File::ReadFloat(const rapidjson::Value& value, std::string name) {
	float f = -1;

	if (value.HasMember(name.c_str()))
		f = value[name.c_str()].GetFloat();
	else
		std::cout << "'" << name << "' NOT FOUND IN MAP FILE\n";
	return f;
}

int File::ReadInt(const rapidjson::Value& value, std::string name) {
	int f = -1;

	if (value.HasMember(name.c_str()))
		f = value[name.c_str()].GetInt();
	else
		std::cout << "'" << name << "' NOT FOUND IN MAP FILE\n";
	return f;
}
