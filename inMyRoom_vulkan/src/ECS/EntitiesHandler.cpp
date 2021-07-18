#include "ECS/EntitiesHandler.h"

#include <cassert>
#include <algorithm>

EntitiesHandler::EntitiesHandler()
{
    InstanceInfo* root_instance_ptr = new InstanceInfo();
    root_instance_ptr->instanceName = "_default";
    root_instance_ptr->size = 1;

    parentOfEachEntity.emplace_back(0);
    instancePtrOfEachEntity.emplace_back(root_instance_ptr);
    nameToInstancePtr_umap.emplace(root_instance_ptr->instanceName, root_instance_ptr);

    availableRanges.emplace(size_t(-1) - 1, std::pair<Entity, Entity>(1, -1));
}

EntitiesHandler::~EntitiesHandler()
{
    for(auto& _this_pair: nameToInstancePtr_umap)
    {
        delete _this_pair.second;
    }
}

InstanceInfo* EntitiesHandler::AddInstanceEntities(const FabInfo* fab_info_ptr,
                                                   Entity parent)
{
    std::string instance_name = fab_info_ptr->fabName + "_" + std::to_string(noNameInstancesSoFar++);
    return AddInstanceEntities(fab_info_ptr, std::move(instance_name), parent);
}

InstanceInfo* EntitiesHandler::AddInstanceEntities(const FabInfo* fab_info_ptr,
                                                   const std::string& instance_name,
                                                   Entity parent)
{
    assert(instancePtrOfEachEntity[parent] != nullptr);
    assert(nameToInstancePtr_umap.find(instance_name) == nameToInstancePtr_umap.end());

    InstanceInfo* const instance_info_ptr = new InstanceInfo();

    instance_info_ptr->fabInfo = fab_info_ptr;
    instance_info_ptr->instanceName = instance_name;
    instance_info_ptr->size = instance_info_ptr->fabInfo->size;

    const std::pair<Entity, Entity> range = GetRange(instance_info_ptr->size);

    instance_info_ptr->entityOffset = range.first;

    ExtentVectors(range.second);

    InstanceInfo* const parent_instance_info_ptr = instancePtrOfEachEntity[parent];
    parent_instance_info_ptr->instanceChildren.emplace(instance_info_ptr);
    instance_info_ptr->parent_instance = parent_instance_info_ptr;

    parentOfEachEntity[range.first] = parent;
    for(size_t index = 1; index != instance_info_ptr->size; index++)
    {
        parentOfEachEntity[index + range.first] = instance_info_ptr->fabInfo->entitiesParents[index] + range.first;
    }

    for(size_t index = 0; index != instance_info_ptr->size; index++)
    {
        instancePtrOfEachEntity[index + range.first] = instance_info_ptr;
    }

    nameToInstancePtr_umap.emplace(instance_name, instance_info_ptr);

    return instance_info_ptr;
}

InstanceInfo* EntitiesHandler::GetInstanceInfo(Entity entity)
{
    InstanceInfo* instance_ptr = instancePtrOfEachEntity[entity];
    assert(instance_ptr != nullptr);

    return instance_ptr;
}

InstanceInfo* EntitiesHandler::GetInstanceInfo(const std::string& name)
{
    auto search = nameToInstancePtr_umap.find(name);
    assert(search != nameToInstancePtr_umap.end());

    return search->second;
}

void EntitiesHandler::RemoveInstancesEntities(const std::set<InstanceInfo*>& instance_to_remove_ptrs_set)
{
    if(instance_to_remove_ptrs_set.empty())
        return;

    std::vector<std::pair<Entity, Entity>> ranges_to_become_available;
    ranges_to_become_available.reserve(instance_to_remove_ptrs_set.size());

    for(const auto& this_instance_ptr: instance_to_remove_ptrs_set)
    {
        InstanceInfo* parent_instance_info_ptr = this_instance_ptr->parent_instance;

        {
            auto search = parent_instance_info_ptr->instanceChildren.find(this_instance_ptr);
            assert(search != parent_instance_info_ptr->instanceChildren.end());

            parent_instance_info_ptr->instanceChildren.erase(search);
        }

        for(size_t index = this_instance_ptr->entityOffset; index != this_instance_ptr->entityOffset + this_instance_ptr->size; ++index)
        {
            parentOfEachEntity[index] = -1;
            instancePtrOfEachEntity[index] = nullptr;
        }

        {
            auto search = nameToInstancePtr_umap.find(this_instance_ptr->instanceName);
            assert(search != nameToInstancePtr_umap.end());

            nameToInstancePtr_umap.erase(search);
        }

        ranges_to_become_available.emplace_back(Entity(this_instance_ptr->entityOffset), Entity(this_instance_ptr->entityOffset + this_instance_ptr->size - 1));     
    }

    for(const auto& this_instance_ptr: instance_to_remove_ptrs_set)
    {
        assert(!this_instance_ptr->instanceChildren.size());
        delete this_instance_ptr;
    }

    AddAvailableRanges(std::move(ranges_to_become_available));
}

