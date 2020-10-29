 #include "ECS/ECSwrapper.h"

ECSwrapper::ECSwrapper(ExportedFunctions* in_enginesExportedFunctions_ptr)
    :exportedFunctions_ptr(in_enginesExportedFunctions_ptr)
{
    componentIDtoComponentBaseClass_map.emplace(static_cast<componentID>(componentIDenum::Default), nullptr);
    entitiesHandler_uptr = std::make_unique<EntitiesHandler>();
}

ECSwrapper::~ECSwrapper()
{
    for (auto& this_component : componentIDtoComponentBaseClass_map)
        if(this_component.second != nullptr)
            this_component.second->Deinit();

    CompleteAddsAndRemoves();

    entitiesHandler_uptr.reset();

    componentsBaseClassOwnership_uptrs.clear();
    entitiesHandler_uptr.reset();
}

EntitiesHandler* ECSwrapper::GetEntitiesHandler() const
{
    return entitiesHandler_uptr.get();
}

ComponentBaseClass* ECSwrapper::GetComponentByID(componentID component_id) const
{
    const auto search = componentIDtoComponentBaseClass_map.find(component_id);
    assert(search != componentIDtoComponentBaseClass_map.end());

    return search->second;
}

componentID ECSwrapper::GetComponentIDbyName(std::string component_name) const
{
    const auto search = componentNameToComponentID_umap.find(component_name);
    assert(search != componentNameToComponentID_umap.end());

    return search->second;
}

const std::map<componentID, ComponentBaseClass*>& ECSwrapper::GetComponentIDtoComponentBaseClassMap() const
{
    return componentIDtoComponentBaseClass_map;
}

std::chrono::duration<float> ECSwrapper::GetUpdateDeltaTime() const
{
    return deltaTime;
}

void ECSwrapper::RefreshUpdateDeltaTime()
{
    auto previous_frame_timePoint = lastFramePoint;
    auto next_frame_timePoint = std::chrono::steady_clock::now();

    deltaTime = next_frame_timePoint - previous_frame_timePoint;

    lastFramePoint = next_frame_timePoint;
}

std::vector<std::string> ECSwrapper::GetComponentsNames() const
{
    std::vector<std::string> return_vector;

    for (auto& this_component : componentIDtoComponentBaseClass_map)
        if (this_component.second != nullptr)
            return_vector.emplace_back(this_component.second->GetComponentName());

    return return_vector;
}

void ECSwrapper::AddComponent(ComponentBaseClass* this_component_ptr)
{
    {
        auto search = componentIDtoComponentBaseClass_map.find(this_component_ptr->GetComponentID());
        assert(search == componentIDtoComponentBaseClass_map.end());
    }
    {
        auto search = componentNameToComponentID_umap.find(this_component_ptr->GetComponentName());
        assert(search == componentNameToComponentID_umap.end());
    }

    componentIDtoComponentBaseClass_map.emplace(this_component_ptr->GetComponentID(), this_component_ptr);
    componentNameToComponentID_umap.emplace(this_component_ptr->GetComponentName(), this_component_ptr->GetComponentID());
}

void ECSwrapper::AddComponentAndOwnership(std::unique_ptr<ComponentBaseClass> this_component_uptr)
{
    AddComponent(this_component_uptr.get());
    componentsBaseClassOwnership_uptrs.emplace_back(std::move(this_component_uptr));
}

void ECSwrapper::AddFabs(const std::vector<Node*>& nodes_ptrs)
{
    assert(fabsInfos.empty());

    // Add existence
    for(const auto& this_node_ptr: nodes_ptrs)
    {
        std::string fab_name = this_node_ptr->nodeName;
        size_t fab_index = fabsInfos.size();

        FabInfo this_fab_info;
        this_fab_info.fabName = fab_name;
        this_fab_info.fabIndex = fab_index;

        this_fab_info.entitiesParents.emplace_back(-1);
        this_fab_info.entityToName.emplace(0, this_node_ptr->nodeName);
        this_fab_info.nameToEntity.emplace(this_node_ptr->nodeName, 0);

        auto& fab_info_ref = fabsInfos.emplace_back(this_fab_info);
        fabNameToIndex_umap.emplace(fab_name, fabsInfos.size() - 1);

        Entity entities_added = 1;
        for(const auto& this_child_uptr: this_node_ptr->children)
        {
            AddNodeExistence(fab_info_ref, this_child_uptr.get(), entities_added, 0, this_node_ptr->nodeName);
        }

        fab_info_ref.size = fab_info_ref.entitiesParents.size();
    }

    // Add component entities
    for(size_t index = 0; index != nodes_ptrs.size(); ++index)
    {
        for (auto& this_component : componentIDtoComponentBaseClass_map)
            if (this_component.second != nullptr)
                this_component.second->PushBackNewFab();

        Entity entities_processed = 0;
        AddFabsCompEntitiesToComponents(fabsInfos[index], nodes_ptrs[index], entities_processed);

        for (auto& this_component : componentIDtoComponentBaseClass_map)
        {
            if (this_component.second != nullptr)
            {
                const auto range = this_component.second->GetLatestFabRange();

                if(range != std::pair<Entity, Entity>(-1, -1))
                {
                    fabsInfos[index].component_ranges.emplace_back(this_component.second, range);
                }
            }
        }
    }
}

