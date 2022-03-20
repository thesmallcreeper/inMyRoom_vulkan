#pragma once

#include "ECS/GeneralCompEntities/LightCompEntity.h"
#include "ECS/ComponentDataClass.h"

#include "ECS/ComponentsIDsEnum.h"

#include "Graphics/Lights.h"

class LightComp final
        : public ComponentDataClass<LightCompEntity, static_cast<componentID>(componentIDenum::Light), "Light", sparse_set>
{
public:
    LightComp(ECSwrapper* ecs_wrapper_ptr, Lights* lights_ptr);

    void Update() override;
    void ToBeRemovedCallback(const std::vector<std::pair<Entity, Entity>>& callback_ranges) override;

    void AddLightInfos(const glm::mat4& viewport_matrix,
                       std::vector<ModelMatrices>& matrices,
                       std::vector<LightInfo>& draw_infos);

private:
    Lights* lights_ptr;
};
