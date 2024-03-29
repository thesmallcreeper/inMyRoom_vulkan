#include "GameImporter.h"

#include "Engine.h"
#include "ECS/ECSwrapper.h"

#include <algorithm>
#include <unordered_set>
#include <cassert>

#include "tiny_gltf.h"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/matrix_decompose.hpp"

GameImporter::GameImporter(Engine* in_engine_ptr, std::string gameConfig_path)
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

void GameImporter::ImportGame()
{
    AddGameDLLcomponents();

    AddEmptyNode();
    AddImports();

    AddDefaultCameraFab();
    AddFabs();

    std::vector<Node*> fab_root_nodes;
    for(const auto& this_fab_name_uptr_pair: fabs_umap)
    {
        fab_root_nodes.emplace_back(this_fab_name_uptr_pair.second.get());
    }

    engine_ptr->GetECSwrapperPtr()->AddFabs(fab_root_nodes);
}

void GameImporter::AddGameDLLcomponents()
{
    if (gameConfig.has_key("gameDLL"))
    {
        printf("-Importing gameDLL\n");
        // TODO
        std::string dll_path = folderName + "/" + gameConfig["gameDLL"]["path"].as_string();

        gameDLLimporter_uptr = std::make_unique<GameDLLimporter>(engine_ptr->GetECSwrapperPtr(), dll_path);
        gameDLLimporter_uptr->AddComponentsToECSwrapper();
    }
}

void GameImporter::AddEmptyNode()
{
    std::unique_ptr<Node> this_empty_node = std::make_unique<Node>();
    imports_umap.emplace("_empty", std::move(this_empty_node));
}

void GameImporter::AddImports()
{
    std::vector<tinygltf::Model> models;
    std::vector<std::string> models_name;
    std::vector<std::string> models_folder;

    for (const auto& this_iterator : gameConfig["imports"].as_object())
    {
        std::string this_file_to_import = folderName + "/" + this_iterator.value().as_string();

        models.emplace_back(LoadglTFmodel(this_file_to_import));
        models_name.emplace_back(this_iterator.key());
        models_folder.emplace_back(GetFilePathFolder(this_file_to_import));
    }

    for (size_t index = 0; index < models.size(); index++)
    {
        printf("-Loading model: %s\n", models_name[index].c_str());
        engine_ptr->GetGraphicsPtr()->LoadModel(models[index], models_folder[index]);
    }

    engine_ptr->GetGraphicsPtr()->EndModelsLoad();

    for (size_t index = 0; index < models.size(); index++)
    {
        std::unique_ptr<Node> this_model_node = ImportModel(models_name[index], models[index]);

        imports_umap.emplace(models_name[index], std::move(this_model_node));
    }

}

void GameImporter::AddDefaultCameraFab()
{
    auto search = imports_umap.find("_empty");
    assert(search != imports_umap.end());

    std::string default_camera_fab_name = "_defaultCamera";

    std::unique_ptr<Node> default_camera_fab = std::make_unique<Node>(*search->second.get());
    default_camera_fab->nodeName = default_camera_fab_name;

    {
        CompEntityInitMap this_map;
        default_camera_fab->componentIDsToInitMaps.emplace(static_cast<componentID>(componentIDenum::Camera), this_map);
    }
    {
        CompEntityInitMap this_map;
        this_map.intMap.emplace("freezed", false);
        default_camera_fab->componentIDsToInitMaps.emplace(static_cast<componentID>(componentIDenum::CameraDefaultInput), this_map);
    }

    fabs_umap.emplace(default_camera_fab_name, std::move(default_camera_fab));
}

