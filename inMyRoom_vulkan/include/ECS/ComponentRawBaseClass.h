#pragma once

#include "ECS/ComponentBaseClass.h"

#include <utility>

template<typename ComponentEntityType>
class ComponentRawBaseClass :
    public ComponentBaseClass
{
public:
    ComponentRawBaseClass(const componentID this_ID, const std::string this_name, ECSwrapper* const in_ecs_wrapper_ptr);
    virtual ~ComponentRawBaseClass() override;
    void Deinit() override;

//  void Update() override;
//  void FixedUpdate() override;
//  void AsyncUpdate(/* to do */) override;
    
    void AddComponent(const Entity this_entity, ComponentEntityType this_componentEntity);                      // Component entity specific task at component memory layout level
    void AddComponentEntityByMap(const Entity this_entity, const CompEntityInitMap this_map) override;          // Component entity specific task at component level
    void RemoveComponentEntity(const Entity this_entity) override;                                              // Component entity memory specific task at component memory layout level
    void CompleteAddsAndRemoves() override;                                                                     // Component entity memory specific task at component memory layout level
    ComponentEntityPtr GetComponentEntity(const Entity this_entity) override;                                   // Component entity memory specific task at component memory layout level

protected:
    std::vector<ComponentEntityType> componentEntitiesRaw;
    std::vector<bool> isItEmptyRawVector;


private:
    std::vector<Entity> componentEntitiesToRemove;
    std::vector<std::pair<Entity, ComponentEntityType>> componentEntitiesToAdd;
};





// -----SOURCE-----

template<typename ComponentEntityType>
inline ComponentRawBaseClass<ComponentEntityType>::ComponentRawBaseClass(const componentID this_ID, const std::string this_name, ECSwrapper* const in_ecs_wrapper_ptr)
    :ComponentBaseClass::ComponentBaseClass(this_ID, this_name, in_ecs_wrapper_ptr)
{
    componentEntitiesRaw.emplace_back(ComponentEntityType::GetEmpty());
    isItEmptyRawVector.emplace_back(true);
}

template<typename ComponentEntityType>
ComponentRawBaseClass<ComponentEntityType>::~ComponentRawBaseClass()
{
    Deinit();
}

template<typename ComponentEntityType>
void ComponentRawBaseClass<ComponentEntityType>::Deinit()
{
    componentEntitiesToRemove.clear();
    componentEntitiesToAdd.clear();

    for (size_t index = 0; index < componentEntitiesRaw.size(); index++)
        if (!isItEmptyRawVector[index])
            RemoveComponentEntity(static_cast<Entity>(index));

    CompleteAddsAndRemoves();
}

/*
template<typename ComponentEntityType>
void ComponentRawBaseClass<ComponentEntityType>::Update()
{
    for (size_t index = 0; index < componentEntitiesRaw.size(); index++)
        if (!isItEmptyRawVector[index])
            componentEntitiesRaw[index].Update();
}

template<typename ComponentEntityType>
void ComponentRawBaseClass<ComponentEntityType>::FixedUpdate()
{
    for (size_t index = 0; index < componentEntitiesRaw.size(); index++)
        if (!isItEmptyRawVector[index])
            componentEntitiesRaw[index].FixedUpdate();
}

template<typename ComponentEntityType>
void ComponentRawBaseClass<ComponentEntityType>::AsyncUpdate()
{
    for (size_t index = 0; index < componentEntitiesRaw.size(); index++)
        if (!isItEmptyRawVector[index])
            componentEntitiesRaw[index].AsyncUpdate();
}
*/

template<typename ComponentEntityType>
void ComponentRawBaseClass<ComponentEntityType>::AddComponent(const Entity this_entity, ComponentEntityType this_componentEntity)
{
    componentEntitiesToAdd.emplace_back(std::make_pair(this_entity, this_componentEntity));
}

template<typename ComponentEntityType>
void ComponentRawBaseClass<ComponentEntityType>::AddComponentEntityByMap(const Entity this_entity, const CompEntityInitMap this_map)
{
    ComponentEntityType this_componentEntity = ComponentEntityType::CreateComponentEntityByMap(this_entity, this_map);
    AddComponent(this_entity, this_componentEntity);
}

template<typename ComponentEntityType>
void ComponentRawBaseClass<ComponentEntityType>::RemoveComponentEntity(const Entity this_entity)
{
    componentEntitiesToRemove.emplace_back(this_entity);
}

template<typename ComponentEntityType>
void ComponentRawBaseClass<ComponentEntityType>::CompleteAddsAndRemoves()
{
    {
        for (Entity this_entity : componentEntitiesToRemove)
        {
            assert(!isItEmptyRawVector[this_entity]);
            componentEntitiesRaw[this_entity] = ComponentEntityType::GetEmpty();
            isItEmptyRawVector[this_entity] = true;

            InformEntitiesHandlerAboutRemoval(this_entity);
        }

        componentEntitiesToRemove.clear();
    }

    {
        for (std::pair<Entity, ComponentEntityType> this_entity_componentEntity_pair : componentEntitiesToAdd)
        {
            Entity& this_entity = this_entity_componentEntity_pair.first;
            ComponentEntityType& this_componentEntity = this_entity_componentEntity_pair.second;

            if (componentEntitiesRaw.size() - 1 < static_cast<size_t>(this_entity))
            {
                componentEntitiesRaw.resize(static_cast<size_t>(this_entity + 1), ComponentEntityType::GetEmpty());
                isItEmptyRawVector.resize(static_cast<size_t>(this_entity + 1), true);
            }

            assert(isItEmptyRawVector[this_entity]);

            componentEntitiesRaw[this_entity] = this_componentEntity;
            isItEmptyRawVector[this_entity] = false;

            InformEntitiesHandlerAboutAddition(this_entity);

            componentEntitiesRaw[this_entity].Init();
        }

        componentEntitiesToAdd.clear();
    }
}

template<typename ComponentEntityType>
ComponentEntityPtr ComponentRawBaseClass<ComponentEntityType>::GetComponentEntity(const Entity this_entity)
{
    assert(componentEntitiesRaw.size() > this_entity);
    assert(!isItEmptyRawVector[this_entity]);
    return reinterpret_cast<ComponentEntityPtr>(&componentEntitiesRaw[this_entity]);
}
