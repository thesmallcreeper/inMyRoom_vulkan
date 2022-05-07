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
    float halfFov_tan = glm::tan(fovy / 2.f);

    perspectiveMatrix = glm::mat4(0.f);
    perspectiveMatrix[0][0] = +1.f / (aspect * halfFov_tan);
    perspectiveMatrix[1][1] = +1.f / (halfFov_tan);
    perspectiveMatrix[2][2] = +(far) / (far - near);
    perspectiveMatrix[2][3] = +1.f;
    perspectiveMatrix[3][2] = -(far * near) / (far - near);
}

void ViewportFrustum::UpdateViewMatrix(glm::vec3 in_camera_position,
                                       glm::vec3 in_camera_looking_direction,
                                       glm::vec3 in_camera_up)
{
    glm::vec3 camera_z = glm::normalize(in_camera_looking_direction);
    glm::vec3 camera_x = glm::normalize(glm::cross(-in_camera_up, camera_z));
    glm::vec3 camera_y = glm::cross(camera_z, camera_x);

    glm::mat4 inv_view_matrix = glm::mat4(glm::vec4(camera_x, 0.f),
                                          glm::vec4(camera_y, 0.f),
                                          glm::vec4(camera_z, 0.f),
                                          glm::vec4(in_camera_position, 1.f));

    viewMatrix = glm::inverse(inv_view_matrix);
}

#ifndef GAME_DLL
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

        Plane this_plane = Plane(this_planes_normal, this_planes_d);
        planes[i] = this_plane;
    }

    return planes;
}

std::array<glm::vec4, 3> ViewportFrustum::GetFullscreenpassTriangleNormals() const
{
    glm::vec4 origin = glm::vec4(0.f, 0.f, 0.f, 1.f);

    glm::mat4 pers_matrix = GetPerspectiveMatrix();
    glm::mat4 inv_pers_matrix = glm::inverse(pers_matrix);

    glm::vec4 down_right_normal;
    {
        glm::vec4 backproject_pos = inv_pers_matrix * glm::vec4(1.f, 1.f, 0.5f, 1.f);
        backproject_pos /= backproject_pos.w;

        down_right_normal = glm::normalize(backproject_pos - origin);
    }

    glm::vec4 down_left_normal;
    {
        glm::vec4 backproject_pos = inv_pers_matrix * glm::vec4(-1.f, 1.f, 0.5f, 1.f);
        backproject_pos /= backproject_pos.w;

        down_left_normal = glm::normalize(backproject_pos - origin);
    }

    glm::vec4 upper_left_normal;
    {
        glm::vec4 backproject_pos = inv_pers_matrix * glm::vec4(-1.f, -1.f, 0.5f, 1.f);
        backproject_pos /= backproject_pos.w;

        upper_left_normal = glm::normalize(backproject_pos - origin);
    }

    std::array<glm::vec4, 3> return_array = {2.f * down_right_normal - down_left_normal,
                                             down_left_normal,
                                             2.f * upper_left_normal - down_left_normal};

    return return_array;
}

std::array<glm::vec4, 3> ViewportFrustum::GetFullscreenpassTrianglePos() const
{
    std::array<glm::vec4, 3> return_array = {glm::vec4( 3.f,  1.f, 0.5f, 1.f),  // down-right vector (offscreen)
                                             glm::vec4(-1.f,  1.f, 0.5f, 1.f),  // down-left  vector (offscreen)
                                             glm::vec4(-1.f, -3.f, 0.5f, 1.f)}; // upper-left vector (offscreen)

    return return_array;
}

#endif

//ViewportFrustum& ViewportFrustum::operator=(const ViewportFrustum& other)
//{
//    perspectiveMatrix = other.perspectiveMatrix;
//    viewMatrix = other.viewMatrix;
//    return *this;
//}
