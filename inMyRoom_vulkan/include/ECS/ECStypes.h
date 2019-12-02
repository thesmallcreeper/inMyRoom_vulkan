#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>

#include "glm/vec4.hpp"

typedef uint32_t Entity;
typedef uint32_t componentID;
typedef void* ComponentEntityPtr;

struct CompEntityInitMap
{
    std::map<std::string, glm::vec4> vec4Map;
    std::map<std::string, float> floatMap;
    std::map<std::string, int> intMap;
    std::map<std::string, std::string> stringMap;
    std::map<std::string, Entity> entityMap;

    CompEntityInitMap()
    {}
    CompEntityInitMap(CompEntityInitMap const& other)
    {
        vec4Map = other.vec4Map;
        floatMap = other.floatMap;
        intMap = other.intMap;
        stringMap = other.stringMap;
        entityMap = other.entityMap;
    }
};

// used a lot in: Game Import

struct Node
{
    std::vector<std::pair<componentID, CompEntityInitMap>> componentsAndInitMaps;
    bool shouldAddNodeGlobalMatrixCompEntity = false;

    Entity latestEntity = 0;                // for fabs->Entities
    uint32_t glTFnodeID = -1;
    std::string nodeName = "";

    std::vector<std::unique_ptr<Node>> children;

    Node()
    {}
    Node(Node const& other)
    {
        componentsAndInitMaps = other.componentsAndInitMaps;
        shouldAddNodeGlobalMatrixCompEntity = other.shouldAddNodeGlobalMatrixCompEntity;

        latestEntity = other.latestEntity;
        glTFnodeID = other.glTFnodeID;
        nodeName = other.nodeName;
        for (size_t index = 0; index < other.children.size(); index++)
        {
            const Node* this_child_node_ptr = other.children[index].get();
            children.emplace_back(std::make_unique<Node>(*this_child_node_ptr));
        }
    }
};

// used a lot in: InputManager

enum class InputType
{
    MouseMove,

    MoveForward,
    MoveBackward,
    MoveRight,
    MoveLeft,
    MoveUp,
    MoveDown,

    StopMovingForward,
    StopMovingBackward,
    StopMovingRight,
    StopMovingLeft,
    StopMovingUp,
    StopMovingDown
};

struct InputMouse
{
    float x_axis;
    float y_axis;
};