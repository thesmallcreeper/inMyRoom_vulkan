#pragma once

#include "ECS/ComponentBaseWrappedClass.h"
#include "ECS/TemplateHelpers.h"

#include <vector>
#include <list>

template<typename ComponentEntityType, componentID component_ID, FixedString component_name,
         template<typename _I, typename _D, typename _D_B, _I _D_B::*> typename data_set>
class ComponentDataClass :
    public ComponentBaseWrappedClass<ComponentEntityType, component_ID, component_name, data_set>
{
public:
    ComponentDataClass(ECSwrapper* in_ecs_wrapper_ptr);
    ~ComponentDataClass() override;
    void Deinit() override;

    void Update() override {} // TODO template tricks                                                                             
    
    size_t PushBackNewFab() override;
    void AddCompEntityAtLatestFab(Entity entity, const std::string& fab_path, const CompEntityInitMap& init_map) override;
    std::pair<Entity, Entity> GetLatestFabRange() override;

    DataSetPtr InitializeFab(Entity offset, size_t fab_index) override;
    void RemoveInstancesByRanges(const std::vector<std::pair<Entity, Entity>>& ranges) override;

    void AddInitializedFabs() override;

private:
    ComponentEntityPtr GetComponentEntityVoidPtr(Entity this_entity) override;

protected:
    data_set<Entity, ComponentEntityType, CompEntityBaseClass, &CompEntityBaseClass::thisEntity>  componentEntities;

private:
    std::vector<data_set<Entity, ComponentEntityType, CompEntityBaseClass, &CompEntityBaseClass::thisEntity>> fabs;
    std::list<data_set<Entity, ComponentEntityType, CompEntityBaseClass, &CompEntityBaseClass::thisEntity>> initialized_fabs;

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
        return std::pair<Entity, Entity>(-1, -1);
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
    if constexpr(HasInitMethod<ComponentEntityType>)
    {
        for(auto& this_range: initialized_fabs)
        {
            for(auto& _this: this_range)
            {
                _this.Init();
            }
        }
    }

    componentEntities.add_elements_list(std::make_move_iterator(initialized_fabs.begin()),
                                        std::make_move_iterator(initialized_fabs.end()));

    initialized_fabs.clear();
}

template<typename ComponentEntityType, componentID component_ID, FixedString component_name,
         template<typename _I, typename _D, typename _D_B, _I _D_B::*> typename data_set>
ComponentEntityPtr ComponentDataClass<ComponentEntityType, component_ID, component_name, data_set>::GetComponentEntityVoidPtr(Entity this_entity)
{
    return static_cast<ComponentEntityPtr>(&componentEntities[this_entity]);
}