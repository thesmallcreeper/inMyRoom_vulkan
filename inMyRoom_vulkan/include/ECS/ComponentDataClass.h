#pragma once

#include "ECS/ComponentBaseWrappedClass.h"
#include "ECS/TemplateHelpers.h"

#include <vector>

template<typename ComponentEntityType, componentID component_ID, FixedString component_name,
         template<typename _I, typename _D, typename _D_B, _I _D_B::*> typename data_set>
class ComponentDataClass :
    public ComponentBaseWrappedClass<ComponentEntityType, component_ID, component_name, data_set>
{
public:
    explicit ComponentDataClass(ECSwrapper* in_ecs_wrapper_ptr);
    ~ComponentDataClass() override;
    void Deinit() override;

    void Update() override { Update_impl(); }
    
    size_t PushBackNewFab() override;
    void AddCompEntityAtLatestFab(Entity entity, const std::string& fab_path, const CompEntityInitMap& init_map) override;
    std::pair<Entity, Entity> GetLatestFabRange() override;

    DataSetPtr InitializeFab(Entity offset, size_t fab_index) override;
    void RemoveInstancesByRanges(const std::vector<std::pair<Entity, Entity>>& ranges) override;

    void AddInitializedFabs() override;
    void NewUpdateSession() override;

protected:
    data_set<Entity, ComponentEntityType, CompEntityBaseClass, &CompEntityBaseClass::thisEntity>& GetContainerByIndex(size_t index);
    size_t GetContainersCount() const;

private:
    ComponentEntityPtr GetComponentEntityVoidPtr(Entity this_entity, size_t index_hint) override;

    void Update_impl() requires TrivialUpdatable<ComponentEntityType>;
    void Update_impl() { assert(not Updateable<ComponentEntityType>); };

protected:
    data_set<Entity, ComponentEntityType, CompEntityBaseClass, &CompEntityBaseClass::thisEntity>  componentEntities;
    std::vector<data_set<Entity, ComponentEntityType, CompEntityBaseClass, &CompEntityBaseClass::thisEntity>> initialized_fabs;

    size_t containersUpdated = 0;

private:
    std::vector<data_set<Entity, ComponentEntityType, CompEntityBaseClass, &CompEntityBaseClass::thisEntity>> fabs;
};


// -----SOURCE-----

template<typename ComponentEntityType, componentID component_ID, FixedString component_name,
         template<typename _I, typename _D, typename _D_B, _I _D_B::*> typename data_set>
ComponentDataClass<ComponentEntityType, component_ID, component_name, data_set>::ComponentDataClass(ECSwrapper* in_ecs_wrapper_ptr)
    :ComponentBaseWrappedClass<ComponentEntityType, component_ID, component_name, data_set>::ComponentBaseWrappedClass(in_ecs_wrapper_ptr)
{
}

template<typename ComponentEntityType, componentID component_ID, FixedString component_name,
         template<typename _I, typename _D, typename _D_B, _I _D_B::*> typename data_set>
ComponentDataClass<ComponentEntityType, component_ID, component_name, data_set>::~ComponentDataClass()
{
    Deinit();
}

template<typename ComponentEntityType, componentID component_ID, FixedString component_name,
         template<typename _I, typename _D, typename _D_B, _I _D_B::*> typename data_set>
void ComponentDataClass<ComponentEntityType, component_ID, component_name, data_set>::Deinit()
{
    fabs.clear();
    initialized_fabs.clear();
    componentEntities.deinit();
}

template<typename ComponentEntityType, componentID component_ID, FixedString component_name,
         template<typename _I, typename _D, typename _D_B, _I _D_B::*> typename data_set>
size_t ComponentDataClass<ComponentEntityType, component_ID, component_name, data_set>::PushBackNewFab()
{
    fabs.emplace_back();
    return fabs.size() - 1;
}

template<typename ComponentEntityType, componentID component_ID, FixedString component_name,
         template<typename _I, typename _D, typename _D_B, _I _D_B::*> typename data_set>
void ComponentDataClass<ComponentEntityType, component_ID, component_name, data_set>::AddCompEntityAtLatestFab(Entity entity,
                                                                                                               const std::string& fab_path,
                                                                                                               const CompEntityInitMap& init_map)
{
    ComponentEntityType comp_entity = ComponentEntityType::CreateComponentEntityByMap(entity, fab_path, init_map);
    fabs.back().add_element(comp_entity);
}

template<typename ComponentEntityType, componentID component_ID, FixedString component_name,
         template<typename _I, typename _D, typename _D_B, _I _D_B::*> typename data_set>
std::pair<Entity, Entity> ComponentDataClass<ComponentEntityType, component_ID, component_name, data_set>::GetLatestFabRange()
{
    if(fabs.back().size() != 0)
    {
        Entity min_entity = -1; // first entity
        Entity max_entity = 0;  // last entity

        for(const auto& this_comp_entity: fabs.back())
        {
            min_entity = std::min(min_entity, this_comp_entity.thisEntity);
            max_entity = std::max(max_entity, this_comp_entity.thisEntity);
        }

        return std::make_pair(min_entity, max_entity);
    }
    else
    {
        return std::make_pair<Entity, Entity>(-1, -1);
    }
}

