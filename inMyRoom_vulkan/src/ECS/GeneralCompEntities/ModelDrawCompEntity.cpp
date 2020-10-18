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

ModelDrawCompEntity ModelDrawCompEntity::CreateComponentEntityByMap(const Entity in_entity, const CompEntityInitMap& in_map)
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

void ModelDrawCompEntity::DrawUsingFrustumCull(LateNodeGlobalMatrixComp* nodeGlobalMatrixComp_ptr,
                                               SkinComp* skin_ptr,
                                               MeshesOfNodes* meshesOfNodes_ptr,
                                               PrimitivesOfMeshes* primitivesOfMeshes_ptr,
                                               FrustumCulling* frustumCulling_ptr,
                                               std::vector<DrawRequest>& opaque_draw_requests,
                                               std::vector<DrawRequest>& transparent_draw_requests) const
{
    if (shouldDraw)
    {
        LateNodeGlobalMatrixCompEntity& this_meshGlobalMatrix_ptr = nodeGlobalMatrixComp_ptr->GetComponentEntity(thisEntity);
        const glm::mat4x4 this_global_matrix = this_meshGlobalMatrix_ptr.globalMatrix;

        const MeshInfo* this_mesh_info_ptr = meshesOfNodes_ptr->GetMeshInfoPtr(meshIndex);

        if(!disableCulling)
        {
            Cuboid this_cuboid = this_global_matrix * this_mesh_info_ptr->boundBoxTree.GetOBB();

            if (frustumCulling_ptr->IsCuboidInsideFrustum(this_cuboid)) // per mesh-object OBB culling
                for (size_t primitive_index = this_mesh_info_ptr->primitiveFirstOffset; primitive_index < this_mesh_info_ptr->primitiveFirstOffset + this_mesh_info_ptr->primitiveRangeSize; primitive_index++)
                {
                    DrawRequest this_draw_request;
                    this_draw_request.primitiveIndex = primitive_index;
                    this_draw_request.objectID = thisEntity;
                    if (!isSkin)
                    {                   
                        this_draw_request.vertexData.TRSmatrix = this_global_matrix;

                        this_draw_request.isSkin = false;
                    }                       
                    else
                    {
                        SkinCompEntity& this_skin_ptr = skin_ptr->GetComponentEntity(thisEntity);
                        this_draw_request.vertexData.inverseBindMatricesOffset = this_skin_ptr.inverseBindMatricesOffset;
                        this_draw_request.vertexData.nodesMatricesOffset = this_skin_ptr.lastNodesMatricesOffset;

                        this_draw_request.isSkin = true;
                    }

                    if (primitivesOfMeshes_ptr->IsPrimitiveTransparent(primitive_index))
                        transparent_draw_requests.emplace_back(this_draw_request);                   
                    else
                        opaque_draw_requests.emplace_back(this_draw_request);
                }
        }
        else
        {
            for (size_t primitive_index = this_mesh_info_ptr->primitiveFirstOffset; primitive_index < this_mesh_info_ptr->primitiveFirstOffset + this_mesh_info_ptr->primitiveRangeSize; primitive_index++)
            {
                DrawRequest this_draw_request;
                this_draw_request.primitiveIndex = primitive_index;
                this_draw_request.objectID = thisEntity;
                if (!isSkin)
                {
                    this_draw_request.vertexData.TRSmatrix = this_global_matrix;

                    this_draw_request.isSkin = false;
                }
                else
                {
                    SkinCompEntity& this_skin_ptr = skin_ptr->GetComponentEntity(thisEntity);
                    this_draw_request.vertexData.inverseBindMatricesOffset = this_skin_ptr.inverseBindMatricesOffset;
                    this_draw_request.vertexData.nodesMatricesOffset = this_skin_ptr.lastNodesMatricesOffset;

                    this_draw_request.isSkin = true;
                }

                if (primitivesOfMeshes_ptr->IsPrimitiveTransparent(primitive_index))
                    transparent_draw_requests.emplace_back(this_draw_request);
                else
                    opaque_draw_requests.emplace_back(this_draw_request);
            }
        }
    }
}

void ModelDrawCompEntity::Init()
{
}

#endif