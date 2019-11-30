#pragma once

#include <memory>
#include <map>
#include <unordered_map>

#include "ECS/ECStypes.h"
#include "ECS/ComponentsIDsEnum.h"

#include "ECS/EntitiesHandler.h"
#include "ECS/ComponentBaseClass.h"
#include "ECS/ExportedFunctions.h"

class ECSwrapper
{
public:
    ECSwrapper(ExportedFunctions* in_enginesExportedFunctions_ptr);
    ~ECSwrapper();

    EntitiesHandler*    GetEntitiesHandler();
    ComponentBaseClass* GetComponentByID(componentID component_id);
    componentID         GetComponentIDbyName(std::string component_name);

    void AddComponent(ComponentBaseClass* this_component_ptr);
    void AddComponentAndOwnership(std::unique_ptr<ComponentBaseClass> this_component_uptr);

    void RemoveEntityAndChildrenFromAllComponentsAndDelete(Entity this_entity); // todo

    void Update(bool complete_adds_and_removes);
    void FixedUpdate(bool complete_adds_and_removes);
    void AsyncUpdate(bool complete_adds_and_removes /*, brah */);      // TODO

    void CompleteAddsAndRemoves();

    ExportedFunctions*    GetEnginesExportedFunctions();

private:    // data
    std::vector<Entity> entitesThatGoingToGetEmptyToRemove;

    std::map<componentID, ComponentBaseClass*> componentIDtoComponentBaseClass_map;
    std::unordered_map<std::string, componentID> componentNameToComponentID_umap;
    // Garbage collector gotta clean the floor
    std::vector<std::unique_ptr<ComponentBaseClass>> componentsBaseClassOwnership_uptrs;

    std::unique_ptr<EntitiesHandler> entitiesHandler_uptr;

    ExportedFunctions* const exportedFunctions_ptr;
};

