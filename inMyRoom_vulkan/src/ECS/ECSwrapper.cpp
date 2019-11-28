#include "ECS/ECSwrapper.h"

ECSwrapper::ECSwrapper(EnginesExportedFunctions* in_enginesExportedFunctions_ptr)
    :enginesExportedFunctions_ptr(in_enginesExportedFunctions_ptr)
{
    componentIDtoComponentBaseClass_map.emplace(static_cast<componentID>(componentIDenum::Default), nullptr);
    entitiesHandler_uptr = std::make_unique<EntitiesHandler>();
}

ECSwrapper::~ECSwrapper()
{
    for (auto& this_component : componentIDtoComponentBaseClass_map)
        if(this_component.second != nullptr)
            this_component.second->Deinit();

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

void ECSwrapper::RemoveEntityFromAllComponents(Entity this_entity)
{
    std::vector<componentID> components_of_entity = entitiesHandler_uptr->GetComponentsOfEntity(this_entity);
    for (componentID this_component : components_of_entity)
        GetComponentByID(this_component)->RemoveComponentEntity(this_entity);
}

void ECSwrapper::RemoveEntityFromAllComponentsAndDelete(Entity this_entity)
{
    RemoveEntityFromAllComponents(this_entity);

    entitesThatGoingToGetEmptyToRemove.emplace_back(this_entity);
}

void ECSwrapper::Update(bool complete_adds_and_removes)
{
    for (auto& this_component : componentIDtoComponentBaseClass_map)
        if (this_component.second != nullptr)
            this_component.second->Update();

    if (complete_adds_and_removes)
        CompleteAddsAndRemoves();
}

void ECSwrapper::FixedUpdate(bool complete_adds_and_removes)
{
    for (auto& this_component : componentIDtoComponentBaseClass_map)
        if (this_component.second != nullptr)
            this_component.second->FixedUpdate();

    if (complete_adds_and_removes)
        CompleteAddsAndRemoves();
}

void ECSwrapper::AsyncUpdate(bool complete_adds_and_removes)
{
    for (auto& this_component : componentIDtoComponentBaseClass_map)
        if (this_component.second != nullptr)
            this_component.second->AsyncUpdate();

    if (complete_adds_and_removes)
        CompleteAddsAndRemoves();
}

void ECSwrapper::CompleteAddsAndRemoves()
{
    for (auto& this_component : componentIDtoComponentBaseClass_map)
        if (this_component.second != nullptr)
            this_component.second->CompleteAddsAndRemoves();

    for (auto& this_entity : entitesThatGoingToGetEmptyToRemove)
        entitiesHandler_uptr->DeleteEmptyEntity(this_entity);

    entitesThatGoingToGetEmptyToRemove.clear();
}

EnginesExportedFunctions* ECSwrapper::GetEnginesExportedFunctions()
{
    return enginesExportedFunctions_ptr;
}