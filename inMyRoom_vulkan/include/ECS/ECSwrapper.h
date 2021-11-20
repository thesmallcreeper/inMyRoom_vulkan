#pragma once

#include <memory>
#include <map>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <unordered_set>

#include "ECS/ECStypes.h"

#include "ECS/EntitiesHandler.h"
#include "ECS/ComponentBaseClass.h"
#include "ECS/ExportedFunctions.h"

#include "ECS/ConceptHelpers.h"

class AdditionInfo
{
public:
    const InstanceInfo* instance_info_ptr;

    template<typename T>
    auto& GetDataSet() const requires Component<T>
    {
        typedef decltype(T::GetDataSetType()) T_data_set;
        componentID this_compID = T::component_ID;

        auto search = componentIDtoDataSetPtr_map.find(this_compID);
        assert(search != componentIDtoDataSetPtr_map.end());

        DataSetPtr data_set_ptr = search->second;
        auto casted_data_set_ptr = static_cast<T_data_set*>(data_set_ptr);

        return *casted_data_set_ptr;
    }

    template<typename T>
    auto& GetDataSet() const requires CompEntity<T>
    {
        typedef decltype(T::GetComponentType()) T_component;
        return GetDataSet<T_component>();
    }

private:
    friend class ECSwrapper;
    void AddDataSetPtr(componentID comp_id, DataSetPtr data_set_ptr)
    {
        componentIDtoDataSetPtr_map.emplace(comp_id, data_set_ptr);
    }

private:
    std::map<componentID, DataSetPtr> componentIDtoDataSetPtr_map;
};

class ECSwrapper
{
public:
    explicit ECSwrapper(ExportedFunctions* in_enginesExportedFunctions_ptr);
    ~ECSwrapper();

    EntitiesHandler*    GetEntitiesHandler() const;
    ComponentBaseClass* GetComponentByID(componentID component_id) const;
    componentID         GetComponentIDbyName(std::string component_name) const;

    const std::map<componentID, ComponentBaseClass*>& GetComponentIDtoComponentBaseClassMap() const;

    std::chrono::duration<float> GetUpdateDeltaTime() const;
    void RefreshUpdateDeltaTime();

    std::vector<std::string> GetComponentsNames() const;

    void AddComponent(ComponentBaseClass* this_component_ptr);
    void AddComponentAndOwnership(std::unique_ptr<ComponentBaseClass> this_component_uptr);

    void AddFabs(const std::vector<Node*>& nodes_ptrs);
    const FabInfo* GetFabInfo(const std::string& fab_name) const;
    Entity GetRelativeEntityOffset(std::string base_path, std::string relative_path) const;

    AdditionInfo* AddInstance(const std::string& fab_name, Entity parent = 0);
    AdditionInfo* AddInstance(const FabInfo* fab_info_ptr, Entity parent = 0);
    AdditionInfo* AddInstance(const std::string& fab_name, const std::string& instance_name, Entity parent = 0);
    AdditionInfo* AddInstance(const FabInfo* fab_info_ptr, const std::string& instance_name, Entity parent = 0);
    void RemoveInstance(InstanceInfo* instance_info_ptr);

    void Update();
    void AsyncInput(InputType input_type, void* struct_data = nullptr);                         // Forbidden to add or remove anything

    void CompleteAddsAndRemoves();

    ExportedFunctions* GetEnginesExportedFunctions() const;

private:
    void AddNodeExistence(FabInfo& fab_info, Node* this_node_ptr, Entity& entities_added, Entity parent, std::string parent_name);
    void AddFabsCompEntitiesToComponents(FabInfo& fab_info, Node* this_node_ptr, Entity& entities_processes);

    void MakeToBeRemovedCallbacks();

    void CompleteAddsAndRemovesUnsafe();
    void CompleteRemovesUnsafe();
    void CompleteAddsUnsafe();

    void GetChildrenInstanceTree(InstanceInfo* instance_info_ptr, std::vector<InstanceInfo*>& children);

private:    // data
    std::unordered_set<InstanceInfo*> instancesToBeRemoved;
    std::vector<InstanceInfo*> instancesToCallbackToBeRemoved;
    std::vector<std::unique_ptr<AdditionInfo>> additionInfoUptrs;

    std::vector<FabInfo> fabsInfos;
    std::unordered_map<std::string, size_t> fabNameToIndex_umap;

    std::vector<std::unique_ptr<ComponentBaseClass>> componentsBaseClassOwnership_uptrs;

    std::map<componentID, ComponentBaseClass*> componentIDtoComponentBaseClass_map;
    std::unordered_map<std::string, componentID> componentNameToComponentID_umap;
    
    std::chrono::steady_clock::time_point lastFramePoint;
    std::chrono::duration<float> deltaTime;

    std::unique_ptr<EntitiesHandler> entitiesHandler_uptr;
    ExportedFunctions* const exportedFunctions_ptr;

    std::mutex controlMutex;
};

