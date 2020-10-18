#pragma once

#include "ECS/ComponentBaseWrappedClass.h"
#include "ECS/TemplateHelpers.h"

#include <unordered_map>
#include <set>
#include <utility>

template<typename ComponentEntityType, componentID component_ID, FixedString component_name>
class ComponentSparseBaseClass :
    public ComponentBaseWrappedClass<ComponentEntityType, component_ID, component_name>
{
public:
    ComponentSparseBaseClass(ECSwrapper* const in_ecs_wrapper_ptr);
    ~ComponentSparseBaseClass() override;
    void Deinit() override;

    void Update() override;

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
    std::set<Entity> entitiesOfComponent_set;
    std::unordered_map<Entity, size_t> entityToIndexToVector_umap;
    std::vector<ComponentEntityType> componentEntitiesSparse;

private:
    std::vector<Entity> componentEntitiesToRemove;

    std::vector<std::pair<Entity, ComponentEntityType>> componentEntitiesToAdd;
    std::vector<Entity> componentEntitiesToInit;
};


// -----SOURCE-----

template<typename ComponentEntityType, componentID component_ID, FixedString component_name>
inline ComponentSparseBaseClass<ComponentEntityType, component_ID, component_name>::ComponentSparseBaseClass(ECSwrapper* const in_ecs_wrapper_ptr)
    :ComponentBaseWrappedClass<ComponentEntityType, component_ID, component_name>::ComponentBaseWrappedClass(in_ecs_wrapper_ptr)
{}

template<typename ComponentEntityType, componentID component_ID, FixedString component_name>
inline ComponentSparseBaseClass<ComponentEntityType, component_ID, component_name>::~ComponentSparseBaseClass()
{
    Deinit();
}

template<typename ComponentEntityType, componentID component_ID, FixedString component_name>
inline void ComponentSparseBaseClass<ComponentEntityType, component_ID, component_name>::Deinit()
{
    componentEntitiesToRemove.clear();
    componentEntitiesToAdd.clear();

    for (auto& this_entity_index_pair : entityToIndexToVector_umap)
        RemoveComponentEntity(this_entity_index_pair.first);

    CompleteAddsAndRemoves();
}

template <typename ComponentEntityType, componentID component_ID, FixedString component_name>
void ComponentSparseBaseClass<ComponentEntityType, component_ID, component_name>::Update()
{
    Update_impl();
}

template <typename ComponentEntityType, componentID component_ID, FixedString component_name>
void ComponentSparseBaseClass<ComponentEntityType, component_ID, component_name>::Update_impl() requires TrivialUpdatable<ComponentEntityType>
{
    auto component_ptrs = GetComponentPtrsOfArguments(ComponentBaseClass::ecsWrapper_ptr, &ComponentEntityType::Update);

    for (size_t index = 0; index < componentEntitiesSparse.size(); index++)
    {
        auto arguments = TranslateComponentPtrsIntoArguments(static_cast<Entity>(componentEntitiesSparse[index].thisEntity), component_ptrs);

        std::apply(&ComponentEntityType::Update, std::tuple_cat(std::tie(componentEntitiesSparse[index]), arguments));
    }
}

template<typename ComponentEntityType, componentID component_ID, FixedString component_name>
inline void ComponentSparseBaseClass<ComponentEntityType, component_ID, component_name>::AddComponent(const Entity this_entity, const ComponentEntityType this_componentEntity)
{
    componentEntitiesToAdd.emplace_back(std::make_pair(this_entity, std::move(this_componentEntity)));
}

template<typename ComponentEntityType, componentID component_ID, FixedString component_name>
inline void ComponentSparseBaseClass<ComponentEntityType, component_ID, component_name>::AddComponentEntityByMap(const Entity this_entity, const CompEntityInitMap& this_map)
{
    ComponentEntityType&& this_componentEntity = ComponentEntityType::CreateComponentEntityByMap(this_entity, this_map);
    AddComponent(this_entity, this_componentEntity);
}

template<typename ComponentEntityType, componentID component_ID, FixedString component_name>
inline void ComponentSparseBaseClass<ComponentEntityType, component_ID, component_name>::RemoveComponentEntity(const Entity this_entity)
{
    componentEntitiesToRemove.emplace_back(this_entity);
}

