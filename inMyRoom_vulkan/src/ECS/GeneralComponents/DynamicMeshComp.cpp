#include "ECS/GeneralComponents/DynamicMeshComp.h"

#include "ECS/ECSwrapper.h"

DynamicMeshComp::DynamicMeshComp(ECSwrapper* in_ecs_wrapper_ptr,
                                 DynamicMeshes* in_dynamicMeshes_ptr,
                                 MeshesOfNodes* in_meshesOfNodes_ptr)
    :ComponentDataClass<DynamicMeshCompEntity, static_cast<componentID>(componentIDenum::DynamicMesh), "DynamicMesh", sparse_set>(in_ecs_wrapper_ptr),
     dynamicMeshes_ptr(in_dynamicMeshes_ptr),
     meshesOfNodes_ptr(in_meshesOfNodes_ptr)
{
}

void DynamicMeshComp::Update()
{
    auto modelDraw_componentID = static_cast<componentID>(componentIDenum::ModelDraw);
    auto modelDrawComp_ptr = static_cast<ModelDrawComp*>(ecsWrapper_ptr->GetComponentByID(modelDraw_componentID));

    size_t containers_count_when_start = GetContainersCount();
    for(; containersUpdated != containers_count_when_start; ++containersUpdated)
    {
        auto& this_container = GetContainerByIndex(containersUpdated);
        for(auto& this_comp_entity: this_container)
            this_comp_entity.Update(modelDrawComp_ptr,
                                    dynamicMeshes_ptr,
                                    meshesOfNodes_ptr);
    }
}

void DynamicMeshComp::ToBeRemovedCallback(const std::vector<std::pair<Entity, Entity>> &callback_ranges)
{
    size_t containers_count_when_start = GetContainersCount();
    for (const std::pair<Entity, Entity>& this_range : callback_ranges) {
        size_t container_index = this->entitiesHandler_ptr->GetContainerIndexOfEntity(this_range.first);
        auto& container = GetContainerByIndex(container_index);

        auto iterators = container.get_range_iterators(this_range);
        for (auto it = iterators.first; it != iterators.second; ++it) {
            it->ToBeRemovedCallBack(dynamicMeshes_ptr);
        }
    }
}



