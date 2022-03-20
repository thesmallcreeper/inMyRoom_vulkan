#include "ECS/GeneralComponents/LightComp.h"

#include "ECS/ECSwrapper.h"

LightComp::LightComp(ECSwrapper *in_ecs_wrapper_ptr, Lights *in_lights_ptr)
    :ComponentDataClass<LightCompEntity, static_cast<componentID>(componentIDenum::Light), "Light", sparse_set>(in_ecs_wrapper_ptr),
     lights_ptr(in_lights_ptr)
{
}

void LightComp::Update()
{
    size_t containers_count_when_start = GetContainersCount();
    for(; containersUpdated != containers_count_when_start; ++containersUpdated)
    {
        auto& this_container = GetContainerByIndex(containersUpdated);
        for(auto& this_comp_entity: this_container)
            this_comp_entity.Update(lights_ptr);
    }
}

void LightComp::ToBeRemovedCallback(const std::vector<std::pair<Entity, Entity>> &callback_ranges)
{
    size_t containers_count_when_start = GetContainersCount();
    for (const std::pair<Entity, Entity>& this_range : callback_ranges) {
        size_t container_index = this->entitiesHandler_ptr->GetContainerIndexOfEntity(this_range.first);
        auto& container = GetContainerByIndex(container_index);

        auto iterators = container.get_range_iterators(this_range);
        for (auto it = iterators.first; it != iterators.second; ++it) {
            it->ToBeRemovedCallBack(lights_ptr);
        }
    }
}

void LightComp::AddLightInfos(const glm::mat4 &viewport_matrix,
                              std::vector<ModelMatrices>& matrices,
                              std::vector<LightInfo>& light_infos)
{
    auto nodeGlobalMatrix_componentID = static_cast<componentID>(componentIDenum::LateNodeGlobalMatrix);
    auto nodeGlobalMatrixComp_ptr = static_cast<const LateNodeGlobalMatrixComp*>(ecsWrapper_ptr->GetComponentByID(nodeGlobalMatrix_componentID));

    size_t containers_count = GetContainersCount();
    for(size_t i = 0; i != containers_count; ++i)
    {
        auto& this_container = GetContainerByIndex(i);
        for(auto& this_comp_entity: this_container)
            this_comp_entity.AddLightInfo(nodeGlobalMatrixComp_ptr,
                                          viewport_matrix,
                                         matrices,
                                          light_infos);
    }
}
