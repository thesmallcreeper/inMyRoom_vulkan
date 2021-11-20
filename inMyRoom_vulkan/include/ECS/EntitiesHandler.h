#pragma once

#include <vector>
#include <unordered_map>
#include <map>
#include <unordered_set>

#include "ECS/ECStypes.h"

class EntitiesHandler
{
public:
    EntitiesHandler();
    ~EntitiesHandler();

    InstanceInfo* AddInstanceEntities(const FabInfo* fab_info_ptr, Entity parent = 0);
    InstanceInfo* AddInstanceEntities(const FabInfo* fab_info_ptr, const std::string& instance_name, Entity parent = 0);

    InstanceInfo* GetInstanceInfo(Entity entity);
    InstanceInfo* GetInstanceInfo(const std::string& name);

    void RemoveInstancesEntities(const std::unordered_set<InstanceInfo*>& instance_to_remove_ptrs_set);
    void AdditionsCompleted();

    Entity FindEntityByPath(const std::string& instance_name, const std::string& fab_path) const;
    Entity FindEntityByPath(const InstanceInfo* instance_ptr, const std::string& fab_path) const;
    std::pair<std::string, std::string> GetEntityName(Entity entity) const;

    std::vector<Entity> GetEntityAncestors(Entity entity) const;

    Entity GetParentOfEntity(Entity entity) const;
    void ChangeParentOfInstance(InstanceInfo* instance_ptr, Entity new_parent);

    size_t GetContainerIndexOfEntity(Entity entity) const;

private:
    std::pair<Entity, Entity> GetRange(size_t size);
    void AddAvailableRanges(std::vector<std::pair<Entity, Entity>>&& new_ranges);

    void ExtentVectors(size_t max_index);

private: // Data
    std::vector<Entity> parentOfEachEntity;
    std::vector<InstanceInfo*> instancePtrOfEachEntity;
    std::vector<uint16_t> containerIndexOfEachEntity;

    std::multimap<size_t, std::pair<Entity, Entity>> availableRanges;
    std::vector<std::pair<Entity, Entity>> additionsNotCompletedRanges;

    std::unordered_map<std::string, InstanceInfo*> nameToInstancePtr_umap;
    size_t noNameInstancesSoFar = 0;
};

