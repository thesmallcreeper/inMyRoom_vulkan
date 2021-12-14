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

    return this_modelDrawCompEntity;
}

void ModelDrawCompEntity::AddDrawInfo(const LateNodeGlobalMatrixComp* nodeGlobalMatrix_ptr,
                                      const SkinComp* skinEntity_ptr,
                                      std::vector<glm::mat4>& matrices,
                                      std::vector<DrawInfo>& draw_infos) const
{
    if (shouldDraw)
    {
        DrawInfo this_draw_info;
        this_draw_info.meshIndex = meshIndex;
        this_draw_info.matricesOffset = matrices.size();
        this_draw_info.dontCull = disableCulling;

        if (not isSkin) {
            this_draw_info.isSkin = false;

            glm::mat4x4 matrix = nodeGlobalMatrix_ptr->GetComponentEntity(thisEntity).globalMatrix;
            matrices.emplace_back(matrix);
        } else {
            this_draw_info.isSkin = true;
            this_draw_info.inverseMatricesOffset = skinEntity_ptr->GetComponentEntity(thisEntity).inverseBindMatricesOffset;

            for(Entity relative_entity: skinEntity_ptr->GetComponentEntity(thisEntity).jointRelativeEntities) {
                glm::mat4x4 joint_matrix = nodeGlobalMatrix_ptr->GetComponentEntity(thisEntity + relative_entity).globalMatrix;
                matrices.emplace_back(joint_matrix);
            }
        }

        draw_infos.emplace_back(this_draw_info);
    }
}
#endif