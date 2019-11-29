#include "ECS/EntitiesHandler.h"

#include <cassert>
#include <algorithm>

EntitiesHandler::EntitiesHandler()
{
    // entityID is null;
    parentOfEachEntity.emplace_back(-1);
    childrenOfEachEntity.emplace_back(std::set<Entity>());
    componentsOfEachEntity.emplace_back(std::vector<componentID>());
    nameToEntity_umap.emplace("", 0);
    entityToName_umap.emplace(0, "");
}

EntitiesHandler::~EntitiesHandler()
{
    for (size_t index = 0; index < componentsOfEachEntity.size(); index++)
    {
        assert(componentsOfEachEntity[index].empty());
        assert(childrenOfEachEntity[index].empty());
        assert(parentOfEachEntity[index] == -1);
    }
}

Entity EntitiesHandler::CreateEntity()
{
    return CreateEntityWithParent(0);
}

Entity EntitiesHandler::CreateEntityWithParent(Entity parent_entity)
{
    Entity return_entity = 0;
    if (entitiesRecycleBin.size())
    {
        for(size_t index = 0; index < entitiesRecycleBin.size(); index++)
            if (entitiesRecycleBin[index] > parent_entity)
            {
                return_entity = entitiesRecycleBin[index];
                entitiesRecycleBin.erase(entitiesRecycleBin.begin() + index);

                break;
            }
    }

    if (return_entity == 0)
    {
        return_entity = static_cast<Entity>(componentsOfEachEntity.size());
        parentOfEachEntity.emplace_back(-1);
        childrenOfEachEntity.emplace_back(std::set<Entity>());
        componentsOfEachEntity.emplace_back(std::vector<componentID>());
    }

    
    assert(return_entity > parent_entity);
    assert(componentsOfEachEntity[return_entity].empty());
    assert(childrenOfEachEntity[return_entity].empty());
    assert(childrenOfEachEntity[parent_entity].find(return_entity) == childrenOfEachEntity[parent_entity].end());

    parentOfEachEntity[return_entity] = parent_entity;
    childrenOfEachEntity[parent_entity].emplace(return_entity);

    return return_entity;
}

void EntitiesHandler::DeleteEmptyEntity(Entity this_entity)
{
    assert(this_entity < componentsOfEachEntity.size());
    assert(parentOfEachEntity[this_entity] != -1 || this_entity == 0);
    assert(componentsOfEachEntity[this_entity].empty());
    assert(childrenOfEachEntity[this_entity].empty());

    if (this_entity == 0)   // If it is default entity then don't do futher shits
        return;

    auto search = entityToName_umap.find(this_entity);
    if (search != entityToName_umap.end())
    {
        nameToEntity_umap.erase(search->second);
        entityToName_umap.erase(search->first);
    }

    Entity parent_entity = parentOfEachEntity[this_entity];

    assert(childrenOfEachEntity[parent_entity].find(this_entity) != childrenOfEachEntity[parent_entity].end());
    childrenOfEachEntity[parent_entity].erase(this_entity);

    parentOfEachEntity[this_entity] = -1;

    entitiesRecycleBin.emplace_back(this_entity);
}

void EntitiesHandler::AddEntityName(Entity this_entity, std::string name)
{
    assert(this_entity < componentsOfEachEntity.size());
    assert(parentOfEachEntity[this_entity] != -1);
    assert(nameToEntity_umap.find(name) == nameToEntity_umap.end());

    nameToEntity_umap.emplace(name, this_entity);
    entityToName_umap.emplace(this_entity, name);
}

Entity EntitiesHandler::FindEntityByName(std::string name)
{
    assert(nameToEntity_umap.find(name) != nameToEntity_umap.end());

    auto search = nameToEntity_umap.find(name);
    Entity return_entity = search->second;

    assert(return_entity < componentsOfEachEntity.size());
    assert(parentOfEachEntity[return_entity] != -1);

    return return_entity;
}

Entity EntitiesHandler::FindEntityByRelativeName(std::string relative_name, Entity relative_entity)
{
    assert(relative_entity < componentsOfEachEntity.size());
    assert(parentOfEachEntity[relative_entity] != -1);

    std::string name;

    if (relative_name.substr(0, relative_name.find_first_of("/")) != "_root")
    {
        std::string name_of_relative_entity = GetEntityName(relative_entity);
        assert(name_of_relative_entity != "");

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
    assert(this_entity < componentsOfEachEntity.size());
    assert(parentOfEachEntity[this_entity] != -1);

    auto search = entityToName_umap.find(this_entity);

    std::string return_name;
    if (entityToName_umap.find(this_entity) != entityToName_umap.end())
        return_name = search->second;
    else
        return_name = "";

    return return_name;
}

std::vector<componentID> EntitiesHandler::GetComponentsOfEntity(Entity this_entity)
{
    assert(this_entity < componentsOfEachEntity.size());
    assert(parentOfEachEntity[this_entity] != -1);

    for (size_t index = 0; index < entitiesRecycleBin.size(); index++)
        assert(entitiesRecycleBin[index] != this_entity);


    return componentsOfEachEntity[this_entity];
}

std::vector<Entity> EntitiesHandler::GetChildrenOfEntity(Entity this_entity)
{
    assert(this_entity < componentsOfEachEntity.size());
    assert(parentOfEachEntity[this_entity] != -1 || this_entity == 0);

    std::vector<Entity> return_vector;
    return_vector.assign(childrenOfEachEntity[this_entity].begin(), childrenOfEachEntity[this_entity].end());

    return return_vector;
}

void EntitiesHandler::EntityAttachedTo(Entity this_entity, componentID at_component)
{
    assert(this_entity < componentsOfEachEntity.size());
    assert(parentOfEachEntity[this_entity] != -1);

    for (size_t index = 0; index < entitiesRecycleBin.size(); index++)
        assert(entitiesRecycleBin[index] != this_entity);

    auto search = std::find(componentsOfEachEntity[this_entity].begin(), componentsOfEachEntity[this_entity].end(), at_component);
    assert(search == componentsOfEachEntity[this_entity].end());

    componentsOfEachEntity[this_entity].emplace_back(at_component);
}

void EntitiesHandler::EntityDeattachFrom(Entity this_entity, componentID at_component)
{
    assert(this_entity < componentsOfEachEntity.size());
    assert(parentOfEachEntity[this_entity] != -1);

    for (size_t index = 0; index < entitiesRecycleBin.size(); index++)
        assert(entitiesRecycleBin[index] != this_entity);

    auto search = std::find(componentsOfEachEntity[this_entity].begin(), componentsOfEachEntity[this_entity].end(), at_component);
    assert(search != componentsOfEachEntity[this_entity].end());

    componentsOfEachEntity[this_entity].erase(search);
}
