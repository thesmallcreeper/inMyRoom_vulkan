#pragma once

#include "ECS/ComponentBaseClass.h"

#include <unordered_map>
#include <set>
#include <utility>

template<typename ComponentEntityType>
class ComponentSparseBaseClass :
    public ComponentBaseClass
{
public:
    ComponentSparseBaseClass(const componentID this_ID, std::string this_name, ECSwrapper* const in_ecs_wrapper_ptr);
    virtual ~ComponentSparseBaseClass() override;
    void Deinit() override;

//  void Update() override;
//  void FixedUpdate() override;
//  void AsyncInput(InputType input_type, void* struct_data = nullptr) override;

    std::vector<std::pair<std::string, MapType>> GetComponentInitMapFields() const override;

    void AddComponent(const Entity this_entity, const ComponentEntityType this_componentEntity);                // Component entity specific task
    void AddComponentEntityByMap(const Entity this_entity, const CompEntityInitMap& this_map) override;         // Component entity specific task
    void RemoveComponentEntity(const Entity this_entity) override;                                              // Component entity memory specific task
    void CompleteAddsAndRemoves() override;                                                                     // Component entity memory specific task
    void InitAdds() override;                                                                                   // Component entity memory specific task
    ComponentEntityPtr GetComponentEntity(const Entity this_entity) override;                                   // Component entity memory specific task

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

template<typename ComponentEntityType>
inline ComponentSparseBaseClass<ComponentEntityType>::ComponentSparseBaseClass(const componentID this_ID, const std::string this_name, ECSwrapper* const in_ecs_wrapper_ptr)
    :ComponentBaseClass::ComponentBaseClass(this_ID, this_name, in_ecs_wrapper_ptr)
{}

template<typename ComponentEntityType>
ComponentSparseBaseClass<ComponentEntityType>::~ComponentSparseBaseClass()
{
    Deinit();
}

template<typename ComponentEntityType>
void ComponentSparseBaseClass<ComponentEntityType>::Deinit()
{
    componentEntitiesToRemove.clear();
    componentEntitiesToAdd.clear();

    for (auto& this_entity_index_pair : entityToIndexToVector_umap)
        RemoveComponentEntity(this_entity_index_pair.first);

    CompleteAddsAndRemoves();
}

/*
template<typename ComponentEntityType>
void ComponentSparseBaseClass<ComponentEntityType>::Update()
{
    for (ComponentEntityType& this_componentEntity : this_componentEntitiesSparse)
        this_componentEntity.Update();
}

template<typename ComponentEntityType>
void ComponentSparseBaseClass<ComponentEntityType>::FixedUpdate()
{
    for (ComponentEntityType& this_componentEntity : this_componentEntitiesSparse)
        this_componentEntity.FixedUpdate();
}

template<typename ComponentEntityType>
void ComponentSparseBaseClass<ComponentEntityType>::AsyncUpdate()
{
    for (ComponentEntityType& this_componentEntity : this_componentEntitiesSparse)
        this_componentEntity.AsyncUpdate();
}
*/

template<typename ComponentEntityType>
std::vector<std::pair<std::string, MapType>> ComponentSparseBaseClass<ComponentEntityType>::GetComponentInitMapFields() const
{
    return ComponentEntityType::GetComponentInitMapFields();
}

template<typename ComponentEntityType>
void ComponentSparseBaseClass<ComponentEntityType>::AddComponent(const Entity this_entity, const ComponentEntityType this_componentEntity)
{
    componentEntitiesToAdd.emplace_back(std::make_pair(this_entity, std::move(this_componentEntity)));
}

template<typename ComponentEntityType>
void ComponentSparseBaseClass<ComponentEntityType>::AddComponentEntityByMap(const Entity this_entity, const CompEntityInitMap& this_map)
{
    ComponentEntityType&& this_componentEntity = ComponentEntityType::CreateComponentEntityByMap(this_entity, this_map);
    AddComponent(this_entity, this_componentEntity);
}

template<typename ComponentEntityType>
void ComponentSparseBaseClass<ComponentEntityType>::RemoveComponentEntity(const Entity this_entity)
{
    componentEntitiesToRemove.emplace_back(this_entity);
}

template<typename ComponentEntityType>
void ComponentSparseBaseClass<ComponentEntityType>::CompleteAddsAndRemoves()
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

            InformEntitiesHandlerAboutRemoval(this_entity);
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

            InformEntitiesHandlerAboutAddition(this_entity);

            componentEntitiesToInit.emplace_back(this_entity);
        }

        componentEntitiesToAdd.clear();
    }
}

template<typename ComponentEntityType>
void ComponentSparseBaseClass<ComponentEntityType>::InitAdds()
{
    for (Entity this_entity : componentEntitiesToInit)
    {
        size_t entity_index = entityToIndexToVector_umap.find(this_entity)->second;
        componentEntitiesSparse[entity_index].Init();
    }

    componentEntitiesToInit.clear();
}

template<typename ComponentEntityType>
ComponentEntityPtr ComponentSparseBaseClass<ComponentEntityType>::GetComponentEntity(const Entity this_entity)
{
    const auto search = entityToIndexToVector_umap.find(this_entity);
    assert(search != entityToIndexToVector_umap.end());

    return reinterpret_cast<ComponentEntityPtr>(&componentEntitiesSparse[search->second]);
}