void GameImporter::AddFabs()
{
    for (const auto& this_iterator : gameConfig["fabs"].as_object())
    {
        std::string this_fab_name = this_iterator.key();
        std::string this_fab_import_path = this_iterator.value()["basedOn"].as_string();

        Node* this_node = GetImportNode(this_fab_import_path);

        std::unique_ptr<Node> this_fab = std::make_unique<Node>(*this_node);
        this_fab->nodeName = this_fab_name;

        // Tweaks
        if (this_iterator.value().has_key("tweak"))
        {
            for (const auto& this_tweak_iterator : this_iterator.value()["tweak"].as_object())
            {
                const std::string node_name = this_tweak_iterator.key();
                const configuru::Config& tweak_properties = this_tweak_iterator.value();

                Node* this_tweaked_node = FindNodeInTree(this_fab.get(), node_name);

                AddTweaksToNode(this_tweaked_node, tweak_properties);
            }
        }

        auto search = fabs_umap.find(this_fab_name);
        if (search == fabs_umap.end()) {
            fabs_umap.emplace(this_fab_name, std::move(this_fab));
        } else {
            search->second = std::move(this_fab);
        }
    }
}

std::unique_ptr<Node> GameImporter::ImportModel(std::string model_name, tinygltf::Model& this_model)
{
    std::unique_ptr<Node> model_node = std::make_unique<Node>();
    model_node->nodeName = model_name;

    for (tinygltf::Scene& this_gltf_scene : this_model.scenes)
    {
        std::unique_ptr<Node> scene_node = std::make_unique<Node>();

        scene_node->nodeName = this_gltf_scene.name;
        assert(scene_node->nodeName != "");

        // Add existances
        for (size_t this_gltf_scene_node_index : this_gltf_scene.nodes)
        {
            tinygltf::Node& this_gltf_scene_node = this_model.nodes[this_gltf_scene_node_index];

            std::unique_ptr<Node> this_scene_node_uptr = ImportNodeExistance(this_gltf_scene_node, this_model);
            this_scene_node_uptr->glTFnodeIndex = this_gltf_scene_node_index;

            scene_node->children.emplace_back(std::move(this_scene_node_uptr));
        }

        // Add componentsCount
        for (size_t index = 0; index < scene_node->children.size(); index++)
            ImportNodeComponents(scene_node->children[index].get(), scene_node.get(), this_model);

        // Add animation composers
        {
            scene_node->children.emplace_back(ImportModelAnimationComposerAsNodes(scene_node.get(), this_model));
        }

        // Add scene node position comp
        {
            scene_node->componentIDsToInitMaps.emplace(static_cast<componentID>(componentIDenum::NodeData), CompEntityInitMap());
            scene_node->componentIDsToInitMaps.emplace(static_cast<componentID>(componentIDenum::EarlyNodeGlobalMatrix), CompEntityInitMap());
            scene_node->componentIDsToInitMaps.emplace(static_cast<componentID>(componentIDenum::LateNodeGlobalMatrix), CompEntityInitMap());
        }

        model_node->children.emplace_back(std::move(scene_node));
    }

    return std::move(model_node);
}

std::unique_ptr<Node> GameImporter::ImportNodeExistance(tinygltf::Node& this_gltf_node, tinygltf::Model& model)
{
    std::unique_ptr<Node> return_node_uptr = std::make_unique<Node>();

    if(this_gltf_node.name != "")
        return_node_uptr->nodeName = this_gltf_node.name;
    else
        return_node_uptr->nodeName = "node_" + NumberToString(anonymousNameCounter++);

    if (this_gltf_node.children.size())
    {
        for (size_t this_gltf_recurse_node_index : this_gltf_node.children)
        {
            tinygltf::Node& this_gltf_recurse_node = model.nodes[this_gltf_recurse_node_index];
            
            std::unique_ptr<Node> this_recurse_node_uptr = ImportNodeExistance(this_gltf_recurse_node, model);
            this_recurse_node_uptr->glTFnodeIndex = this_gltf_recurse_node_index;

            return_node_uptr->children.emplace_back(std::move(this_recurse_node_uptr));
        }
    }

    return std::move(return_node_uptr);
}

