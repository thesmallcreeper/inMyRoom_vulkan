#include "ECS/GeneralCompEntities/ModelDrawCompEntity.h"

#include "ECS/ECSwrapper.h"

#ifndef GAME_DLL

#include "Geometry/FrustumCulling.h"
#include "glm/gtc/matrix_inverse.hpp"

ModelDrawCompEntity::ModelDrawCompEntity(const Entity this_entity)
    :CompEntityBaseWrappedClass<ModelDrawComp>(this_entity)
{
}

ModelDrawCompEntity ModelDrawCompEntity::GetEmpty()
{
    ModelDrawCompEntity this_modelDrawCompEntity(0);

    return this_modelDrawCompEntity;
}

ModelDrawCompEntity ModelDrawCompEntity::CreateComponentEntityByMap(const Entity in_entity, std::string entity_name, const CompEntityInitMap& in_map)
{
    ModelDrawCompEntity this_modelDrawCompEntity(in_entity);

    // "MeshIndex", meshIndex = int
    {
        auto search = in_map.intMap.find("MeshIndex");
        assert(search != in_map.intMap.end());

        int this_int = search->second;
        this_modelDrawCompEntity.meshIndex = static_cast<uint32_t>(this_int);
    }
    // "ShouldDraw", shouldDraw = int (optional)
    {
        auto search = in_map.intMap.find("ShouldDraw");
        if (search != in_map.intMap.end())
        {
            int this_int = search->second;
            this_modelDrawCompEntity.shouldDraw = static_cast<bool>(this_int);
        }
    }
    // "DisableCulling", disableCulling = int (optional)
    {
        auto search = in_map.intMap.find("DisableCulling");
        if (search != in_map.intMap.end())
        {
            int this_int = search->second;
            this_modelDrawCompEntity.disableCulling = static_cast<bool>(this_int);
        }
    }
    // "IsSkin", isSkin = int (optional)
    {
        auto search = in_map.intMap.find("IsSkin");
        if (search != in_map.intMap.end())
        {
            int this_int = search->second;
            this_modelDrawCompEntity.isSkin = static_cast<bool>(this_int);
        }
    }
    // "HasMorphTargets", hasMorphTargets = int (optional)
    {
        auto search = in_map.intMap.find("HasMorphTargets");
        if (search != in_map.intMap.end())
        {
            int this_int = search->second;
            this_modelDrawCompEntity.hasMorphTargets = static_cast<bool>(this_int);
        }
    }

    return this_modelDrawCompEntity;
}

void ModelDrawCompEntity::AddDrawInfo(const LateNodeGlobalMatrixComp* nodeGlobalMatrix_ptr,
                                      const DynamicMeshComp* dynamicMeshComp_ptr,
                                      const glm::mat4& viewport_matrix,
                                      std::vector<ModelMatrices>& model_matrices,
                                      std::vector<DrawInfo>& draw_infos) const
{
    if (shouldDraw)
    {
        DrawInfo this_draw_info;
        this_draw_info.meshIndex = meshIndex;
        this_draw_info.matricesOffset = model_matrices.size();
        this_draw_info.dontCull = disableCulling;

        if (not isSkin && not hasMorphTargets) {
            glm::mat4 pos_matrix = viewport_matrix * nodeGlobalMatrix_ptr->GetComponentEntity(thisEntity).globalMatrix;
            glm::mat4 normal_matrix = glm::adjointTranspose(pos_matrix);
            model_matrices.emplace_back(ModelMatrices({pos_matrix, normal_matrix}));
        } else {
            glm::mat4 parent_pos_matrix = viewport_matrix * nodeGlobalMatrix_ptr->GetComponentEntity(thisEntity).globalMatrix;
            glm::mat4 parent_normal_matrix = glm::adjointTranspose(parent_pos_matrix);
            model_matrices.emplace_back(ModelMatrices({parent_pos_matrix, parent_normal_matrix}));

            const auto& dynamic_mesh_entity = dynamicMeshComp_ptr->GetComponentEntity(thisEntity);
            this_draw_info.dynamicMeshIndex = dynamic_mesh_entity.dynamicMeshIndex;

            if (isSkin) {
                this_draw_info.isSkin = true;
                this_draw_info.inverseMatricesOffset = dynamic_mesh_entity.inverseBindMatricesOffset;

                glm::mat4 inverse_parent_matrix = glm::inverse(parent_pos_matrix);
                for(Entity relative_entity: dynamic_mesh_entity.jointRelativeEntities) {
                    glm::mat4 joint_pos_matrix = inverse_parent_matrix * nodeGlobalMatrix_ptr->GetComponentEntity(thisEntity + relative_entity).globalMatrix;
                    glm::mat4 joint_normal_matrix = glm::adjointTranspose(joint_pos_matrix);
                    model_matrices.emplace_back(ModelMatrices({joint_pos_matrix, joint_normal_matrix}));
                }
            }
            if (hasMorphTargets) {
                this_draw_info.weights = dynamic_mesh_entity.morphTargetsWeights;
                this_draw_info.hasMorphTargets = true;
            }
        }

        draw_infos.emplace_back(this_draw_info);
    }
}

void ModelDrawCompEntity::ToBeRemovedCallback()
{
    shouldDraw = false;
}

#endif