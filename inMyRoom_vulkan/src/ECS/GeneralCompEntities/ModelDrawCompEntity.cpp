#include "ECS/GeneralCompEntities/ModelDrawCompEntity.h"

#include "ECS/GeneralComponents/ModelDrawComp.h"
#include "ECS/GeneralComponents/NodeGlobalMatrixComp.h"

ModelDrawComp* ModelDrawCompEntity::modelDrawComp_ptr = nullptr;

ModelDrawCompEntity::ModelDrawCompEntity(const Entity this_entity)
    :thisEntity(this_entity)
{
}

ModelDrawCompEntity::~ModelDrawCompEntity()
{
}

ModelDrawCompEntity ModelDrawCompEntity::GetEmpty()
{
    ModelDrawCompEntity this_modelDrawCompEntity(0);

    return this_modelDrawCompEntity;
}

ModelDrawCompEntity ModelDrawCompEntity::CreateComponentEntityByMap(const Entity in_entity, const CompEntityInitMap in_map)
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

    return this_modelDrawCompEntity;
}

std::vector<std::pair<std::string, MapType>> ModelDrawCompEntity::GetComponentInitMapFields()
{
    std::vector<std::pair<std::string, MapType>> return_pair;
    return_pair.emplace_back(std::make_pair("MeshIndex", MapType::int_type));
    return_pair.emplace_back(std::make_pair("ShouldDraw", MapType::int_type));

    return return_pair;
}

void ModelDrawCompEntity::DrawUsingFrustumCull(NodeGlobalMatrixComp* nodeGlobalMatrixComp_ptr, MeshesOfNodes* meshesOfNodes_ptr, FrustumCulling* frustumCulling_ptr, std::vector<DrawRequest>& draw_requests) const
{
    if (shouldDraw)
    {
        NodeGlobalMatrixCompEntity* this_meshGlobalMatrix_ptr = reinterpret_cast<NodeGlobalMatrixCompEntity*>(nodeGlobalMatrixComp_ptr->GetComponentEntity(thisEntity));
        const glm::mat4x4 this_global_matrix = this_meshGlobalMatrix_ptr->globalMatrix;

        MeshInfo this_mesh_info = meshesOfNodes_ptr->GetMesh(meshIndex);
        const OBB& this_OBB = this_mesh_info.boundBox;

        Cuboid this_cuboid = this_global_matrix * this_OBB;

        if (frustumCulling_ptr->IsCuboidInsideFrustum(this_cuboid)) // per mesh-object OBB culling
            for (size_t primitive_index = this_mesh_info.primitiveFirstOffset; primitive_index < this_mesh_info.primitiveFirstOffset + this_mesh_info.primitiveRangeSize; primitive_index++)
            {
                DrawRequest this_draw_request;
                this_draw_request.primitiveIndex = primitive_index;
                this_draw_request.objectID = thisEntity;
                this_draw_request.TRSmatrix = this_global_matrix;

                draw_requests.emplace_back(this_draw_request);
            }
    }
}

void ModelDrawCompEntity::Init()
{
}
