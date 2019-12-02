#include "Geometry/ViewportFrustum.h"

ViewportFrustum::ViewportFrustum()
{
    perspectiveMatrix = glm::mat4x4();
    viewMatrix = glm::mat4x4();
}

ViewportFrustum::~ViewportFrustum()
{
}

glm::mat4x4 ViewportFrustum::GetPerspectiveMatrix() const
{
    return perspectiveMatrix;
}
glm::mat4x4 ViewportFrustum::GetViewMatrix() const
{
    return viewMatrix;
}
glm::mat4x4 ViewportFrustum::GetCombinedMatrix() const
{
    return perspectiveMatrix * viewMatrix;
}

void ViewportFrustum::UpdatePerspectiveMatrix(float fovy,
                                              float aspect,
                                              float near,
                                              float far)
{
    perspectiveMatrix = glm::perspective(fovy, aspect, near, far)
                      * glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,    // Multiply with diag(1,-1,1,1) in order to make glm::perspective "vulkan-ready"                                                                                                                                                                                                                       
                                  0.0f, -1.0f, 0.0f, 0.0f,
                                  0.0f, 0.0f, 1.0f, 0.0f,
                                  0.0f, 0.0f, 0.0f, 1.0f);
}

void ViewportFrustum::UpdateViewMatrix(glm::vec3 in_camera_position,
                                       glm::vec3 in_camera_looking_direction,
                                       glm::vec3 in_camera_up)
{
    viewMatrix = glm::lookAt(in_camera_position,
                             in_camera_position + in_camera_looking_direction,
                             in_camera_up);
}

std::array<Plane, 6> ViewportFrustum::GetWorldSpacePlanesOfFrustum() const
{
    // Copyied and modified code from here: https://github.com/SaschaWillems/Vulkan/blob/master/base/frustum.hpp
    // Creator's copyrights:
    /*
    *View frustum culling class
        *
        * Copyright(C) 2016 by Sascha Willems - www.saschawillems.de
        *
        *This code is licensed under the MIT license(MIT) (http ://opensource.org/licenses/MIT)
    */

    std::array<glm::vec4, 6> planes_glm;
    glm::mat4x4 matrix = perspectiveMatrix * viewMatrix;

    planes_glm[LEFT].x = matrix[0].w + matrix[0].x;
    planes_glm[LEFT].y = matrix[1].w + matrix[1].x;
    planes_glm[LEFT].z = matrix[2].w + matrix[2].x;
    planes_glm[LEFT].w = matrix[3].w + matrix[3].x;

    planes_glm[RIGHT].x = matrix[0].w - matrix[0].x;
    planes_glm[RIGHT].y = matrix[1].w - matrix[1].x;
    planes_glm[RIGHT].z = matrix[2].w - matrix[2].x;
    planes_glm[RIGHT].w = matrix[3].w - matrix[3].x;

    planes_glm[TOP].x = matrix[0].w - matrix[0].y;
    planes_glm[TOP].y = matrix[1].w - matrix[1].y;
    planes_glm[TOP].z = matrix[2].w - matrix[2].y;
    planes_glm[TOP].w = matrix[3].w - matrix[3].y;

    planes_glm[BOTTOM].x = matrix[0].w + matrix[0].y;
    planes_glm[BOTTOM].y = matrix[1].w + matrix[1].y;
    planes_glm[BOTTOM].z = matrix[2].w + matrix[2].y;
    planes_glm[BOTTOM].w = matrix[3].w + matrix[3].y;

    planes_glm[BACK].x = matrix[0].z;
    planes_glm[BACK].y = matrix[1].z;
    planes_glm[BACK].z = matrix[2].z;
    planes_glm[BACK].w = matrix[3].z;

    planes_glm[FRONT].x = matrix[0].w - matrix[0].z;
    planes_glm[FRONT].y = matrix[1].w - matrix[1].z;
    planes_glm[FRONT].z = matrix[2].w - matrix[2].z;
    planes_glm[FRONT].w = matrix[3].w - matrix[3].z;

    std::array<Plane, 6> planes;

    for (size_t i = 0; i < planes_glm.size(); i++)
    {
        glm::vec3 this_planes_normal = - glm::vec3(planes_glm[i].x, planes_glm[i].y, planes_glm[i].z);
        float this_planes_d = - planes_glm[i].w;

        Plane this_plane = Plane::CreatePlane(this_planes_normal, this_planes_d);
        planes[i] = this_plane;
    }

    return planes;
}

//ViewportFrustum& ViewportFrustum::operator=(const ViewportFrustum& other)
//{
//    perspectiveMatrix = other.perspectiveMatrix;
//    viewMatrix = other.viewMatrix;
//    return *this;
//}
