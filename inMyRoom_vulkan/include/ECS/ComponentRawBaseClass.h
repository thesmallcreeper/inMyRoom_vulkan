#pragma once

#include "ECS/ComponentBaseWrappedClass.h"
#include "ECS/TemplateHelpers.h"

#include <utility>

template<typename ComponentEntityType, componentID component_ID, FixedString component_name>
class ComponentRawBaseClass :
    public ComponentBaseWrappedClass<ComponentEntityType, component_ID, component_name>
{
public:
    ComponentRawBaseClass(ECSwrapper* const in_ecs_wrapper_ptr);
    ~ComponentRawBaseClass() override;
    void Deinit() override;

    void Update() override;

    std::vector<std::pair<std::string, MapType>> GetComponentInitMapFields() const override;
    
    void AddComponent(const Entity this_entity, const ComponentEntityType this_componentEntity);                // Component entity specific task
    void AddComponentEntityByMap(const Entity this_entity, const CompEntityInitMap& this_map) override;         // Component entity specific task
    void RemoveComponentEntity(const Entity this_entity) override;                                              // Component entity memory specific task
    void CompleteAddsAndRemoves() override;                                                                     // Component entity memory specific task
    void InitAdds() override;                                                                                   // Component entity memory specific task
    ComponentEntityPtr GetComponentEntityVoidPtr(const Entity this_entity) override;                            // Component entity memory specific task

private:
    void Update_impl() requires TrivialUpdatable<ComponentEntityType>;
    void Update_impl() {}

protected:
    std::vector<ComponentEntityType> componentEntitiesRaw;
    std::vector<bool> isItEmptyRawVector;


private:
    std::vector<Entity> componentEntitiesToRemove;

    std::vector<std::pair<Entity, ComponentEntityType>> componentEntitiesToAdd;
    std::vector<Entity> componentEntitiesToInit;

    template <typename T> void Hi();

};

// -----SOURCE-----

template<typename ComponentEntityType, componentID component_ID, FixedString component_name>
inline ComponentRawBaseClass<ComponentEntityType, component_ID, component_name>::ComponentRawBaseClass(ECSwrapper* const in_ecs_wrapper_ptr)
    :ComponentBaseWrappedClass<ComponentEntityType, component_ID, component_name>::ComponentBaseWrappedClass(in_ecs_wrapper_ptr)
{
    componentEntitiesRaw.emplace_back(ComponentEntityType::GetEmpty());
    isItEmptyRawVector.emplace_back(true);
}

template<typename ComponentEntityType, componentID component_ID, FixedString component_name>
inline ComponentRawBaseClass<ComponentEntityType, component_ID, component_name>::~ComponentRawBaseClass()
{
    Deinit();
}

template<typename ComponentEntityType, componentID component_ID, FixedString component_name>
inline void ComponentRawBaseClass<ComponentEntityType, component_ID, component_name>::Deinit()
{
    componentEntitiesToRemove.clear();
    componentEntitiesToAdd.clear();

    for (size_t index = 0; index < componentEntitiesRaw.size(); index++)
        if (!isItEmptyRawVector[index])
            RemoveComponentEntity(static_cast<Entity>(index));

    CompleteAddsAndRemoves();
}

template<typename ComponentEntityType, componentID component_ID, FixedString component_name>
inline void ComponentRawBaseClass<ComponentEntityType, component_ID, component_name>::Update()
{
    Update_impl();
}

template<typename ComponentEntityType, componentID component_ID, FixedString component_name>
inline void ComponentRawBaseClass<ComponentEntityType, component_ID, component_name>::Update_impl() requires TrivialUpdatable<ComponentEntityType>
{
    auto component_ptrs = GetComponentPtrsOfArguments(ComponentBaseClass::ecsWrapper_ptr, &ComponentEntityType::Update);

    for (size_t index = 0; index < componentEntitiesRaw.size(); index++)
    {
        if (!isItEmptyRawVector[index])
        {
            auto arguments = TranslateComponentPtrsIntoArguments(static_cast<Entity>(index), component_ptrs);

            std::apply(&ComponentEntityType::Update, std::tuple_cat(std::tie(componentEntitiesRaw[index]), arguments));
        }
    }
}