template<typename ComponentEntityType, componentID component_ID, FixedString component_name>
inline void ComponentSparseBaseClass<ComponentEntityType, component_ID, component_name>::CompleteAddsAndRemoves()
{
    {
        for (Entity this_entity : componentEntitiesToRemove)
        {
            const auto search = entityToIndexToVector_umap.find(this_entity);
            assert(search != entityToIndexToVector_umap.end());

            for (auto this_bigger_entity = entitiesOfComponent_set.upper_bound(this_entity); this_bigger_entity != entitiesOfComponent_set.end(); this_bigger_entity++)
                entityToIndexToVector_umap.find(*this_bigger_entity)->second -= 1;

            componentEntitiesSparse.erase(componentEntitiesSparse.begin() + search->second);
            entitiesOfComponent_set.erase(this_entity);
            entityToIndexToVector_umap.erase(this_entity);

            ComponentBaseClass::InformEntitiesHandlerAboutRemoval(this_entity);
        }

        componentEntitiesToRemove.clear();
    }

    {
        componentEntitiesSparse.reserve(componentEntitiesSparse.size() + componentEntitiesToAdd.size());
        entityToIndexToVector_umap.reserve(componentEntitiesSparse.size() + componentEntitiesToAdd.size());

        for (std::pair<Entity, ComponentEntityType>& this_entity_componentEntity_pair : componentEntitiesToAdd)
        {
            Entity& this_entity = this_entity_componentEntity_pair.first;
            ComponentEntityType& this_componentEntity = this_entity_componentEntity_pair.second;

            {   // sanity
                const auto search = entityToIndexToVector_umap.find(this_entity);
                assert(search == entityToIndexToVector_umap.end());
            }

            size_t entity_index;

            auto search_entity_with_index_to_be_stolen = entitiesOfComponent_set.upper_bound(this_entity);
            if (search_entity_with_index_to_be_stolen != entitiesOfComponent_set.end())
            {
                Entity entity_with_index_to_be_stolen = *search_entity_with_index_to_be_stolen;
                auto search_entity_stolenIndex_pair = entityToIndexToVector_umap.find(entity_with_index_to_be_stolen);

                for (auto this_bigger_entity = entitiesOfComponent_set.upper_bound(this_entity); this_bigger_entity != entitiesOfComponent_set.end(); this_bigger_entity++)
                    entityToIndexToVector_umap.find(*this_bigger_entity)->second += 1;

                entity_index = search_entity_stolenIndex_pair->second;
                componentEntitiesSparse.insert(componentEntitiesSparse.begin() + entity_index, std::move(this_componentEntity));
            }
            else
            {
                entity_index = componentEntitiesSparse.size();
                componentEntitiesSparse.emplace_back(std::move(this_componentEntity));
            }

            entitiesOfComponent_set.emplace(this_entity);
            entityToIndexToVector_umap.emplace(this_entity, entity_index);

            ComponentBaseClass::InformEntitiesHandlerAboutAddition(this_entity);

            componentEntitiesToInit.emplace_back(this_entity);
        }

        componentEntitiesToAdd.clear();
    }
}

template<typename ComponentEntityType, componentID component_ID, FixedString component_name>
inline void ComponentSparseBaseClass<ComponentEntityType, component_ID, component_name>::InitAdds()
{
    for (Entity this_entity : componentEntitiesToInit)
    {
        size_t entity_index = entityToIndexToVector_umap.find(this_entity)->second;
        componentEntitiesSparse[entity_index].Init();
    }

    componentEntitiesToInit.clear();
}

template<typename ComponentEntityType, componentID component_ID, FixedString component_name>
inline ComponentEntityPtr ComponentSparseBaseClass<ComponentEntityType, component_ID, component_name>::GetComponentEntityVoidPtr(const Entity this_entity)
{
    const auto search = entityToIndexToVector_umap.find(this_entity);
    assert(search != entityToIndexToVector_umap.end());

    return reinterpret_cast<ComponentEntityPtr>(&componentEntitiesSparse[search->second]);
}