std::unique_ptr<Node> GameImporter::ImportModelAnimationComposerAsNodes(Node* root_node, tinygltf::Model& model)
{
    std::unique_ptr<Node> return_node_uptr = std::make_unique<Node>();
    return_node_uptr->nodeName = "_animations";

    for (const tinygltf::Animation& this_animation : model.animations)
    {
        std::unique_ptr<Node> this_animation_composer_node_uptr = std::make_unique<Node>();

        CompEntityInitMap this_map;

        std::string this_animation_name;
        if (this_animation.name != "")
            this_animation_name = this_animation.name;
        else
            this_animation_name = "animation_" + NumberToString(anonymousNameCounter++);

        this_animation_composer_node_uptr->nodeName = this_animation_name;
        this_map.stringMap.emplace(std::make_pair("AnimationName", this_animation_name));

        std::unordered_set<std::string> animation_actor_nodes_uset;
        for (const tinygltf::AnimationChannel& this_animationChannel : this_animation.channels)
        {
            std::string this_channel_target_node_path = GetPathUsingGLTFindex(root_node, this_animationChannel.target_node);
            std::string this_animation_composer_node_path = root_node->nodeName + "/" + return_node_uptr->nodeName + "/" + this_animation_name;

            std::string relative_path = GetRelativePath(this_channel_target_node_path, this_animation_composer_node_path);

            animation_actor_nodes_uset.emplace(relative_path);
        }

        {
            size_t index = 0;
            for (const std::string& this_node_relative_name : animation_actor_nodes_uset)
            {
                this_map.stringMap.emplace(std::make_pair("NodesRelativeName_" + std::to_string(index), this_node_relative_name));
                index++;
            }
        }

        this_animation_composer_node_uptr->componentIDsToInitMaps.emplace(static_cast<componentID>(componentIDenum::AnimationComposer), this_map);

        return_node_uptr->children.emplace_back(std::move(this_animation_composer_node_uptr));
    }

    return std::move(return_node_uptr);
}


