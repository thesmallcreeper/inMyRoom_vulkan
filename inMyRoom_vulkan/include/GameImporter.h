#pragma once

#include <string>

#include "configuru.hpp"

#include "ECS/ECStypes.h"
#include "ECS/GeneralComponents/AnimationActorComp.h"
#include "ECS/GeneralComponents/NodeDataComp.h"
#include "ECS/GeneralComponents/EarlyNodeGlobalMatrixComp.h"
#include "ECS/GeneralComponents/LateNodeGlobalMatrixComp.h"
#include "ECS/GeneralComponents/ModelDrawComp.h"
#include "ECS/GeneralComponents/DynamicMeshComp.h"

#include "GameDLLimporter.h"


class Engine;       // Forward declaration

class GameImporter
{
public:
    GameImporter(Engine* in_engine_ptr, std::string gameConfig_path);

private:
    void ImportGame();

    void AddGameDLLcomponents();
    void AddEmptyNode();
    void AddImports();
    void AddDefaultCameraFab();
    void AddFabs();

    void InitializeGame();
    void InitOneDefaultCameraAndBindIt();

    void AddTweaksToNode(Node* node, const configuru::Config& compoents_properties);

    tinygltf::Model LoadglTFmodel(std::string path);

    std::unique_ptr<Node> ImportModel(std::string model_name, tinygltf::Model& this_model);

    std::unique_ptr<Node> ImportNodeExistance(tinygltf::Node& this_gltf_node, tinygltf::Model& model);
    std::unique_ptr<Node> ImportModelAnimationComposerAsNodes(Node* root_node, tinygltf::Model& model);
    void ImportNodeComponents(Node* this_node, Node* root_node, tinygltf::Model& this_model);

    Node* GetImportNode(std::string import_node_path);
    Node* GetFabNode(std::string fab_node_path);

    static std::string GetFilePathFolder(const std::string& in_fileName);
    static std::string GetFilePathExtension(const std::string& in_fileName);

    static CompEntityInitMap CreatePositionInitMap(const tinygltf::Node& in_node);
    static std::vector<float> GetglTFAccessorFloat(const tinygltf::Accessor& in_animationSampler, const tinygltf::Model& this_model);

    static Node* FindNodeInTree(Node* root_node, std::string path);

    static std::string GetPathUsingGLTFindex(Node* root_node, size_t glTF_node_Index);
    static std::string GetRelativePath(std::string path, std::string relative_to);

    std::string NumberToString(size_t number);

private:    // data
    size_t anonymousNameCounter = 1;

    std::unordered_map<std::string, std::unique_ptr<Node>> imports_umap;
    std::unordered_map<std::string, std::unique_ptr<Node>> fabs_umap;

    configuru::Config gameConfig;
    const std::string folderName;

    std::unique_ptr<GameDLLimporter> gameDLLimporter_uptr;

    Engine* const engine_ptr;
};

