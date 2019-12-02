#pragma once

#include "ECS/ECStypes.h"

#include "Geometry/ViewportFrustum.h"




class CameraComp;

class CameraCompEntity
{
public:
    CameraCompEntity(const Entity this_entity);
    ~CameraCompEntity();

    static CameraCompEntity GetEmpty();

    /*  CreateComponentEntityByMap - CameraCompEntity
        "FOVy",              fovy                    = float         (optional-default from config)
        "aspect",            aspect                  = float         (optional-default from config)  
        "nearPlane",         near                    = float         (optional-default from config)
        "farPlane",          far                     = float         (optional-default from config)
        "GlobalPosition",    globalPosition          = vec4.xyz      (optional-default  0, 0, 0)
        "GlobalDirection",   globalDirection         = vec4.xyz      (optional-default  0, 0, 1)
        "UpDirection",       globalUp                = vec4.xyz      (optional-default  0,-1, 0)
    */
    static CameraCompEntity CreateComponentEntityByMap(const Entity in_entity, const CompEntityInitMap in_map);

    void Init();
    void Update(bool cull_debugging);

    void UpdateCameraViewMatrix(glm::vec3 in_camera_position,
                                glm::vec3 in_camera_looking_direction,
                                glm::vec3 in_camera_up);

    void UpdateCameraPerspectiveMatrix(float fovy,
                                       float aspect,
                                       float near,
                                       float far);
public:
    ViewportFrustum cameraViewportFrustum;
    ViewportFrustum cullingViewportFrustum;

    Entity thisEntity;
private: // static_variable
    friend class CameraComp;
    static CameraComp* cameraComp_ptr;
};