void GameImporter::ImportNodeComponents(Node* this_node, Node* root_node, tinygltf::Model& model)
{
    tinygltf::Node& this_gltf_node = model.nodes[this_node->glTFnodeIndex];

    // NodeData component
    {
        this_node->componentIDsToInitMaps.emplace(static_cast<componentID>(componentIDenum::NodeData), CreatePositionInitMap(this_gltf_node));
        this_node->componentIDsToInitMaps.emplace(static_cast<componentID>(componentIDenum::EarlyNodeGlobalMatrix), CompEntityInitMap());
        this_node->componentIDsToInitMaps.emplace(static_cast<componentID>(componentIDenum::LateNodeGlobalMatrix), CompEntityInitMap());
    }

    // Model draw component
    int mesh_index = -1;
    bool is_skin = false;
    bool has_morphTargets = false;
    if (this_gltf_node.mesh != -1)
    {
        CompEntityInitMap this_map;

        mesh_index = this_gltf_node.mesh + static_cast<int>(engine_ptr->GetGraphicsPtr()->GetMeshesOfNodesPtr()->GetMeshIndexOffsetOfModel(model));
        is_skin = engine_ptr->GetGraphicsPtr()->GetMeshesOfNodesPtr()->GetMeshInfo(mesh_index).IsSkinned();
        has_morphTargets = engine_ptr->GetGraphicsPtr()->GetMeshesOfNodesPtr()->GetMeshInfo(mesh_index).HasMorphTargets();

        this_map.intMap["MeshIndex"] = mesh_index;

        assert(not is_skin && this_gltf_node.skin == -1 || is_skin && this_gltf_node.skin != -1);

        if (is_skin || has_morphTargets)
        {
            this_map.intMap["IsSkin"] = static_cast<int>(is_skin);
            this_map.intMap["HasMorphTargets"] = static_cast<int>(has_morphTargets);
            this_map.intMap["DisableCulling"] = static_cast<int>(true);
        }
        if (this_node->nodeName.find("_collision") != std::string::npos)
        {
            this_map.intMap["ShouldDraw"] = static_cast<int>(false);
        }

        this_node->componentIDsToInitMaps.emplace(static_cast<componentID>(componentIDenum::ModelDraw), this_map);
    }

    // Model collision component
    if (mesh_index != -1 && not is_skin && not has_morphTargets)
    {
        CompEntityInitMap this_map;
        this_map.intMap["MeshIndex"] = this_gltf_node.mesh + static_cast<int>(engine_ptr->GetGraphicsPtr()->GetMeshesOfNodesPtr()->GetMeshIndexOffsetOfModel(model));

        this_node->componentIDsToInitMaps.emplace(static_cast<componentID>(componentIDenum::ModelCollision), this_map);
    }

    // Skin and morph target component
    if (mesh_index != -1 && (is_skin || has_morphTargets))
    {
        CompEntityInitMap this_map;

        if (is_skin) {
            size_t skin_index = static_cast<size_t>(this_gltf_node.skin) + engine_ptr->GetGraphicsPtr()->GetSkinsOfMeshesPtr()->GetSkinIndexOffsetOfModel(model);
            SkinInfo this_skinInfo = engine_ptr->GetGraphicsPtr()->GetSkinsOfMeshesPtr()->GetSkin(skin_index);

            this_map.intMap["InverseBindMatricesOffset"] = static_cast<int>(this_skinInfo.inverseBindMatricesFirstOffset);

            std::string this_node_path = GetPathUsingGLTFindex(root_node, this_node->glTFnodeIndex);
            for (size_t index = 0; index < this_skinInfo.glTFnodesJoints.size(); ++index) {
                std::string this_joint_path = GetPathUsingGLTFindex(root_node, this_skinInfo.glTFnodesJoints[index]);
                std::string relative_path = GetRelativePath(this_joint_path, this_node_path);

                this_map.stringMap.emplace(std::make_pair("JointRelativeName_" + std::to_string(index), relative_path));
            }
        }
        if (has_morphTargets) {
            std::vector<float> weights = engine_ptr->GetGraphicsPtr()->GetMeshesOfNodesPtr()->GetMeshInfo(mesh_index).morphDefaultWeights;

            if (this_gltf_node.weights.size()) {
                assert(weights.size() == this_gltf_node.weights.size());
                std::transform(this_gltf_node.weights.begin(), this_gltf_node.weights.end(), weights.begin(),
                               [](double w) -> float { return float(w); });
            }

            for (size_t index = 0; index < weights.size(); ++index) {
                this_map.floatMap.emplace(std::make_pair("DefaultWeight_" + std::to_string(index), weights[index]));
            }
        }

        this_node->componentIDsToInitMaps.emplace(static_cast<componentID>(componentIDenum::DynamicMesh), this_map);
    }

    // Animation actor component
    {
        CompEntityInitMap this_map;

        size_t animations_count_of_actor = 0;
        for (const tinygltf::Animation& this_animation : model.animations)
        {
            size_t animationData_index;
            bool has_animation_init_on_actor = false;
            
            for (const tinygltf::AnimationChannel& this_channel : this_animation.channels)
            {
                if (this_channel.target_node == this_node->glTFnodeIndex)
                {
                    if (!has_animation_init_on_actor)
                    {
                        animationData_index = engine_ptr->GetGraphicsPtr()->GetAnimationsDataOfNodesPtr()->RegistAnimationsDataAndGetIndex();

                        this_map.stringMap.emplace(std::make_pair("Animation_" + std::to_string(animations_count_of_actor), this_animation.name));
                        this_map.intMap.emplace(std::make_pair(this_animation.name + "_animationIndex", static_cast<int>(animationData_index)));

                        has_animation_init_on_actor = true;
                    }

                    const tinygltf::AnimationSampler& this_animationSampler = this_animation.samplers[this_channel.sampler];    

                    InterpolationType this_interpolation_type;
                    if (this_animationSampler.interpolation == "CUBICSPLINE")
                        this_interpolation_type = InterpolationType::CubicSpline;
                    else if (this_animationSampler.interpolation == "STEP")
                        this_interpolation_type = InterpolationType::Step;
                    else
                        this_interpolation_type = InterpolationType::Linear;

                    std::vector<float> sampler_input_data = GetglTFAccessorFloat(model.accessors[this_animationSampler.input], model);
                    std::vector<float> sampler_output_data = GetglTFAccessorFloat(model.accessors[this_animationSampler.output], model);

                    if (this_channel.target_path == "translation")
                    {
                        assert(sampler_input_data.size() * 3 == sampler_output_data.size());

                        AnimationData this_partOf_animationData;
                        this_partOf_animationData.timeToTranslation_interpolation = this_interpolation_type;

                        for (size_t index = 0; index < sampler_input_data.size(); index++)
                        {
                            float input_key = sampler_input_data[index];
                            glm::vec3 output_key = glm::vec3(sampler_output_data[index * 3], -sampler_output_data[index * 3 + 1], -sampler_output_data[index * 3 + 2]);

                            this_partOf_animationData.timeToTranslationKey_map.emplace(input_key, output_key);
                        }

                        engine_ptr->GetGraphicsPtr()->GetAnimationsDataOfNodesPtr()->AddAnimationData(animationData_index, this_partOf_animationData);
                    }
                    if (this_channel.target_path == "rotation")
                    {
                        assert(sampler_input_data.size() * 4 == sampler_output_data.size());

                        AnimationData this_partOf_animationData;
                        this_partOf_animationData.timeToRotation_interpolation = this_interpolation_type;

                        for (size_t index = 0; index < sampler_input_data.size(); index++)
                        {
                            float input_key = sampler_input_data[index];
                           
                            glm::qua<float> output_key = glm::qua<float>(sampler_output_data[index * 4 + 3], sampler_output_data[index * 4], -sampler_output_data[index * 4 + 1], -sampler_output_data[index * 4 + 2]);
                            this_partOf_animationData.timeToRotationKey_map.emplace(input_key, output_key);
                        }

                        engine_ptr->GetGraphicsPtr()->GetAnimationsDataOfNodesPtr()->AddAnimationData(animationData_index, this_partOf_animationData);
                    }
                    if (this_channel.target_path == "scale")
                    {
                        assert(sampler_input_data.size() * 3 == sampler_output_data.size());

                        AnimationData this_partOf_animationData;
                        this_partOf_animationData.timeToScale_interpolation = this_interpolation_type;

                        for (size_t index = 0; index < sampler_input_data.size(); index++)
                        {
                            float input_key = sampler_input_data[index];
                            glm::vec3 output_key = glm::vec3(sampler_output_data[index * 3], sampler_output_data[index * 3 + 1], sampler_output_data[index * 3 + 2]);

                            this_partOf_animationData.timeToScaleKey_map.emplace(input_key, output_key);
                        }

                        engine_ptr->GetGraphicsPtr()->GetAnimationsDataOfNodesPtr()->AddAnimationData(animationData_index, this_partOf_animationData);
                    }
                }
            }

            if (has_animation_init_on_actor)
                animations_count_of_actor++;
        }

        if(this_map.stringMap.size())
            this_node->componentIDsToInitMaps.emplace(static_cast<componentID>(componentIDenum::AnimationActor), this_map);
    }

    for (size_t index = 0; index < this_node->children.size(); index++)
        ImportNodeComponents(this_node->children[index].get(), root_node, model);
}

