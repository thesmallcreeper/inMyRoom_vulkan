#pragma once

#include <memory>
#include <string>
#include <map>
#include <vector>
#include <utility>

#include "ECS/ECStypes.h"

#include "glm/vec4.hpp"

class ECSwrapper;       // Forward declaration

class ComponentBaseClass
{
public:
    ComponentBaseClass(ECSwrapper* const in_ecs_wrapper_ptr);
    virtual ~ComponentBaseClass();
    virtual void Deinit() = 0;

    virtual std::vector<std::pair<std::string, MapType>> GetComponentInitMapFields() const = 0;

    virtual void Update() {}
    virtual void AsyncInput(InputType input_type, void* struct_data = nullptr) {}

    virtual void CollisionCallback(Entity this_entity, const CollisionCallbackData& this_collisionCallbackData) {}
     
//  ------- void AddComponent(const Entity this_entity, ComponentEntityType this_componentEntity);                 // Component entity specific task
    virtual void AddComponentEntityByMap(const Entity this_entity, const CompEntityInitMap& this_map) = 0;         // Component entity specific task
    virtual void RemoveComponentEntity(const Entity this_entity) = 0;                                              // Component entity memory specific task
    virtual void CompleteAddsAndRemoves() = 0;                                                                     // Component entity memory specific task
    virtual void InitAdds() = 0;                                                                                   // Component entity memory specific task
protected:
    virtual ComponentEntityPtr GetComponentEntityVoidPtr(const Entity this_entity) = 0;                            // Component entity memory specific task

public:
    virtual componentID GetComponentID() const = 0;
    virtual std::string GetComponentName() const = 0;

    ECSwrapper* GetECSwrapper() const;

public:  // Only component entities should call that
    void InformEntitiesHandlerAboutAddition(const Entity this_entity) const;
    void InformEntitiesHandlerAboutRemoval(const Entity this_entity) const;

protected:
    std::vector<std::pair<std::string, MapType>> componentInitMapFields;

    ECSwrapper* const ecsWrapper_ptr;
};

