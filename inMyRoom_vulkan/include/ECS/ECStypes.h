#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <set>

#include "glm/gtx/quaternion.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

typedef uint16_t Entity;
typedef uint32_t componentID;
typedef void* ComponentEntityPtr;
typedef void* DataSetPtr;

struct CompEntityInitMap
{
    std::unordered_map<std::string, glm::vec4> vec4Map;       // vec4_type
    std::unordered_map<std::string, float> floatMap;          // float_type
    std::unordered_map<std::string, int> intMap;              // int_type
    std::unordered_map<std::string, std::string> stringMap;   // string_type

    CompEntityInitMap()
    {}
    CompEntityInitMap(CompEntityInitMap const& other)
    {
        vec4Map = other.vec4Map;
        floatMap = other.floatMap;
        intMap = other.intMap;
        stringMap = other.stringMap;
    }
};

struct Node
{
    std::map<componentID, CompEntityInitMap> componentIDsToInitMaps;

    size_t glTFnodeIndex = -1;
    std::string nodeName = "";

    std::vector<std::unique_ptr<Node>> children;

    Node() {}
    Node(Node const& other)
    {
        componentIDsToInitMaps = other.componentIDsToInitMaps;

        glTFnodeIndex = other.glTFnodeIndex;
        nodeName = other.nodeName;

        for (size_t index = 0; index < other.children.size(); index++)
        {
            const Node* this_child_node_ptr = other.children[index].get();
            children.emplace_back(std::make_unique<Node>(*this_child_node_ptr));
        }
    }
};

struct FabInfo
{
    std::string fabName;
    size_t fabIndex;

    size_t size;
    std::vector<Entity> entitiesParents;

    std::unordered_map<std::string, Entity> nameToEntity;
    std::unordered_map<Entity, std::string> entityToName;

    std::vector<std::pair<class ComponentBaseClass*, std::pair<Entity, Entity>>> component_ranges;
};


struct InstanceInfo
{
    const FabInfo* fabInfo = nullptr;
    Entity entityOffset = 0;

    std::string instanceName;

    InstanceInfo* parent_instance = nullptr;
    std::set<InstanceInfo*> instanceChildren;
};

// used a lot in: Animation Actors
enum class InterpolationType
{
    Linear,
    Step,
    CubicSpline
};

struct AnimationData
{
    std::map<float, glm::vec3> timeToScaleKey_map;
    InterpolationType timeToScale_interpolation = InterpolationType::Linear;
    std::map<float, glm::qua<float>> timeToRotationKey_map;
    InterpolationType timeToRotation_interpolation = InterpolationType::Linear;
    std::map<float, glm::vec3> timeToTranslationKey_map;
    InterpolationType timeToTranslation_interpolation = InterpolationType::Linear;
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

// used a lot in: CollisionDetection
struct CollisionDetectionEntry
{
    glm::mat4x4 currentGlobalMatrix;
    glm::mat4x4 previousGlobalMatrix;
    const class OBBtree* OBBtree_ptr;
    bool shouldCallback;
    Entity entity;
};

struct CollisionCallbackData
{
    Entity familyEntity;
    Entity collideWithEntity;
    glm::vec3 deltaVector = glm::vec3(0.f, 0.f, 0.f);
};

// used a lot in: Drawing
struct DrawInfo
{
    size_t meshIndex = -1;
    size_t matricesOffset = -1;

    bool isSkin = false;
    size_t inverseMatricesOffset = -1;

    bool dontCull = false;
};