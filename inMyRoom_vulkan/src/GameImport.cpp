#include "GameImport.h"
#include "Engine.h"

#include <algorithm>
#include <cassert>

#include "tiny_gltf.h"
#include "glm/gtc/type_ptr.hpp"

GameImport::GameImport(Engine* in_engine_ptr, std::string gameConfig_path)
    :folderName(GetFilePathFolder(gameConfig_path)),
     engine_ptr(in_engine_ptr)
{
    #ifdef _WIN32       // cause win32 is a moving cancer
    for (auto& this_char : gameConfig_path)
        if (this_char == '/')
            this_char = '\\';
    #endif

    gameConfig = configuru::parse_file(gameConfig_path, configuru::CFG);

    printf("Importing game\n");
    ImportGame();
    printf("Initializing game\n");
    InitializeGame();
}

void GameImport::ImportGame()
{
    {   // imports
        std::vector<tinygltf::Model> models;
        std::vector<std::string> models_name;
        std::vector<std::string> models_folder;

        for (const configuru::Config& arrayIterator : gameConfig["imports"]["toImport"].as_array())
        {
            std::string this_file_to_import = folderName + "/" + gameConfig["imports"][arrayIterator.as_string()].as_string();

            models.emplace_back(LoadModel(this_file_to_import));
            models_name.emplace_back(arrayIterator.as_string());
            models_folder.emplace_back(GetFilePathFolder(this_file_to_import));
        }

        for (size_t index = 0; index < models.size(); index++)
            engine_ptr->GetGraphicsPtr()->LoadModel(models[index], models_folder[index]);

        engine_ptr->GetGraphicsPtr()->EndModelsLoad();

        for (size_t index = 0; index < models.size(); index++)
        {
            std::unique_ptr<Node> this_model_node = ImportModel(models_name[index], models[index]);

            imports_umap.emplace(models_name[index], std::move(this_model_node));
        }
    }

    {   // fabs
        for (const configuru::Config& arrayIterator : gameConfig["fabs"]["toFab"].as_array())
        {
            std::string this_fab_name = arrayIterator.as_string();
            std::string this_fab_import_path = gameConfig["fabs"][this_fab_name]["basedOn"].as_string();

            auto search = imports_umap.find(this_fab_import_path.substr(0, this_fab_import_path.find_first_of("/")));
            assert(search != imports_umap.end());

            const Node* this_import_ptr = search->second.get();
            std::string rest_of_path = this_fab_import_path.substr(this_fab_import_path.find_first_of("/") + 1);

            const Node* this_node = this_import_ptr;
            while (rest_of_path.substr(0, rest_of_path.find_first_of("/")) != "")
            {
                std::string child_name = rest_of_path.substr(0, rest_of_path.find_first_of("/"));

                auto search = std::find_if(this_node->children.begin(), this_node->children.end(), [child_name](const auto& child_uptr_ref) {return child_uptr_ref->nodeName == child_name; });
                assert(search != this_node->children.end());

                this_node = search->get();

                if (rest_of_path.find_first_of("/") != std::string::npos)
                    rest_of_path = rest_of_path.substr(rest_of_path.find_first_of("/") + 1);
                else
                    rest_of_path = "";
            }
            // now this_node points to the base of the fab

            std::unique_ptr<Node> this_fab = std::make_unique<Node>(*this_node);
            this_fab->nodeName = this_fab_name;

            // TODO

            fabs_umap.emplace(this_fab_name, std::move(this_fab));
        }
    }
}

std::unique_ptr<Node> GameImport::ImportModel(std::string model_name, tinygltf::Model& this_model)
{
    std::unique_ptr<Node> model_node = std::make_unique<Node>();
    model_node->nodeName = model_name;

    for (tinygltf::Scene& this_gltf_scene : this_model.scenes)
    {
        std::unique_ptr<Node> scene_node = std::make_unique<Node>();;

        scene_node->nodeName = this_gltf_scene.name;
        assert(scene_node->nodeName != "");

        // Add position comp entity
        {
            CompEntityInitMap this_map;
            scene_node->componentsAndInitMaps.emplace_back(static_cast<componentID>(componentIDenum::Position), this_map);
        }
        scene_node->shouldAddNodeGlobalMatrixCompEntity = true;

        for (size_t this_gltf_scene_node_index : this_gltf_scene.nodes)
        {
            tinygltf::Node& this_gltf_scene_node = this_model.nodes[this_gltf_scene_node_index];
            scene_node->children.emplace_back(ImportNode(this_gltf_scene_node, this_model));
        }
        
        model_node->children.emplace_back(std::move(scene_node));
    }

    return std::move(model_node);
}

