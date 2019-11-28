#include "EntitiesHandler.h"

#include <cassert>

EntitiesHandler::EntitiesHandler()
{
    // entityID is null;
    componentsOfEachEntity.emplace_back(std::vector<componentID>());
    nameToEntity_umap.emplace("", 0);
    entityToName_umap.emplace(0, "");
}

EntitiesHandler::~EntitiesHandler()
{
    for (size_t index = 0; index < componentsOfEachEntity.size(); index++)
        assert(componentsOfEachEntity[index].empty());
}

Entity EntitiesHandler::CreateEntity()
{
    Entity return_entity = 0;
    if (!entitiesRecycleBin.empty())
    {
        return_entity = entitiesRecycleBin[0];
        entitiesRecycleBin.erase(entitiesRecycleBin.begin() + 0);
    }
    else
    {
        return_entity = static_cast<Entity>(componentsOfEachEntity.size());
        componentsOfEachEntity.emplace_back(std::vector<componentID>());
    }

    return return_entity;
}

Entity EntitiesHandler::CreateEntityWithParent(Entity parent)
{
    Entity return_entity = 0;
    if (!entitiesRecycleBin.empty())
    {
        for(size_t index = 0; index < entitiesRecycleBin.size(); index++)
            if (entitiesRecycleBin[index] > parent)
            {
                return_entity = entitiesRecycleBin[index];
                entitiesRecycleBin.erase(entitiesRecycleBin.begin() + index);

                break;
            }
    }

    if (return_entity == 0)
    {
        return_entity = static_cast<Entity>(componentsOfEachEntity.size());
        componentsOfEachEntity.emplace_back(std::vector<componentID>());
    }

    assert(return_entity > parent);

    return return_entity;
}

Entity EntitiesHandler::CreateEntityAtEnd()
{
    Entity return_entity = 0;
    return_entity = static_cast<Entity>(componentsOfEachEntity.size());
    componentsOfEachEntity.emplace_back(std::vector<componentID>());

    return return_entity;
}

void EntitiesHandler::DeleteEmptyEntity(Entity this_entity)
{
    assert(componentsOfEachEntity[this_entity].empty());

    entitiesRecycleBin.emplace_back(this_entity);

    auto search = entityToName_umap.find(this_entity);
    if (search != entityToName_umap.end())
    {
        nameToEntity_umap.erase(search->second);
        entityToName_umap.erase(search->first);
    }
}

void EntitiesHandler::AddEntityName(Entity this_entity, std::string name)
{
    assert(nameToEntity_umap.find(name) == nameToEntity_umap.end());

    nameToEntity_umap.emplace(name, this_entity);
    entityToName_umap.emplace(this_entity, name);
}

Entity EntitiesHandler::FindEntityByName(std::string name)
{
    assert(nameToEntity_umap.find(name) != nameToEntity_umap.end());

    auto search = nameToEntity_umap.find(name);
    Entity return_entity = search->second;

    return return_entity;
}

Entity EntitiesHandler::FindEntityByRelativeName(std::string relative_name, Entity relative_entity)
{
    std::string name;

    if (relative_name.substr(0, relative_name.find_first_of("/")) == "_main")
    {
        std::string name_of_relative_entity = GetEntityName(relative_entity);

        while (relative_name.substr(0, relative_name.find_first_of("/")) == "..")
        {
            assert(name_of_relative_entity.find_last_of("/") != std::string::npos);
            name_of_relative_entity = name_of_relative_entity.substr(0, name_of_relative_entity.find_last_of("/"));

            if (relative_name.find_first_of("/") != std::string::npos)
                relative_name = relative_name.substr(relative_name.find_first_of("/") + 1);
            else
                relative_name = "";
        }

        if (relative_name != "")
            name = name_of_relative_entity + "/" + relative_name;
        else
            name = name_of_relative_entity;
    }
    else
        name = relative_name.substr(relative_name.find_first_of("/") + 1);

    return FindEntityByName(name);
}

std::string EntitiesHandler::GetEntityName(Entity this_entity)
{
    assert(entityToName_umap.find(this_entity) != entityToName_umap.end());

    auto search = entityToName_umap.find(this_entity);
    std::string return_name = search->second;

    return return_name;
}

std::vector<componentID> EntitiesHandler::GetComponentsOfEntity(Entity this_entity)
{
    for (size_t index = 0; index < entitiesRecycleBin.size(); index++)
        assert(entitiesRecycleBin[index] != this_entity);

    assert(this_entity < componentsOfEachEntity.size());

    return componentsOfEachEntity[this_entity];
}

void EntitiesHandler::EntityAttachedTo(Entity this_entity, componentID at_component)
{
    for (size_t index = 0; index < entitiesRecycleBin.size(); index++)
        assert(entitiesRecycleBin[index] != this_entity);

    assert(this_entity < componentsOfEachEntity.size());

    auto search = std::find(componentsOfEachEntity[this_entity].begin(), componentsOfEachEntity[this_entity].end(), at_component);
    assert(search == componentsOfEachEntity[this_entity].end());

    componentsOfEachEntity[this_entity].emplace_back(at_component);
}

void EntitiesHandler::EntityDeattachFrom(Entity this_entity, componentID at_component)
{
    for (size_t index = 0; index < entitiesRecycleBin.size(); index++)
        assert(entitiesRecycleBin[index] != this_entity);

    assert(this_entity < componentsOfEachEntity.size());

    auto search = std::find(componentsOfEachEntity[this_entity].begin(), componentsOfEachEntity[this_entity].end(), at_component);
    assert(search != componentsOfEachEntity[this_entity].end());

    componentsOfEachEntity[this_entity].erase(search);
}