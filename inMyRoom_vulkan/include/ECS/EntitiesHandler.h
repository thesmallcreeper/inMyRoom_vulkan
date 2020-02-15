#pragma once

#include <vector>
#include <unordered_map>
#include <set>

#include "ECS/ECStypes.h"

class EntitiesHandler
{
public:
    EntitiesHandler();
    ~EntitiesHandler();

    Entity CreateEntity();
    Entity CreateEntityWithParent(Entity parent);
    void   DeleteEmptyEntity(Entity this_entity);

    void   AddEntityName(Entity this_entity, std::string name);
    Entity FindEntityByName(std::string name);
    Entity FindEntityByRelativeName(std::string name, Entity relative_entity);      // "_root/.." points to the root
    std::string GetEntityName(Entity this_entity);

    std::vector<componentID> GetComponentsOfEntity(Entity this_entity);
    std::vector<Entity> GetChildrenOfEntity(Entity this_entity);
    Entity GetParentOfEntity(Entity this_entity);

    // Callbacks
    void EntityAttachedTo(Entity this_entity, componentID at_component);
    void EntityDeattachFrom(Entity this_entity, componentID at_component);

private: // Data
    std::vector<Entity> parentOfEachEntity;
    std::vector<std::set<Entity>> childrenOfEachEntity;
    std::vector<std::vector<componentID>> componentsOfEachEntity;

    std::vector<Entity> entitiesRecycleBin;

    std::unordered_map<std::string, Entity> nameToEntity_umap;
    std::unordered_map<Entity, std::string> entityToName_umap;
};