tinygltf::Model GameImporter::LoadglTFmodel(std::string path)
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

CompEntityInitMap GameImporter::CreatePositionInitMap(const tinygltf::Node& in_node)
{
    NodeDataCompEntity targeted_positionCompEntity = NodeDataCompEntity::GetEmpty();

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

std::vector<float> GameImporter::GetglTFAccessorFloat(const tinygltf::Accessor& in_accessor, const tinygltf::Model& in_model)
{
    std::vector<unsigned char> temp_buffer;
    {
        size_t count_of_elements = in_accessor.count;
        size_t accessor_byte_offset = in_accessor.byteOffset;

        size_t size_of_each_component_in_byte;
        switch (static_cast<glTFcomponentType>(in_accessor.componentType))
        {
            default:
            case glTFcomponentType::type_byte:
            case glTFcomponentType::type_unsigned_byte:
                size_of_each_component_in_byte = sizeof(int8_t);
                break;
            case glTFcomponentType::type_short:
            case glTFcomponentType::type_unsigned_short:
                size_of_each_component_in_byte = sizeof(int16_t);
                break;
            case glTFcomponentType::type_float:
                size_of_each_component_in_byte = sizeof(int32_t);
                break;
        }

        size_t number_of_components_per_type;
        switch (static_cast<glTFtype>(in_accessor.type))
        {
            default:
            case glTFtype::type_scalar:
                number_of_components_per_type = 1;
                break;
            case glTFtype::type_vec3:
                number_of_components_per_type = 3;
                break;
            case glTFtype::type_vec4:
                number_of_components_per_type = 4;
                break;
        }

        const tinygltf::BufferView& this_bufferView = in_model.bufferViews[in_accessor.bufferView];
        size_t bufferview_byte_offset = this_bufferView.byteOffset;

        const tinygltf::Buffer& this_buffer = in_model.buffers[this_bufferView.buffer];

        std::copy(&this_buffer.data[bufferview_byte_offset + accessor_byte_offset],
                    &this_buffer.data[bufferview_byte_offset + accessor_byte_offset] + count_of_elements * size_of_each_component_in_byte * number_of_components_per_type,
                    std::back_inserter(temp_buffer));
    }

    std::vector<float> return_float_buffer;

    switch (static_cast<glTFcomponentType>(in_accessor.componentType))
    {
        default:
        case glTFcomponentType::type_byte:
        {
            size_t size = temp_buffer.size();
            int8_t* data = reinterpret_cast<int8_t*>(temp_buffer.data());

            for (size_t index = 0; index < size; index++)
                return_float_buffer.emplace_back(std::max<float>( static_cast<float>(data[index]) / 127.f, -1.f));

            break;
        }
        case glTFcomponentType::type_unsigned_byte:
        {
            size_t size = temp_buffer.size();
            uint8_t* data = reinterpret_cast<uint8_t*>(temp_buffer.data());

            for (size_t index = 0; index < size; index++)
                return_float_buffer.emplace_back( static_cast<float>(data[index]) / 255.f);

            break;
        }  
        case glTFcomponentType::type_short:
        {
            size_t size = temp_buffer.size() / sizeof(int16_t);
            int16_t* data = reinterpret_cast<int16_t*>(temp_buffer.data());

            for (size_t index = 0; index < size; index++)
                return_float_buffer.emplace_back(std::max<float>( static_cast<float>(data[index]) / 32767.f, -1.f));

            break;
        }
        case glTFcomponentType::type_unsigned_short:
        {
            size_t size = temp_buffer.size() / sizeof(uint16_t);
            uint16_t* data = reinterpret_cast<uint16_t*>(temp_buffer.data());

            for (size_t index = 0; index < size; index++)
                return_float_buffer.emplace_back( static_cast<float>(data[index]) / 65535.f);

            break;
        }
        case glTFcomponentType::type_float:
        {
            {
                size_t size = temp_buffer.size() / sizeof(float);
                float* data = reinterpret_cast<float*>(temp_buffer.data());

                for (size_t index = 0; index < size; index++)
                    return_float_buffer.emplace_back(data[index]);

                break;
            }

            break;
        }
    }

    return return_float_buffer;
}

std::string GameImporter::GetFilePathExtension(const std::string& in_filePath)
{
    if (in_filePath.find_last_of(".") != std::string::npos)
        return in_filePath.substr(in_filePath.find_last_of("."));
    return "";
}

std::string GameImporter::GetFilePathFolder(const std::string& in_filePath)
{
    if (in_filePath.find_last_of("/") != std::string::npos)
        return in_filePath.substr(0, in_filePath.find_last_of("/"));
    else
        return "";
}

void GameImporter::InitializeGame()
{
    InitOneDefaultCameraAndBindIt();

    for (const auto& this_iterator : gameConfig["init"].as_object())
    {
        std::string this_init_name = this_iterator.key();
        std::string this_init_fab_using = this_iterator.value().as_string();

        engine_ptr->GetECSwrapperPtr()->AddInstance(this_init_fab_using, this_init_name);

        engine_ptr->GetECSwrapperPtr()->CompleteAddsAndRemoves();
    }
}

void GameImporter::InitOneDefaultCameraAndBindIt()
{
    AdditionInfo* default_camera_addition_ptr = engine_ptr->GetECSwrapperPtr()->AddInstance("_defaultCamera", "_defaultCamera");
    Entity default_camera_entity = default_camera_addition_ptr->instance_info_ptr->entityOffset;

    engine_ptr->GetECSwrapperPtr()->CompleteAddsAndRemoves();

    CameraComp* camera_comp_ptr = reinterpret_cast<CameraComp*>(engine_ptr->GetECSwrapperPtr()->GetComponentByID(static_cast<componentID>(componentIDenum::Camera)));
    camera_comp_ptr->BindCameraEntity(default_camera_entity);
}

void GameImporter::AddTweaksToNode(Node* this_node, const configuru::Config& tweak_properties)
{

    for (const std::string component_name : engine_ptr->GetECSwrapperPtr()->GetComponentsNames())
    {
        if (tweak_properties.has_key(component_name))
        {
            componentID this_componentID = engine_ptr->GetECSwrapperPtr()->GetComponentIDbyName(component_name);

            this_node->componentIDsToInitMaps.try_emplace(this_componentID, CompEntityInitMap());

            for(const auto& this_iterator: tweak_properties[component_name].as_object())
            {
                std::string init_map_key = this_iterator.key();

                if(this_iterator.value().is_array())
                {
                    const configuru::Config& this_array = this_iterator.value().as_array();
                    if(this_array.array_size() == 4)
                    {
                            glm::vec4 this_vec4 = glm::vec4(this_array[0].as_float(), this_array[1].as_float(), this_array[2].as_float(), this_array[3].as_float());
                            this_node->componentIDsToInitMaps[this_componentID].vec4Map[init_map_key] = this_vec4;
                    }
                    else if(this_array.array_size() == 3)
                    {
                            glm::vec4 this_vec4 = glm::vec4(this_array[0].as_float(), this_array[1].as_float(), this_array[2].as_float(), 0.f);
                            this_node->componentIDsToInitMaps[this_componentID].vec4Map[init_map_key] = this_vec4;
                    }
                    else if(this_array.array_size() == 2)
                    {
                            glm::vec4 this_vec4 = glm::vec4(this_array[0].as_float(), this_array[1].as_float(), 0.f, 0.f);
                            this_node->componentIDsToInitMaps[this_componentID].vec4Map[init_map_key] = this_vec4;
                    }
                }
                else if(this_iterator.value().is_float())
                {
                    float this_float = this_iterator.value().as_float();
                    this_node->componentIDsToInitMaps[this_componentID].floatMap[init_map_key] = this_float;
                }
                else if(this_iterator.value().is_int())
                {
                    int this_int = this_iterator.value().as_integer<int>();
                    this_node->componentIDsToInitMaps[this_componentID].intMap[init_map_key] = this_int;
                }
                else if(this_iterator.value().is_bool())
                {
                    bool this_bool = this_iterator.value().as_bool();
                    this_node->componentIDsToInitMaps[this_componentID].intMap[init_map_key] = static_cast<int>(this_bool);
                }
                else if(this_iterator.value().is_string())
                {
                    std::string this_string = this_iterator.value().as_string();
                    this_node->componentIDsToInitMaps[this_componentID].stringMap[init_map_key] = this_string;
                }
            }
        }
    }
}

Node* GameImporter::GetImportNode(std::string import_node_path)
{
    std::string import_name = import_node_path.substr(0, import_node_path.find_first_of("/"));

    auto search = imports_umap.find(import_name);
    assert(search != imports_umap.end());

    Node* this_root_node = search->second.get();

    std::string rest_of_path;
    if (import_node_path.find_first_of("/") != std::string::npos)
        rest_of_path = import_node_path.substr(import_node_path.find_first_of("/") + 1);
    else
        rest_of_path = "";

    Node* requested_node = FindNodeInTree(this_root_node, rest_of_path);

    return requested_node;
}

Node* GameImporter::GetFabNode(std::string fab_node_path)
{
    std::string fab_name = fab_node_path.substr(0, fab_node_path.find_first_of("/"));

    auto search = fabs_umap.find(fab_name);
    assert(search != fabs_umap.end());

    Node* this_root_node = search->second.get();

    std::string rest_of_path;
    if (fab_node_path.find_first_of("/") != std::string::npos)
        rest_of_path = fab_node_path.substr(fab_node_path.find_first_of("/") + 1);
    else
        rest_of_path = "";

    Node* requested_node = FindNodeInTree(this_root_node, rest_of_path);

    return requested_node;
}

Node* GameImporter::FindNodeInTree(Node* root_node, std::string path)
{
    Node* this_node = root_node;
    while (path.substr(0, path.find_first_of("/")) != "")
    {
        std::string child_name = path.substr(0, path.find_first_of("/"));

        auto search = std::find_if(this_node->children.begin(), this_node->children.end(), [child_name](const auto& child_uptr_ref) {return child_uptr_ref->nodeName == child_name; });
        assert(search != this_node->children.end());

        this_node = search->get();

        if (path.find_first_of("/") != std::string::npos)
            path = path.substr(path.find_first_of("/") + 1);
        else
            path = "";
    }

    return this_node;
}

std::string GameImporter::GetPathUsingGLTFindex(Node* root_node, size_t glTF_node_Index)
{
    // recursive
    std::string return_path = "";

    if (root_node->glTFnodeIndex == glTF_node_Index)
        return_path = root_node->nodeName;
    else
    {
        for (size_t index = 0; index < root_node->children.size(); index++)
        {
            std::string this_result_path = GetPathUsingGLTFindex(root_node->children[index].get(), glTF_node_Index);

            if (this_result_path != "")
            {
                return_path = root_node->nodeName + "/" + this_result_path;
                break;
            }
        }
    }

    return return_path;
}

std::string GameImporter::GetRelativePath(std::string path, std::string relative_to_path)
{
    // Code tweaked from https://stackoverflow.com/a/9978227

    std::string return_relative_path;

    // find out where the two paths diverge
    while (path != "" && relative_to_path != "" && path.substr(0, path.find_first_of("/", 0)) == relative_to_path.substr(0, relative_to_path.find_first_of("/", 0)))
    {
        path = path.substr(path.find_first_of("/", 0) + 1);
        relative_to_path = relative_to_path.substr(relative_to_path.find_first_of("/", 0) + 1);
    }

    // add "../" for each remaining token in relative_to
    while (relative_to_path != "") 
    {
        return_relative_path += "../";

        if (relative_to_path.find_first_of("/") != std::string::npos)
            relative_to_path = relative_to_path.substr(relative_to_path.find_first_of("/", 0) + 1);
        else
            relative_to_path = "";
    }

    // add remaining path
    while (path != "") 
    {
        return_relative_path += path.substr(0, path.find_first_of("/", 0));

        if (path.find_first_of("/") != std::string::npos)
        {
            if (*return_relative_path.rbegin() != '/')
                return_relative_path += "/";

            path = path.substr(path.find_first_of("/", 0) + 1);
        }
        else
            path = "";
    }

    return return_relative_path;
}

std::string GameImporter::NumberToString(size_t number)
{
    uint8_t* number_uint8 = reinterpret_cast<uint8_t*>(&number);

    std::string return_string;
    for (size_t i = 0; i < sizeof(size_t); i++)
    {
        if (number_uint8[i] != 0)
        {
            return_string += static_cast<char>((uint8_t)('A') + (number_uint8[i] >> 4));
            return_string += static_cast<char>((uint8_t)('A') + (number_uint8[i] & 0x0F));
        }
    }
    return return_string;
}