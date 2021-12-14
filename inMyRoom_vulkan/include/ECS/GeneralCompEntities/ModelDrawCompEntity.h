#pragma once

#include "ECS/CompEntityBaseWrappedClass.h"

class ModelDrawComp;

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
    static ModelDrawCompEntity CreateComponentEntityByMap(Entity in_entity, std::string entity_name, const CompEntityInitMap& in_map);

    void AddDrawInfo(const LateNodeGlobalMatrixComp* nodeGlobalMatrix_ptr,
                     const SkinComp* skinEntity_ptr,
                     std::vector<glm::mat4>& matrices,
                     std::vector<DrawInfo>& draw_infos) const;

#endif
public: // data
    uint32_t meshIndex;
    bool shouldDraw = true;
    bool disableCulling = false;
    
    bool isSkin = false;
};

#ifdef GAME_DLL
class ModelDrawComp
    :public ComponentBaseWrappedClass<ModelDrawCompEntity, static_cast<componentID>(componentIDenum::ModelDraw), "ModelDraw", sparse_set> {};
#else
#include "ECS/GeneralComponents/ModelDrawComp.h"
#endif