template<typename ComponentEntityType, componentID component_ID, FixedString component_name>
inline std::vector<std::pair<std::string, MapType>> ComponentRawBaseClass<ComponentEntityType, component_ID, component_name>::GetComponentInitMapFields() const
{
    return ComponentEntityType::GetComponentInitMapFields();
}

template<typename ComponentEntityType, componentID component_ID, FixedString component_name>
inline void ComponentRawBaseClass<ComponentEntityType, component_ID, component_name>::AddComponent(const Entity this_entity, const ComponentEntityType this_componentEntity)
{
    componentEntitiesToAdd.emplace_back(std::make_pair(this_entity, std::move(this_componentEntity)));
}

template<typename ComponentEntityType, componentID component_ID, FixedString component_name>
inline void ComponentRawBaseClass<ComponentEntityType, component_ID, component_name>::AddComponentEntityByMap(const Entity this_entity, const CompEntityInitMap& this_map)
{
    ComponentEntityType&& this_componentEntity = ComponentEntityType::CreateComponentEntityByMap(this_entity, this_map);
    AddComponent(this_entity, this_componentEntity);
}

template<typename ComponentEntityType, componentID component_ID, FixedString component_name>
inline void ComponentRawBaseClass<ComponentEntityType, component_ID, component_name>::RemoveComponentEntity(const Entity this_entity)
{
    componentEntitiesToRemove.emplace_back(this_entity);
}

template<typename ComponentEntityType, componentID component_ID, FixedString component_name>
inline void ComponentRawBaseClass<ComponentEntityType, component_ID, component_name>::CompleteAddsAndRemoves()
{
    {
        for (Entity this_entity : componentEntitiesToRemove)
        {
            assert(!isItEmptyRawVector[this_entity]);
            componentEntitiesRaw[this_entity] = ComponentEntityType::GetEmpty();
            isItEmptyRawVector[this_entity] = true;

            ComponentBaseClass::InformEntitiesHandlerAboutRemoval(this_entity);
        }

        componentEntitiesToRemove.clear();
    }

    {
        componentEntitiesRaw.reserve(componentEntitiesRaw.size() + componentEntitiesToAdd.size());
        isItEmptyRawVector.reserve(componentEntitiesRaw.size() + componentEntitiesToAdd.size());

        for (std::pair<Entity, ComponentEntityType>& this_entity_componentEntity_pair : componentEntitiesToAdd)
        {
            Entity& this_entity = this_entity_componentEntity_pair.first;
            ComponentEntityType& this_componentEntity = this_entity_componentEntity_pair.second;

            if (componentEntitiesRaw.size() - 1 < static_cast<size_t>(this_entity))
            {
                componentEntitiesRaw.resize(static_cast<size_t>(this_entity + 1), ComponentEntityType::GetEmpty());
                isItEmptyRawVector.resize(static_cast<size_t>(this_entity + 1), true);
            }

            assert(isItEmptyRawVector[this_entity]);

            componentEntitiesRaw[this_entity] = std::move(this_componentEntity);
            isItEmptyRawVector[this_entity] = false;

            ComponentBaseClass::InformEntitiesHandlerAboutAddition(this_entity);

            componentEntitiesToInit.emplace_back(this_entity);
        }

        componentEntitiesToAdd.clear();
    }
}

template<typename ComponentEntityType, componentID component_ID, FixedString component_name>
inline void ComponentRawBaseClass<ComponentEntityType, component_ID, component_name>::InitAdds()
{
    for (Entity this_entity : componentEntitiesToInit)
    {
        componentEntitiesRaw[this_entity].Init();
    }

    componentEntitiesToInit.clear();
}


template<typename ComponentEntityType, componentID component_ID, FixedString component_name>
inline ComponentEntityPtr ComponentRawBaseClass<ComponentEntityType, component_ID, component_name>::GetComponentEntityVoidPtr(const Entity this_entity)
{
    assert(componentEntitiesRaw.size() > this_entity);
    assert(!isItEmptyRawVector[this_entity]);
    return reinterpret_cast<ComponentEntityPtr>(&componentEntitiesRaw[this_entity]);
}