void ECSwrapper::AddNodeExistence(FabInfo& fab_info,
                                           Node* this_node_ptr,
                                           Entity& entities_added,
                                           Entity parent,
                                           std::string parent_name)
{
    Entity current_entity = entities_added;
    std::string current_name = parent_name + "/" + this_node_ptr->nodeName;

    fab_info.entitiesParents.emplace_back(parent);
    fab_info.entityToName.emplace(current_entity, current_name);
    fab_info.nameToEntity.emplace(current_name, current_entity);

    ++entities_added;
    for(const auto& this_child_uptr: this_node_ptr->children)
    {
        AddNodeExistence(fab_info, this_child_uptr.get(), entities_added, current_entity, current_name);     
    }
}

void ECSwrapper::AddFabsCompEntitiesToComponents(FabInfo& fab_info,
                                     Node* this_node_ptr,
                                     Entity& entities_processed)
{
    Entity current_entity = entities_processed;
    std::string entity_name = fab_info.entityToName.find(current_entity)->second;

    for(const auto& this_compID_init_map_pair: this_node_ptr->componentIDsToInitMaps)
    {
        GetComponentByID(this_compID_init_map_pair.first)->AddCompEntityAtLatestFab(current_entity, entity_name, this_compID_init_map_pair.second);
    }

    ++entities_processed;
    for(const auto& this_child_uptr: this_node_ptr->children)
    {
        AddFabsCompEntitiesToComponents(fab_info, this_child_uptr.get(),entities_processed);
    }
}

const FabInfo* ECSwrapper::GetFabInfo(const std::string& fab_name) const
{
    auto search = fabNameToIndex_umap.find(fab_name);
    assert(search != fabNameToIndex_umap.end());

    return &fabsInfos[search->second];
}

Entity ECSwrapper::GetRelativeEntityOffset(std::string base_path, std::string relative_path) const
{
    std::string fab_name = base_path.substr(0, base_path.find_first_of("/"));

    auto fab_search = fabNameToIndex_umap.find(fab_name);
    assert(fab_search != fabNameToIndex_umap.end());

    size_t fab_index = fab_search->second;

    Entity base_entity = 0;
    if(base_path != base_path.substr(0, base_path.find_first_of("/")))
    {
        std::string base_entity_name = base_path;
        auto base_entity_search = fabsInfos[fab_index].nameToEntity.find(base_entity_name);

        assert(base_entity_search != fabsInfos[fab_index].nameToEntity.end());

        base_entity = base_entity_search->second;
    }

    Entity relative_entity = 0;
    if(relative_path.substr(0, relative_path.find_first_of("/")) == "_root")
    {
        std::string entity_name = fab_name + "/" + relative_path.substr(relative_path.find_first_of("/") + 1);
        auto entity_search = fabsInfos[fab_index].nameToEntity.find(entity_name);

        assert(entity_search != fabsInfos[fab_index].nameToEntity.end());

        relative_entity = entity_search->second;
    }
    else
    {
        while (relative_path.substr(0, relative_path.find_first_of("/")) == "..")
        {
            base_path = base_path.substr(0, base_path.find_last_of("/"));

            if (relative_path.find_first_of("/") != std::string::npos)
                relative_path = relative_path.substr(relative_path.find_first_of("/") + 1);
            else
                relative_path = "";
        }

        std::string entity_name;
        if(relative_path != "")
            entity_name = base_path + "/" + relative_path;
        else
            entity_name = base_path;

        auto entity_search = fabsInfos[fab_index].nameToEntity.find(entity_name);
        assert(entity_search != fabsInfos[fab_index].nameToEntity.end());

        relative_entity = entity_search->second;
    }

    return relative_entity - base_entity;
}

AdditionInfo* ECSwrapper::AddInstance(const std::string& fab_name, Entity parent)
{
    return AddInstance(GetFabInfo(fab_name), parent);
}

AdditionInfo* ECSwrapper::AddInstance(const std::string& fab_name, const std::string& instance_name, Entity parent)
{
    return AddInstance(GetFabInfo(fab_name), instance_name, parent);
}

AdditionInfo* ECSwrapper::AddInstance(const FabInfo* fab_info_ptr, Entity parent)
{
    auto& addition_info_uptr_ref = additionInfoUptrs.emplace_back(std::make_unique<AdditionInfo>());

    InstanceInfo* instance_info_ptr = entitiesHandler_uptr->AddInstanceEntities(fab_info_ptr, parent);
    addition_info_uptr_ref->instance_info_ptr = instance_info_ptr;

    for(const auto& this_component_vector_of_ranges_pair: fab_info_ptr->component_ranges)
    {
        DataSetPtr this_component_addition_ptr = this_component_vector_of_ranges_pair.first->InitializeFab(instance_info_ptr->entityOffset, fab_info_ptr->fabIndex);
        addition_info_uptr_ref->AddDataSetPtr(this_component_vector_of_ranges_pair.first->GetComponentID(), this_component_addition_ptr);
    }

    return addition_info_uptr_ref.get();
}

