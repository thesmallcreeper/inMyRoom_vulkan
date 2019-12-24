#pragma once

#include "ECS/ECStypes.h"

#include "Geometry/FrustumCulling.h"
#include "Meshes/MeshesOfNodes.h"
#include "Meshes/SkinsOfMeshes.h"
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
            "DisableCulling",    disableCulling          = int         (optional)
            "IsSkin",            isSkin                  = int         (optional)
    */
    static ModelDrawCompEntity CreateComponentEntityByMap(const Entity in_entity, const CompEntityInitMap in_map);
    static std::vector<std::pair<std::string, MapType>> GetComponentInitMapFields();

    void Init();

    void DrawUsingFrustumCull(class NodeGlobalMatrixComp* nodeGlobalMatrix_ptr,
                              class SkinComp* skin_ptr,
                              MeshesOfNodes* meshesOfNodes_ptr,
                              PrimitivesOfMeshes* primitivesOfMeshes_ptr,
                              FrustumCulling* frustumCulling_ptr,
                              std::vector<DrawRequest>& opaque_draw_requests,
                              std::vector<DrawRequest>& transparent_draw_requests) const;

public: // data
    uint32_t meshIndex;
    bool shouldDraw = true;
    bool disableCulling = false;
    
    bool isSkin = false;

    Entity thisEntity;

private: // static variable
    friend class ModelDrawComp;
    static ModelDrawComp* modelDrawComp_ptr;
};
