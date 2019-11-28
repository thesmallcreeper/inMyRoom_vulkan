#pragma once

#include <vector>
#include <unordered_map>

#include "ECStypes.h"

class EntitiesHandler
{
public:
    EntitiesHandler();
    ~EntitiesHandler();

    Entity CreateEntity();
    Entity CreateEntityWithParent(Entity parent);
    Entity CreateEntityAtEnd();
    void   DeleteEmptyEntity(Entity this_entity);

    void   AddEntityName(Entity this_entity, std::string name);
    Entity FindEntityByName(std::string name);
    Entity FindEntityByRelativeName(std::string name, Entity relative_entity);
    std::string GetEntityName(Entity this_entity);

    std::vector<componentID> GetComponentsOfEntity(Entity this_entity);

    // Callbacks
    void EntityAttachedTo(Entity this_entity, componentID at_component);
    void EntityDeattachFrom(Entity this_entity, componentID at_component);

private: // Data
    std::vector<std::vector<componentID>> componentsOfEachEntity;
    std::vector<Entity> entitiesRecycleBin;

    std::unordered_map<std::string, Entity> nameToEntity_umap;
    std::unordered_map<Entity, std::string> entityToName_umap;
};

