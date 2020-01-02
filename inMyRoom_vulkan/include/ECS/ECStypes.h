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
    std::map<std::string, glm::vec4> vec4Map;       // vec4_type
    std::map<std::string, float> floatMap;          // float_type
    std::map<std::string, int> intMap;              // int_type
    std::map<std::string, std::string> stringMap;   // string_type
    std::map<std::string, Entity> entityMap;        // entity_type

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

// used a lot in: Animation Actors
enum class InterpolationType
{
    Linear,
    Step,
    CubicSpline
};

// used a lot in: Materials

enum class MapType
{
    vec4_type,
    vec3_type,
    vec2_type,
    float_type,
    int_type,
    string_type,
    entity_type,
    bool_type
};

// used a lot in: Game Import

struct Node
{
    std::map<componentID, CompEntityInitMap> componentIDsToInitMaps;
    bool shouldAddNodeGlobalMatrixCompEntity = false;

    Entity latestEntity = 0;                // for fabs->Entities
    size_t glTFnodeIndex = -1;
    std::string nodeName = "";

    std::vector<std::unique_ptr<Node>> children;

    Node()
    {}
    Node(Node const& other)
    {
        componentIDsToInitMaps = other.componentIDsToInitMaps;
        shouldAddNodeGlobalMatrixCompEntity = other.shouldAddNodeGlobalMatrixCompEntity;

        latestEntity = other.latestEntity;
        glTFnodeIndex = other.glTFnodeIndex;
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