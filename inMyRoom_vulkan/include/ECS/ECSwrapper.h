#pragma once

#include <memory>
#include <map>
#include <unordered_map>
#include <mutex>
#include <chrono>

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

    std::chrono::duration<float> GetUpdateDeltaTime();
    void RefreshUpdateDeltaTime();

    std::vector<std::string> GetComponentsNames();

    void AddComponent(ComponentBaseClass* this_component_ptr);                                  // On runtime the very same Update function adds and removes components, entities.
    void AddComponentAndOwnership(std::unique_ptr<ComponentBaseClass> this_component_uptr);     // So no need for mutex(dangerous) or recursive_mutex(safe).

    void RemoveEntityAndChildrenFromAllComponentsAndDelete(Entity this_entity);

    void Update(bool complete_adds_and_removes);
    void FixedUpdate();                                                                         // Forbidden to add or remove anything
    void AsyncInput(InputType input_type, void* struct_data = nullptr);                         // Forbidden to add or remove anything

    void CompleteAddsAndRemoves();

    ExportedFunctions*    GetEnginesExportedFunctions();

private:
    void CompleteAddsAndRemovesUnsafe();

private:    // data
    std::vector<Entity> entitesThatGoingToGetEmptyToRemove;

    std::map<componentID, ComponentBaseClass*> componentIDtoComponentBaseClass_map;
    std::unordered_map<std::string, componentID> componentNameToComponentID_umap;
    // Garbage collector gotta clean the floor
    std::vector<std::unique_ptr<ComponentBaseClass>> componentsBaseClassOwnership_uptrs;

    std::unique_ptr<EntitiesHandler> entitiesHandler_uptr;

    std::chrono::steady_clock::time_point lastFramePoint;
    std::chrono::duration<float> deltaTime;

    ExportedFunctions* const exportedFunctions_ptr;

    std::mutex controlMutex;
};