template<typename ComponentEntityType, componentID component_ID, FixedString component_name,
         template<typename _I, typename _D, typename _D_B, _I _D_B::*> typename data_set>
DataSetPtr ComponentDataClass<ComponentEntityType, component_ID, component_name, data_set>::InitializeFab(Entity offset,
                                                                                                          size_t fab_index)
{
    auto& ref = initialized_fabs.emplace_back(fabs[fab_index], offset);
    return static_cast<DataSetPtr>(&ref);
}

template<typename ComponentEntityType, componentID component_ID, FixedString component_name,
         template<typename _I, typename _D, typename _D_B, _I _D_B::*> typename data_set>
void ComponentDataClass<ComponentEntityType, component_ID, component_name, data_set>::RemoveInstancesByRanges(const std::vector<std::pair<Entity, Entity>>& ranges)
{
    componentEntities.remove_oneshot_added_ranges(ranges);
}

template<typename ComponentEntityType, componentID component_ID, FixedString component_name,
         template<typename _I, typename _D, typename _D_B, _I _D_B::*> typename data_set>
void ComponentDataClass<ComponentEntityType, component_ID, component_name, data_set>::AddInitializedFabs()
{
    // Don't add deleted component entities
    std::erase_if(initialized_fabs,
                  [this](const auto& this_inited_fab)
                  {
                        if (this_inited_fab.size() && this->entitiesHandler_ptr->GetInstanceInfo(this_inited_fab.begin()->thisEntity))
                            return false;
                        else
                            return true;
                  });

    componentEntities.add_elements_list(std::make_move_iterator(initialized_fabs.begin()),
                                        std::make_move_iterator(initialized_fabs.end()));

    initialized_fabs.clear();
}

template<typename ComponentEntityType, componentID component_ID, FixedString component_name,
         template<typename _I, typename _D, typename _D_B, _I _D_B::*> typename data_set>
ComponentEntityPtr ComponentDataClass<ComponentEntityType, component_ID, component_name, data_set>::GetComponentEntityVoidPtr(Entity this_entity, size_t index_hint)
{
    auto& container_by_hint = GetContainerByIndex(index_hint);
    if(container_by_hint.does_exist(this_entity))
        return static_cast<ComponentEntityPtr>(&container_by_hint[this_entity]);
    else
    {
        size_t actual_index = this->entitiesHandler_ptr->GetContainerIndexOfEntity(this_entity);
        auto& actual_container = GetContainerByIndex(actual_index);
        return static_cast<ComponentEntityPtr>(&actual_container[this_entity]);
    }
}

template<typename ComponentEntityType, componentID component_ID, FixedString component_name,
        template<typename _I, typename _D, typename _D_B, _I _D_B::*> typename data_set>
data_set<Entity, ComponentEntityType, CompEntityBaseClass, &CompEntityBaseClass::thisEntity> &
ComponentDataClass<ComponentEntityType, component_ID, component_name, data_set>::GetContainerByIndex(size_t index)
{
    if(index == 0)
        return componentEntities;
    else
    {
        size_t vector_index = index - 1;

        assert(vector_index < initialized_fabs.size());
        return initialized_fabs[vector_index];
    }
}

template<typename ComponentEntityType, componentID component_ID, FixedString component_name,
        template<typename _I, typename _D, typename _D_B, _I _D_B::*> typename data_set>
void ComponentDataClass<ComponentEntityType, component_ID, component_name, data_set>::NewUpdateSession()
{
    containersUpdated = 0;
}

template<typename ComponentEntityType, componentID component_ID, FixedString component_name,
        template<typename _I, typename _D, typename _D_B, _I _D_B::*> typename data_set>
size_t ComponentDataClass<ComponentEntityType, component_ID, component_name, data_set>::GetContainersCount() const
{
    return initialized_fabs.size() + 1;
}

template<typename ComponentEntityType, componentID component_ID, FixedString component_name,
        template<typename _I, typename _D, typename _D_B, _I _D_B::*> typename data_set>
void ComponentDataClass<ComponentEntityType, component_ID, component_name, data_set>::Update_impl() requires TrivialUpdatable<ComponentEntityType>
{
    auto component_ptrs = GetComponentPtrsOfArguments(ComponentBaseClass::ecsWrapper_ptr, &ComponentEntityType::Update);

    size_t containers_count_when_start = GetContainersCount();
    for(; containersUpdated != containers_count_when_start; ++containersUpdated)
    {
        auto& this_container = GetContainerByIndex(containersUpdated);
        for(auto& this_comp_entity: this_container)
        {
            auto arguments = TranslateComponentPtrsIntoArguments(static_cast<Entity>(this_comp_entity.thisEntity), containersUpdated, component_ptrs);

            std::apply(&ComponentEntityType::Update, std::tuple_cat(std::tie(this_comp_entity), arguments));
        }
    }
}
