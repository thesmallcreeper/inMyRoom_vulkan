#pragma once

#include "ECS/CompEntityBaseWrappedClass.h"

class ModelDrawComp;

#include "ECS/GeneralCompEntities/LateNodeGlobalMatrixCompEntity.h"
#include "ECS/GeneralCompEntities/DynamicMeshCompEntity.h"

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
            "HasMorphTargets",   hasMorphTargets         = int         (optional)
    */
    static ModelDrawCompEntity CreateComponentEntityByMap(Entity in_entity, std::string entity_name, const CompEntityInitMap& in_map);

    void AddDrawInfo(const LateNodeGlobalMatrixComp* nodeGlobalMatrix_ptr,
                     const DynamicMeshComp* dynamicMeshComp_ptr,
                     const glm::mat4& viewport_matrix,
                     std::vector<ModelMatrices>& model_matrices,
                     std::vector<DrawInfo>& draw_infos) const;

    void ToBeRemovedCallback();

#endif
public: // data
    uint32_t meshIndex = -1;
    bool shouldDraw = true;
    bool disableCulling = false;

    bool isSkin = false;
    bool hasMorphTargets = false;
};

#ifdef GAME_DLL
class ModelDrawComp
    :public ComponentBaseWrappedClass<ModelDrawCompEntity, static_cast<componentID>(componentIDenum::ModelDraw), "ModelDraw", sparse_set> {};
#else
#include "ECS/GeneralComponents/ModelDrawComp.h"
#endif