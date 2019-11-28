#pragma once

#include <memory>
#include <string>
#include <map>

#include "glm/vec4.hpp"

typedef uint32_t Entity;
typedef uint32_t componentID;
typedef void* ComponentEntityPtr;

struct ComponentEntityInitMap
{
    std::map<std::string, glm::vec4> vec4Map;
    std::map<std::string, float> floatMap;
    std::map<std::string, int> intMap;
    std::map<std::string, std::string> stringMap;
    std::map<std::string, Entity> entityMap;
};
