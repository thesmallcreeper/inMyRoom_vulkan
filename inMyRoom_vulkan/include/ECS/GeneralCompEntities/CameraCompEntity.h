#pragma once

#include "ECS/CompEntityBaseWrappedClass.h"

#include "Geometry/ViewportFrustum.h"

class CameraComp;

class CameraCompEntity :
    public CompEntityBaseWrappedClass<CameraComp>
{
#ifndef GAME_DLL
public:
    CameraCompEntity(Entity this_entity);

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
    static CameraCompEntity CreateComponentEntityByMap(Entity in_entity, std::string entity_name, const CompEntityInitMap& in_map);

    void Update(bool cull_debugging);

#endif
public: // public functions
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
};

#ifdef GAME_DLL
class CameraComp
    :public ComponentBaseWrappedClass<CameraCompEntity, static_cast<componentID>(componentIDenum::Camera), "Camera", sparse_set> {};
#else
#include "ECS/GeneralComponents/CameraComp.h"
#endif
