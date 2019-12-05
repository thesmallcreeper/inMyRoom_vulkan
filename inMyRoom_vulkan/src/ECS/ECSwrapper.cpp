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

    RemoveEntityAndChildrenFromAllComponentsAndDelete(0);
    CompleteAddsAndRemoves();

    entitiesHandler_uptr.reset();

    componentsBaseClassOwnership_uptrs.clear();
    entitiesHandler_uptr.reset();
}

EntitiesHandler* ECSwrapper::GetEntitiesHandler()
{
    return entitiesHandler_uptr.get();
}

ComponentBaseClass* ECSwrapper::GetComponentByID(componentID component_id)
{
    auto search = componentIDtoComponentBaseClass_map.find(component_id);
    assert(search != componentIDtoComponentBaseClass_map.end());

    return search->second;
}

componentID ECSwrapper::GetComponentIDbyName(std::string component_name)
{
    auto search = componentNameToComponentID_umap.find(component_name);
    assert(search != componentNameToComponentID_umap.end());

    return search->second;
}

std::vector<std::string> ECSwrapper::GetComponentsNames()
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

void ECSwrapper::RemoveEntityAndChildrenFromAllComponentsAndDelete(Entity this_entity)
{
    std::vector<Entity> children_of_entity = entitiesHandler_uptr->GetChildrenOfEntity(this_entity);
    for (auto& this_child : children_of_entity)
        RemoveEntityAndChildrenFromAllComponentsAndDelete(this_child);

    if (this_entity != 0)
    {
        std::vector<componentID> components_of_entity = entitiesHandler_uptr->GetComponentsOfEntity(this_entity);
        for (componentID this_component : components_of_entity)
            GetComponentByID(this_component)->RemoveComponentEntity(this_entity);
    }

    entitesThatGoingToGetEmptyToRemove.emplace_back(this_entity);
}

void ECSwrapper::Update(bool complete_adds_and_removes)
{
    std::lock_guard<std::mutex> lock(controlMutex);

    for (auto& this_component : componentIDtoComponentBaseClass_map)
        if (this_component.second != nullptr)
            this_component.second->Update();

    if (complete_adds_and_removes)
        CompleteAddsAndRemovesUnsafe();
}

void ECSwrapper::FixedUpdate()
{
    std::lock_guard<std::mutex> lock(controlMutex);

    for (auto& this_component : componentIDtoComponentBaseClass_map)
        if (this_component.second != nullptr)
            this_component.second->FixedUpdate();
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

void ECSwrapper::CompleteAddsAndRemovesUnsafe()
{
    for (auto& this_component : componentIDtoComponentBaseClass_map)
        if (this_component.second != nullptr)
            this_component.second->CompleteAddsAndRemoves();

    for (auto& this_entity : entitesThatGoingToGetEmptyToRemove)
        entitiesHandler_uptr->DeleteEmptyEntity(this_entity);

    entitesThatGoingToGetEmptyToRemove.clear();
}

ExportedFunctions* ECSwrapper::GetEnginesExportedFunctions()
{
    return exportedFunctions_ptr;
}