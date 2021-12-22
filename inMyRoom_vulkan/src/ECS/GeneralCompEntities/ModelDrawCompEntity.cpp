#include "ECS/GeneralCompEntities/ModelDrawCompEntity.h"

#include "ECS/ECSwrapper.h"

#ifndef GAME_DLL

#include "Geometry/FrustumCulling.h"

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
                                      std::vector<glm::mat4>& matrices,
                                      std::vector<DrawInfo>& draw_infos) const
{
    if (shouldDraw)
    {
        DrawInfo this_draw_info;
        this_draw_info.meshIndex = meshIndex;
        this_draw_info.matricesOffset = matrices.size();
        this_draw_info.dontCull = disableCulling;

        if (not isSkin && not hasMorphTargets) {
            glm::mat4 matrix = nodeGlobalMatrix_ptr->GetComponentEntity(thisEntity).globalMatrix;
            matrices.emplace_back(matrix);
        } else {
            const auto& dynamic_mesh_entity = dynamicMeshComp_ptr->GetComponentEntity(thisEntity);
            this_draw_info.dynamicMeshIndex = dynamic_mesh_entity.dynamicMeshIndex;

            if (isSkin) {
                this_draw_info.isSkin = true;
                this_draw_info.inverseMatricesOffset = dynamic_mesh_entity.inverseBindMatricesOffset;

                glm::mat4 parent_matrix = nodeGlobalMatrix_ptr->GetComponentEntity(thisEntity).globalMatrix;
                matrices.emplace_back(parent_matrix);

                glm::mat4 inverse_parent_matrix = glm::inverse(parent_matrix);
                for(Entity relative_entity: dynamic_mesh_entity.jointRelativeEntities) {
                    glm::mat4 joint_matrix = nodeGlobalMatrix_ptr->GetComponentEntity(thisEntity + relative_entity).globalMatrix;
                    matrices.emplace_back(inverse_parent_matrix * joint_matrix);
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