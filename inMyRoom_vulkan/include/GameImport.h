#pragma once

#include <string>
#include <algorithm>
#include <cassert>
#include <iterator>

#include "configuru.hpp"

#include "ECS/GeneralComponents/PositionComp.h"
#include "ECS/GeneralComponents/NodeGlobalMatrixComp.h"
#include "ECS/GeneralComponents/ModelDrawComp.h"

#include "glm/gtx/matrix_decompose.hpp"

struct Node
{
    PositionCompEntity positionEntity = PositionCompEntity::GetEmpty(); // entity stays unitialiazed
    uint32_t meshIndex = -1;

    Entity latestEntity = 0;                // for fabs->Entities
    std::string nodeName = "";
    std::vector<std::unique_ptr<Node>> children;

    Node()
    {
    }
    Node(Node const& other) 
    {
        positionEntity = other.positionEntity;
        meshIndex = other.meshIndex;
        latestEntity = other.latestEntity;
        nodeName = other.nodeName;
        for (size_t index = 0; index < other.children.size(); index++)
        {
            const Node* this_child_node_ptr = other.children[index].get();
            children.emplace_back(std::make_unique<Node>(*this_child_node_ptr));
        }
    }
};

class Engine;       // Forward declaration

class GameImport
{
public:
    GameImport(Engine* in_engine_ptr, std::string gameConfig_path);

    Entity AddFabAndGetRoot(std::string prefab, Entity parent_entity = 0, std::string preferred_name = "");
    Entity AddFabAndGetRoot(std::string prefab, std::string parent_path , std::string preferred_name = "");
private:
    void ImportGame();
    void InitializeGame();

    tinygltf::Model LoadModel(std::string path);

    std::unique_ptr<Node> ImportModel(std::string model_name, tinygltf::Model& this_model);
    std::unique_ptr<Node> ImportNode(tinygltf::Node& this_node, tinygltf::Model& this_model);

    void AddNodeToEntityHandler(Entity parent_entity, Node* this_node);
    void AddNodeComponentsToECS(Entity parent_entity, Node* this_node);
    
    static std::string GetFilePathFolder(const std::string& in_fileName);
    static std::string GetFilePathExtension(const std::string& in_fileName);

    static PositionCompEntity CreatePositionInfo(const tinygltf::Node& in_node);

    std::string NumberToString(size_t number);

private:    // data
    size_t anonymousAddedFabsSoFar = 0;

    std::unordered_map<std::string, std::unique_ptr<Node>> imports_umap;
    std::unordered_map<std::string, std::unique_ptr<Node>> fabs_umap;

    configuru::Config gameConfig;
    const std::string folderName;
    Engine* const engine_ptr;
};

