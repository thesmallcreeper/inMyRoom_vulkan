#pragma once

#include "ECStypes.h"

#include "FrustumCulling.h"
#include "MeshesOfNodes.h"
#include "Drawer.h"

class ModelDrawComp;

class ModelDrawCompEntity
{
public:
    ModelDrawCompEntity(const Entity this_entity);
    ~ModelDrawCompEntity();

    static ModelDrawCompEntity GetEmpty();

    /*  CreateComponentEntityByMap - ModelDraw
            "MeshIndex",         meshIndex               = int    
            "ShouldDraw",        shouldDraw              = int         (optional)
    */
    static ModelDrawCompEntity CreateComponentEntityByMap(const Entity in_entity, const ComponentEntityInitMap in_map);

    void DrawUsingFrustumCull(class NodeGlobalMatrixComp* nodeGlobalMatrix_ptr, MeshesOfNodes* meshesOfNodes_ptr, FrustumCulling* frustumCulling_ptr, std::vector<DrawRequest>& draw_requests) const;

public: // data
    uint32_t meshIndex;
    bool shouldDraw = true;

    Entity thisEntity;

private: // static variable
    friend class ModelDrawComp;
    static ModelDrawComp* modelDrawComp_ptr;
};