std::unique_ptr<Node> GameImport::ImportNode(tinygltf::Node& this_gltf_node, tinygltf::Model& this_model)
{
    std::unique_ptr<Node> return_node_uptr = std::make_unique<Node>();

    return_node_uptr->nodeName = this_gltf_node.name;

    // Position component
    {
        return_node_uptr->componentsAndInitMaps.emplace_back(std::make_pair(static_cast<componentID>(componentIDenum::Position), CreatePositionInfo(this_gltf_node)));
    }

    // Model draw component
    if (this_gltf_node.mesh != -1)
    {
        CompEntityInitMap this_map;
        this_map.intMap["MeshIndex"] = this_gltf_node.mesh + static_cast<int>(engine_ptr->GetGraphicsPtr()->GetMeshesOfNodesPtr()->GetMeshIndexOffsetOfModel(this_model));

        return_node_uptr->componentsAndInitMaps.emplace_back(std::make_pair(static_cast<componentID>(componentIDenum::ModelDraw), this_map));
    }

    // NodeGlobalMatrixCompEntity is added on the spot (later)
    return_node_uptr->shouldAddNodeGlobalMatrixCompEntity = true;

    if (this_gltf_node.children.size())
    {
        for (size_t this_gltf_recurse_node_index : this_gltf_node.children)
        {
            tinygltf::Node& this_gltf_recurse_node = this_model.nodes[this_gltf_recurse_node_index];
            return_node_uptr->children.emplace_back(ImportNode(this_gltf_recurse_node, this_model));
        }
    }

    return std::move(return_node_uptr);
}

tinygltf::Model GameImport::LoadModel(std::string path)
{
    tinygltf::Model model;

    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    std::string ext = GetFilePathExtension(path);

    bool ret = false;
    if (ext.compare(".glb") == 0) {
        // assume binary glTF.
        ret = loader.LoadBinaryFromFile(&model, &err, &warn, path.c_str());
    }
    else {
        // assume ascii glTF.
        ret = loader.LoadASCIIFromFile(&model, &err, &warn, path.c_str());
    }

    if (!warn.empty()) {
        printf("Warn: %s\n", warn.c_str());
    }

    if (!err.empty()) {
        printf("ERR: %s\n", err.c_str());
    }
    if (!ret) {
        printf("Failed to load .glTF : %s\n", path.c_str());
        exit(-1);
    }

    return std::move(model);
}

std::string GameImport::GetFilePathExtension(const std::string& in_filePath)
{
    if (in_filePath.find_last_of(".") != std::string::npos)
        return in_filePath.substr(in_filePath.find_last_of("."));
    return "";
}

std::string GameImport::GetFilePathFolder(const std::string& in_filePath)
{
    if (in_filePath.find_last_of("/") != std::string::npos)
        return in_filePath.substr(0, in_filePath.find_last_of("/"));
    else
        return "";
}

CompEntityInitMap GameImport::CreatePositionInfo(const tinygltf::Node& in_node)
{
    PositionCompEntity targeted_positionCompEntity = PositionCompEntity::GetEmpty();

    if (in_node.matrix.size() == 16)
    {
        std::array<float, 16>  matrix_data;
        for (size_t index = 0; index < matrix_data.size(); index++)
            matrix_data[index] = static_cast<float>(in_node.matrix[index]);

        glm::mat4 TRSmatrix =    glm::mat4(1.f, 0.f, 0.f, 0.f,
                                           0.f, -1.f, 0.f, 0.f,
                                           0.f, 0.f, -1.f, 0.f,
                                           0.f, 0.f, 0.f, 1.f)
                               * glm::make_mat4<float>(matrix_data.data())
                               * glm::mat4(1.f, 0.f, 0.f, 0.f,
                                           0.f, -1.f, 0.f, 0.f,
                                           0.f, 0.f, -1.f, 0.f,
                                           0.f, 0.f, 0.f, 1.f);

        glm::vec3 skew_temp;
        glm::vec4 perspective_temp;
        glm::decompose<float, glm::qualifier::packed_highp>(TRSmatrix, targeted_positionCompEntity.localScale, targeted_positionCompEntity.localRotation, targeted_positionCompEntity.localTranslation, skew_temp, perspective_temp);
    }
    else
    {
        glm::mat4 return_matrix = glm::mat4(1.0f);
        if (in_node.scale.size() == 3)
        {
            targeted_positionCompEntity.localScale = glm::vec3(in_node.scale[0], in_node.scale[1], in_node.scale[2]);
        }
        if (in_node.rotation.size() == 4)
        {
            targeted_positionCompEntity.localRotation = glm::qua<float>(static_cast<float>(in_node.rotation[3]), static_cast<float>(in_node.rotation[0]), static_cast<float>(-in_node.rotation[1]), static_cast<float>(-in_node.rotation[2]));
        }
        if (in_node.translation.size() == 3)
        {
            targeted_positionCompEntity.localTranslation = glm::vec3(in_node.translation[0], -in_node.translation[1], -in_node.translation[2]); 
        }
    }

    CompEntityInitMap return_compEntity;
    return_compEntity.vec4Map["LocalScale"] = glm::vec4(targeted_positionCompEntity.localScale, 1.0);
    return_compEntity.vec4Map["LocalRotation"] = glm::vec4(targeted_positionCompEntity.localRotation.x, targeted_positionCompEntity.localRotation.y, 
                                                           targeted_positionCompEntity.localRotation.z, targeted_positionCompEntity.localRotation.w);
    return_compEntity.vec4Map["LocalTranslation"] = glm::vec4(targeted_positionCompEntity.localTranslation, 1.0);

    return return_compEntity;
}

