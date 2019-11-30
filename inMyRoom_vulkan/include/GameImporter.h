#pragma once

#include <string>

#include "configuru.hpp"

#include "ECS/ECStypes.h"
#include "ECS/GeneralComponents/PositionComp.h"
#include "ECS/GeneralComponents/NodeGlobalMatrixComp.h"
#include "ECS/GeneralComponents/ModelDrawComp.h"

#include "glm/gtx/matrix_decompose.hpp"

class Engine;       // Forward declaration

class GameImporter
{
public:
    GameImporter(Engine* in_engine_ptr, std::string gameConfig_path);

    Entity AddFabAndGetRoot(std::string fab_name, Entity parent_entity = 0, std::string preferred_name = "");
    Entity AddFabAndGetRoot(std::string fab_name, std::string parent_path , std::string preferred_name = "");

    Node* GetFabNode(std::string fab_node);
private:
    void ImportGame();
    void AddImports();
    void AddFabs();

    void InitializeGame();

    tinygltf::Model LoadModel(std::string path);

    std::unique_ptr<Node> ImportModel(std::string model_name, tinygltf::Model& this_model);
    std::unique_ptr<Node> ImportNode(tinygltf::Node& this_node, tinygltf::Model& this_model);

    void AddNodeToEntityHandler(Entity parent_entity, std::string parent_full_name, Node* this_node);           // fullname == "" means no name for the children D:
    void AddNodeComponentsToECS(Entity parent_entity, Node* this_node);
    
    static std::string GetFilePathFolder(const std::string& in_fileName);
    static std::string GetFilePathExtension(const std::string& in_fileName);

    static CompEntityInitMap CreatePositionInfo(const tinygltf::Node& in_node);

    std::string NumberToString(size_t number);

private:    // data
    size_t anonymousAddedFabsSoFar = 0;

    std::unordered_map<std::string, std::unique_ptr<Node>> imports_umap;
    std::unordered_map<std::string, std::unique_ptr<Node>> fabs_umap;

    configuru::Config gameConfig;
    const std::string folderName;
    Engine* const engine_ptr;
};

