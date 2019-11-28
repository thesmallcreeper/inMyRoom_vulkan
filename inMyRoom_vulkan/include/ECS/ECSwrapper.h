#pragma once

#include <memory>
#include <map>
#include <unordered_map>

#include "ECS/ECStypes.h"
#include "ECS/ComponentsIDsEnum.h"

#include "ECS/EntitiesHandler.h"
#include "ECS/ComponentBaseClass.h"
#include "ECS/EnginesExportedFunctions.h"

class ECSwrapper
{
public:
    ECSwrapper(EnginesExportedFunctions* in_enginesExportedFunctions_ptr);
    ~ECSwrapper();

    EntitiesHandler* GetEntitiesHandler();
    ComponentBaseClass* GetComponentByID(componentID component_id);
    componentID GetComponentIDbyName(std::string component_name);

    void AddComponent(ComponentBaseClass* this_component_ptr);
    void AddComponentAndOwnership(std::unique_ptr<ComponentBaseClass> this_component_uptr);

    void RemoveEntityFromAllComponents(Entity this_entity);
    void RemoveEntityFromAllComponentsAndDelete(Entity this_entity);

    void Update(bool complete_adds_and_removes);
    void FixedUpdate(bool complete_adds_and_removes);
    void AsyncUpdate(bool complete_adds_and_removes /*, brah */);      // TODO

    void CompleteAddsAndRemoves();

    EnginesExportedFunctions*    GetEnginesExportedFunctions();

private:    // data
    std::vector<Entity> entitesThatGoingToGetEmptyToRemove;

    std::map<componentID, ComponentBaseClass*> componentIDtoComponentBaseClass_map;
    std::unordered_map<std::string, componentID> componentNameToComponentID_umap;
    // Garbage collector gotta clean the floor
    std::vector<std::unique_ptr<ComponentBaseClass>> componentsBaseClassOwnership_uptrs;

    std::unique_ptr<EntitiesHandler> entitiesHandler_uptr;

    EnginesExportedFunctions* const enginesExportedFunctions_ptr;
};