Entity EntitiesHandler::FindEntityByPath(const std::string& instance_name, const std::string& fab_path) const
{
    auto search = nameToInstancePtr_umap.find(instance_name);
    assert(search != nameToInstancePtr_umap.end());

    return FindEntityByPath(search->second, fab_path);
}

Entity EntitiesHandler::FindEntityByPath(const InstanceInfo* instance_ptr, const std::string& fab_path) const
{
    auto search = instance_ptr->fabInfo->nameToEntity.find(fab_path);
    assert(search != instance_ptr->fabInfo->nameToEntity.end());

    Entity fab_entity = search->second;

    return fab_entity + instance_ptr->entityOffset;
}

std::pair<std::string, std::string> EntitiesHandler::GetEntityName(Entity entity) const
{
    assert(instancePtrOfEachEntity[entity] != nullptr);

    const InstanceInfo* instance_info_ptr = instancePtrOfEachEntity[entity];
    Entity fab_entity = entity - instance_info_ptr->entityOffset;

    auto search = instance_info_ptr->fabInfo->entityToName.find(fab_entity);
    assert(search != instance_info_ptr->fabInfo->entityToName.end());

    std::string fab_name = search->second;
    std::string instance_name = instance_info_ptr->instanceName;

    return std::make_pair<std::string, std::string>(std::move(instance_name), std::move(fab_name));
}

std::vector<Entity> EntitiesHandler::GetEntityAncestors(Entity entity) const
{
    assert(entity != 0);

    std::vector<Entity> return_vector;

    Entity this_entity = entity;
    while(true)
    {
        return_vector.emplace_back(this_entity);
        if(this_entity == 0) break;

        this_entity = GetParentOfEntity(this_entity);
    }

    std::reverse(return_vector.begin(), return_vector.end());

    return return_vector;
}

Entity EntitiesHandler::GetParentOfEntity(Entity entity) const
{
    assert(parentOfEachEntity[entity] != Entity(-1));
    return parentOfEachEntity[entity];
}

void EntitiesHandler::ChangeParentOfInstance(InstanceInfo* instance_ptr, Entity new_parent)
{
    {   // Remove old
        InstanceInfo* old_parent_instance_info_ptr = instance_ptr->parent_instance;

        auto search = old_parent_instance_info_ptr->instanceChildren.find(instance_ptr);
        assert(search != old_parent_instance_info_ptr->instanceChildren.end());

        old_parent_instance_info_ptr->instanceChildren.erase(search);
    }

    {   // Add new
        InstanceInfo* new_parent_instance_info_ptr = GetInstanceInfo(new_parent);
        new_parent_instance_info_ptr->instanceChildren.emplace(instance_ptr);

        instance_ptr->parent_instance = new_parent_instance_info_ptr;
        parentOfEachEntity[instance_ptr->entityOffset] = Entity(new_parent);
    }
}

std::pair<Entity, Entity> EntitiesHandler::GetRange(size_t size)
{
    auto search_bigger_than_size = availableRanges.lower_bound(size);
    assert(search_bigger_than_size != availableRanges.end());

    std::pair<Entity, Entity> free_range = search_bigger_than_size->second;

    std::pair<Entity, Entity> return_range = std::pair<Entity, Entity>(free_range.first, Entity(free_range.first + size - 1));
    std::pair<Entity, Entity> unused_range = std::pair<Entity, Entity>(Entity(free_range.first + size), free_range.second);

    size_t unused_range_size = unused_range.second - unused_range.first + 1;

    availableRanges.erase(search_bigger_than_size);
    if(unused_range_size != 0)
    {
        availableRanges.emplace(unused_range_size, unused_range);
    }

    return return_range;
}

void EntitiesHandler::AddAvailableRanges(std::vector<std::pair<Entity, Entity>>&& new_ranges)
{
    if(new_ranges.empty())
        return;

    std::vector<std::pair<Entity, Entity>> ranges = std::move(new_ranges);
    ranges.reserve(ranges.size() + availableRanges.size());

    for(auto& this_availabe_range: availableRanges)
        ranges.emplace_back(this_availabe_range.second);

    availableRanges.clear();

    std::sort(ranges.begin(), ranges.end(), [](const auto& a, const auto& b) {return a.first < b.first;});

    {
        std::pair<Entity, Entity> latest_continue_range = ranges.front();
        for(auto range_it = ranges.begin() + 1; range_it != ranges.end(); ++range_it)
        {
            if(latest_continue_range.second + 1 == range_it->first)
            {
                latest_continue_range.second = range_it->second;
            }
            else
            {
                availableRanges.emplace(latest_continue_range.second - latest_continue_range.first + 1, latest_continue_range);
                latest_continue_range = *range_it;
            }
        }
        availableRanges.emplace(latest_continue_range.second - latest_continue_range.first + 1, latest_continue_range);
    }
}

void EntitiesHandler::ExtentVectors(size_t max_index)
{
    if(max_index > parentOfEachEntity.size() - 1)
    {
        parentOfEachEntity.resize(max_index + 1, -1);
        instancePtrOfEachEntity.resize(max_index + 1, nullptr);
    }
}