#pragma once

#include "ECS/GeneralCompEntities/ModelDrawCompEntity.h"
#include "ECS/ComponentDataClass.h"

#include "ECS/ComponentsIDsEnum.h"

#include "Graphics/Meshes/MeshesOfNodes.h"
#include "Graphics/Meshes/PrimitivesOfMeshes.h"
#include "Geometry/FrustumCulling.h"

class ModelDrawComp final
    : public ComponentDataClass<ModelDrawCompEntity, static_cast<componentID>(componentIDenum::ModelDraw), "ModelDraw", sparse_set>
{
public:
    explicit ModelDrawComp(ECSwrapper* const in_ecs_wrapper_ptr);
    ~ModelDrawComp() override;

    void AddDrawInfos(const glm::mat4& viewport_matrix,
                      std::vector<ModelMatrices>& matrices,
                      std::vector<DrawInfo>& draw_infos);
    void ToBeRemovedCallback(const std::vector<std::pair<Entity, Entity>>& callback_ranges) override;
};

