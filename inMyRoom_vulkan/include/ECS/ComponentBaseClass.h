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
    ComponentBaseClass(ECSwrapper* in_ecs_wrapper_ptr);
    virtual ~ComponentBaseClass();
    virtual void Deinit() = 0;

    virtual void Update() {}
    virtual void AsyncInput(InputType input_type, void* struct_data = nullptr) {}
    virtual void CollisionCallback(const std::vector<std::pair<Entity, CollisionCallbackData>>& callback_entity_data_pairs) {}
     
    virtual size_t PushBackNewFab() = 0;
    virtual void AddCompEntityAtLatestFab(Entity entity, const std::string& fab_path, const CompEntityInitMap& init_map) = 0;
    virtual std::pair<Entity, Entity> GetLatestFabRange() = 0;

    virtual DataSetPtr InitializeFab(Entity offset, size_t fab_index) = 0;
    virtual void RemoveInstancesByRanges(const std::vector<std::pair<Entity, Entity>>& ranges) = 0;

    virtual void AddInitializedFabs() = 0;
                                                                        
protected:
    virtual ComponentEntityPtr GetComponentEntityVoidPtr(const Entity this_entity) = 0;

public:
    virtual componentID GetComponentID() const = 0;
    virtual std::string GetComponentName() const = 0;

    ECSwrapper* GetECSwrapper() const;

protected:
    ECSwrapper* const ecsWrapper_ptr;
};

