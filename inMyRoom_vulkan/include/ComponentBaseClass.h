#pragma once

#include <memory>
#include <string>
#include <map>
#include <vector>

#include "ECStypes.h"

#include "glm/vec4.hpp"

class ECSwrapper;       // Forward declaration

class ComponentBaseClass
{
public:
    ComponentBaseClass(const componentID this_ID, const std::string this_name, ECSwrapper* const in_ecs_wrapper_ptr);
    virtual ~ComponentBaseClass();
    virtual void Deinit() = 0;

    virtual void Update() = 0;
    virtual void FixedUpdate() = 0;
    virtual void AsyncUpdate( /* brah */ ) = 0;  // TODO
     
//  ------- void AddComponent(const Entity this_entity, ComponentEntityType this_componentEntity);                 // Component entity specific task at component memory layout level
    virtual void AddComponentEntityByMap(const Entity this_entity, const ComponentEntityInitMap this_map) = 0;     // Component entity specific task at component level
    virtual void RemoveComponentEntity(const Entity this_entity) = 0;                                              // Component entity memory specific task at component memory layout level
    virtual void CompleteAddsAndRemoves() = 0;                                                                     // Component entity memory specific task at component memory layout level
    virtual ComponentEntityPtr GetComponentEntity(const Entity this_entity) = 0;                                   // Component entity memory specific task at component memory layout level

    componentID GetComponentID() const;
    std::string GetComponentName() const;

public:  // Only component entities should call that
    void InformEntitiesHandlerAboutAddition(const Entity this_entity) const;
    void InformEntitiesHandlerAboutRemoval(const Entity this_entity) const;

protected:
    ECSwrapper* const ecsWrapper_ptr;
    const componentID thisComponentID;
    const std::string thisComponentName;
};

