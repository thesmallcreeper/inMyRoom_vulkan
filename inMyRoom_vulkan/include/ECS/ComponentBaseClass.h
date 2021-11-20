#pragma once

#include <memory>
#include <string>
#include <map>
#include <vector>
#include <utility>

#include "ECS/ECStypes.h"

#include "glm/vec4.hpp"

class ECSwrapper;       // Forward declaration
class EntitiesHandler;

class ComponentBaseClass
{
public:
    explicit ComponentBaseClass(ECSwrapper* in_ecs_wrapper_ptr);
    virtual ~ComponentBaseClass();
    virtual void Deinit() {};

    virtual void Update() {};
    virtual void AsyncInput(InputType input_type, void* struct_data = nullptr) {};
    virtual void CollisionCallback(const std::vector<std::pair<Entity, std::vector<CollisionCallbackData>>>& callback_entity_data_pairs) {};
    virtual void ToBeRemovedCallback(const std::vector<std::pair<Entity, Entity>>& callback_ranges) {};
     
    virtual size_t PushBackNewFab() {return -1;};
    virtual void AddCompEntityAtLatestFab(Entity entity, const std::string& fab_path, const CompEntityInitMap& init_map) {};
    virtual std::pair<Entity, Entity> GetLatestFabRange() {return std::make_pair<Entity, Entity>(-1,-1);};

    virtual DataSetPtr InitializeFab(Entity offset, size_t fab_index) {return nullptr; };
    virtual void RemoveInstancesByRanges(const std::vector<std::pair<Entity, Entity>>& ranges) {};

    virtual void AddInitializedFabs() {};
    virtual void NewUpdateSession() {};
                                                                        
protected:
    virtual ComponentEntityPtr GetComponentEntityVoidPtr(Entity this_entity, size_t index_hint) {return nullptr; };

public:
    virtual componentID GetComponentID() const {return -1; };
    virtual std::string GetComponentName() const {return ""; };

    ECSwrapper* GetECSwrapper() const;

protected:
    ECSwrapper* const ecsWrapper_ptr;
    EntitiesHandler* const entitiesHandler_ptr;
};