AdditionInfo* ECSwrapper::AddInstance(const FabInfo* fab_info_ptr, const std::string& instance_name, Entity parent)
{
    auto& addition_info_uptr_ref = additionInfoUptrs.emplace_back(std::make_unique<AdditionInfo>());

    InstanceInfo* instance_info_ptr = entitiesHandler_uptr->AddInstanceEntities(fab_info_ptr, instance_name, parent);
    addition_info_uptr_ref->instance_info_ptr = instance_info_ptr;

    for(const auto& this_component_vector_of_ranges_pair: fab_info_ptr->component_ranges)
    {
        DataSetPtr this_component_addition_ptr = this_component_vector_of_ranges_pair.first->InitializeFab(instance_info_ptr->entityOffset, fab_info_ptr->fabIndex);
        addition_info_uptr_ref->AddDataSetPtr(this_component_vector_of_ranges_pair.first->GetComponentID(), this_component_addition_ptr);
    }

    return addition_info_uptr_ref.get();
}

void ECSwrapper::RemoveInstance(InstanceInfo* instance_info_ptr)
{
    instancesToBeRemoved.emplace_back(instance_info_ptr);
}

void ECSwrapper::Update(bool complete_adds_and_removes)
{
    std::lock_guard<std::mutex> lock(controlMutex);

    RefreshUpdateDeltaTime();

    for (auto& this_component : componentIDtoComponentBaseClass_map)
        if (this_component.second != nullptr)
            this_component.second->Update();

    if (complete_adds_and_removes)
        CompleteAddsAndRemovesUnsafe();
}

void ECSwrapper::AsyncInput(InputType input_type, void* struct_data)
{
    std::lock_guard<std::mutex> lock(controlMutex);

    for (auto& this_component : componentIDtoComponentBaseClass_map)
        if (this_component.second != nullptr)
            this_component.second->AsyncInput(input_type, struct_data);
}

void ECSwrapper::CompleteAddsAndRemoves()
{
    std::lock_guard<std::mutex> lock(controlMutex);
    CompleteAddsAndRemovesUnsafe();
}

ExportedFunctions* ECSwrapper::GetEnginesExportedFunctions() const
{
    return exportedFunctions_ptr;
}

void ECSwrapper::CompleteAddsAndRemovesUnsafe()
{
    CompleteRemovesUnsafe();
    CompleteAddsUnsafe();
}

void ECSwrapper::CompleteRemovesUnsafe()
{
    std::set<InstanceInfo*> instances_and_children_to_be_deleted;
    for(InstanceInfo* this_instance_ptr: instancesToBeRemoved)
    {
        instances_and_children_to_be_deleted.emplace(this_instance_ptr);
        GetChildrenInstanceTree(this_instance_ptr, instances_and_children_to_be_deleted);
    }

    std::map<ComponentBaseClass*, std::vector<std::pair<Entity, Entity>>> component_to_ranges_to_be_deleted_map;
    for(InstanceInfo* this_instance_ptr: instances_and_children_to_be_deleted)
    {
        for(const auto& this_component_range_pair:this_instance_ptr->fabInfo->component_ranges)
        {
            std::pair<Entity, Entity> range_to_be_removed = std::pair<Entity, Entity>(this_instance_ptr->entityOffset + this_component_range_pair.second.first,
                                                                                      this_instance_ptr->entityOffset + this_component_range_pair.second.second);

            auto iterator_bool_pair = component_to_ranges_to_be_deleted_map.try_emplace(this_component_range_pair.first);
            iterator_bool_pair.first->second.emplace_back(range_to_be_removed);
        }
    }

    // Remove from components
    for(const auto& this_component_vector_of_ranges_pair: component_to_ranges_to_be_deleted_map)
    {
        this_component_vector_of_ranges_pair.first->RemoveInstancesByRanges(this_component_vector_of_ranges_pair.second);
    }

    // Remove from entities handler and free up these entities
    entitiesHandler_uptr->RemoveInstancesEntities(instances_and_children_to_be_deleted);

    instancesToBeRemoved.clear();
}

void ECSwrapper::GetChildrenInstanceTree(InstanceInfo* instance_info_ptr, std::set<InstanceInfo*>& set_of_children)
{
    for(InstanceInfo* this_instance_ptr: instance_info_ptr->instanceChildren)
    {
        set_of_children.emplace(this_instance_ptr);
        GetChildrenInstanceTree(this_instance_ptr, set_of_children);
    }
}

void ECSwrapper::CompleteAddsUnsafe()
{
    for (auto& this_component : componentIDtoComponentBaseClass_map)
        if (this_component.second != nullptr)
            this_component.second->AddInitializedFabs();

    additionInfoUptrs.clear();
}
