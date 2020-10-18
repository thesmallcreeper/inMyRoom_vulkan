#pragma once

#include "ECS/CompEntityBaseWrappedClass.h"

class ModelDrawComp;
class ModelDrawCompEntity;
#ifdef GAME_DLL
class ModelDrawComp
    :public ComponentBaseWrappedClass<ModelDrawCompEntity, static_cast<componentID>(componentIDenum::ModelDraw), "ModelDraw"> {};
#else
#include "ECS/GeneralComponents/ModelDrawComp.h"
#endif

#include "ECS/GeneralCompEntities/LateNodeGlobalMatrixCompEntity.h"
#include "ECS/GeneralCompEntities/SkinCompEntity.h"

class ModelDrawCompEntity :
    public CompEntityBaseWrappedClass<ModelDrawComp>
{
#ifndef GAME_DLL
public:
    ModelDrawCompEntity(Entity this_entity);

    static ModelDrawCompEntity GetEmpty();

    /*  CreateComponentEntityByMap - ModelDraw
            "MeshIndex",         meshIndex               = int    
            "ShouldDraw",        shouldDraw              = int         (optional)
            "DisableCulling",    disableCulling          = int         (optional)
            "IsSkin",            isSkin                  = int         (optional)
    */
    static ModelDrawCompEntity CreateComponentEntityByMap(Entity in_entity, const CompEntityInitMap& in_map);

    void Init();

    void DrawUsingFrustumCull(LateNodeGlobalMatrixComp* nodeGlobalMatrix_ptr,
                              SkinComp* skin_ptr,
                              class MeshesOfNodes* meshesOfNodes_ptr,
                              class PrimitivesOfMeshes* primitivesOfMeshes_ptr,
                              class FrustumCulling* frustumCulling_ptr,
                              std::vector<DrawRequest>& opaque_draw_requests,
                              std::vector<DrawRequest>& transparent_draw_requests) const;

#endif
public: // data
    uint32_t meshIndex;
    bool shouldDraw = true;
    bool disableCulling = false;
    
    bool isSkin = false;
};