void GameImport::InitializeGame()
{
    for (const configuru::Config& arrayIterator : gameConfig["init"]["toInit"].as_array())
    {
        std::string this_init_name = arrayIterator.as_string();
        std::string this_init_fab_using = gameConfig["init"][this_init_name].as_string();

        AddFabAndGetRoot(this_init_fab_using, 0, this_init_name);
    }
}

Entity GameImport::AddFabAndGetRoot(std::string prefab, std::string parent_path, std::string preferred_name)
{
    Entity parent_entity = engine_ptr->GetECSwrapperPtr()->GetEntitiesHandler()->FindEntityByName(parent_path);
    return AddFabAndGetRoot(prefab, parent_entity, preferred_name);
}

Entity GameImport::AddFabAndGetRoot(std::string fab_name, Entity parent_entity, std::string preferred_name)
{
    if (preferred_name == "")
        preferred_name = fab_name + "_" + NumberToString(anonymousAddedFabsSoFar++);

    auto search = fabs_umap.find(fab_name);
    assert(search != fabs_umap.end());

    Node* this_root_node = search->second.get();
    this_root_node->nodeName = preferred_name;          // Will be used as a variable to pass down the preferred name;

    std::string parent_full_name;
    if (parent_entity != 0)
        parent_full_name = engine_ptr->GetECSwrapperPtr()->GetEntitiesHandler()->GetEntityName(parent_entity);
    else
        parent_full_name = "_root";

    AddNodeToEntityHandler(parent_entity, parent_full_name, this_root_node);

    AddNodeComponentsToECS(parent_entity, this_root_node);

    engine_ptr->GetECSwrapperPtr()->CompleteAddsAndRemoves();

    return this_root_node->latestEntity;
}

void GameImport::AddNodeToEntityHandler(Entity parent_entity, std::string parent_full_name, Node* this_node)
{
    // TODO naming

    Entity this_entity;
    if (parent_entity != 0)
        this_entity = engine_ptr->GetECSwrapperPtr()->GetEntitiesHandler()->CreateEntityWithParent(parent_entity);
    else
        this_entity = engine_ptr->GetECSwrapperPtr()->GetEntitiesHandler()->CreateEntity();

    std::string this_full_name = "";
    if (parent_full_name.size() && this_node->nodeName != "")
    {
        if (parent_full_name == "_root")
            this_full_name = this_node->nodeName;
        else
            this_full_name = parent_full_name + "/" + this_node->nodeName;

        engine_ptr->GetECSwrapperPtr()->GetEntitiesHandler()->AddEntityName(this_entity, this_full_name);
    }

    this_node->latestEntity = this_entity;

    for (size_t index = 0; index < this_node->children.size(); index++)
        AddNodeToEntityHandler(this_entity, this_full_name, this_node->children[index].get());
}

void GameImport::AddNodeComponentsToECS(Entity parent_entity, Node* this_node)
{
    Entity this_entity = this_node->latestEntity;

    // Add fab's components
    for (auto& this_component_initMap_pair : this_node->componentsAndInitMaps)
    {
        engine_ptr->GetECSwrapperPtr()->GetComponentByID(this_component_initMap_pair.first)->AddComponentEntityByMap(this_entity, this_component_initMap_pair.second);
    }

    // Add NodeGlobalMatrixCompEntity
    if(this_node->shouldAddNodeGlobalMatrixCompEntity)
    {
        NodeGlobalMatrixCompEntity this_node_global_matrix_entity(this_entity);
        this_node_global_matrix_entity.parentEntity = parent_entity;

        NodeGlobalMatrixComp* node_global_matrix_comp_ptr = static_cast<NodeGlobalMatrixComp*>(engine_ptr->GetECSwrapperPtr()->GetComponentByID(static_cast<componentID>(componentIDenum::NodeGlobalMatrix)));
        node_global_matrix_comp_ptr->AddComponent(this_entity, this_node_global_matrix_entity);
    }

    for (size_t index = 0; index < this_node->children.size(); index++)
        AddNodeComponentsToECS(this_entity, this_node->children[index].get());
}

std::string GameImport::NumberToString(size_t number)
{
    uint8_t* number_uint8 = reinterpret_cast<uint8_t*>(&number);

    std::string return_string;
    for (size_t i = 0; i < sizeof(size_t); i++)
    {
        if (number_uint8[i] != 0)
        {
            return_string += 'A' + number_uint8[i] & 0x0F;
            return_string += 'A' + (number_uint8[i] >> 4);
        }
    }
    return return_string;
}