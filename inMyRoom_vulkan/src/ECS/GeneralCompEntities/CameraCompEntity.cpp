#include "ECS/GeneralCompEntities/CameraCompEntity.h"

#include "ECS/ECSwrapper.h"

#ifndef GAME_DLL

CameraCompEntity::CameraCompEntity(const Entity this_entity)
    :CompEntityBaseWrappedClass<CameraComp>(this_entity)
{
}

CameraCompEntity CameraCompEntity::GetEmpty()
{
    CameraCompEntity this_cameraCompEntity(0);

    this_cameraCompEntity.UpdateCameraPerspectiveMatrix(GetComponentPtr()->default_fovy,
                                                        GetComponentPtr()->default_aspect,
                                                        GetComponentPtr()->default_near,
                                                        GetComponentPtr()->default_far);

    this_cameraCompEntity.UpdateCameraViewMatrix(glm::vec3(0.f, 0.f, 0.f),
                                                 glm::vec3(0.f, 0.f, 1.f),
                                                 glm::vec3(0.f,-1.f, 0.f));

    this_cameraCompEntity.cullingViewportFrustum = this_cameraCompEntity.cameraViewportFrustum;

    return this_cameraCompEntity;
}

CameraCompEntity CameraCompEntity::CreateComponentEntityByMap(const Entity in_entity, std::string entity_name, const CompEntityInitMap& in_map)
{
    CameraCompEntity this_cameraCompEntity(in_entity);

    {
        // "FOVy",              fovy                    = float         (optional-default from config)
        float fovy = GetComponentPtr()->default_fovy;
        {
            auto search = in_map.floatMap.find("FOVy");
            if (search != in_map.floatMap.end())
                fovy = search->second;
        }

        // "aspect",            aspect                  = float         (optional-default from config)  
        float aspect = GetComponentPtr()->default_aspect;
        {
            auto search = in_map.floatMap.find("aspect");
            if (search != in_map.floatMap.end())
                aspect = search->second;
        }

        // "nearPlane",         near                    = float         (optional-default from config)
        float near  = GetComponentPtr()->default_near;
        {
            auto search = in_map.floatMap.find("nearPlane");
            if (search != in_map.floatMap.end())
                near = search->second;
        }

        // "farPlane",          far                     = float         (optional-default from config)
        float far = GetComponentPtr()->default_far;
        {
            auto search = in_map.floatMap.find("farPlane");
            if (search != in_map.floatMap.end())
                far = search->second;
        }

        this_cameraCompEntity.UpdateCameraPerspectiveMatrix(fovy,
                                                            aspect,
                                                            near,
                                                            far);
    }

    {
        // "GlobalPosition",    globalPosition          = vec4.xyz      (optional-default  0, 0, 0)
        glm::vec3 position = glm::vec3(0.f, 0.f, 0.f);
        {
            auto search = in_map.vec4Map.find("GlobalPosition");
            if (search != in_map.vec4Map.end())
                position = glm::vec3(search->second.x, search->second.y, search->second.z);
        }

        // "GlobalDirection",   globalDirection         = vec4.xyz      (optional-default  0, 0, 1)
        glm::vec3 direction = glm::vec3(0.f, 0.f, 1.f);
        {
            auto search = in_map.vec4Map.find("GlobalDirection");
            if (search != in_map.vec4Map.end())
                direction = glm::vec3(search->second.x, search->second.y, search->second.z);
        }

        // "UpDirection",       globalUp                = vec4.xyz      (optional-default  0,-1, 0)
        glm::vec3 up = glm::vec3(0.f, -1.f, 0.f);
        {
            auto search = in_map.vec4Map.find("UpDirection");
            if (search != in_map.vec4Map.end())
                up = glm::vec3(search->second.x, search->second.y, search->second.z);
        }

        this_cameraCompEntity.UpdateCameraViewMatrix(position,
                                                     direction,
                                                     up);

    }

    this_cameraCompEntity.cullingViewportFrustum = this_cameraCompEntity.cameraViewportFrustum;

    return this_cameraCompEntity;
}

void CameraCompEntity::Update(bool cull_debugging)
{
    if (!cull_debugging)
        cullingViewportFrustum = cameraViewportFrustum;
}

#endif

void CameraCompEntity::UpdateCameraPerspectiveMatrix(float fovy, float aspect, float near, float far)
{
    cameraViewportFrustum.UpdatePerspectiveMatrix(fovy,
                                                  aspect,
                                                  near,
                                                  far);
}

void CameraCompEntity::UpdateCameraViewMatrix(glm::vec3 in_camera_position, glm::vec3 in_camera_looking_direction, glm::vec3 in_camera_up)
{
    cameraViewportFrustum.UpdateViewMatrix(in_camera_position,
                                           in_camera_looking_direction,
                                           in_camera_up);
}