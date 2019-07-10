#pragma once

#define GLM_FORECE_DEPTH_ZERO_TO_ONE

#include "glm/mat4x4.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "Geometry/Plane.h"

#include <array>

class ViewportFrustum
{
public:
    enum side { LEFT = 0, RIGHT = 1, TOP = 2, BOTTOM = 3, BACK = 4, FRONT = 5 };

    ViewportFrustum();
    ~ViewportFrustum();

    glm::mat4x4 GetPerspectiveMatrix() const;
    glm::mat4x4 GetViewMatrix() const;
    glm::mat4x4 GetCombinedMatrix() const;

    void UpdatePerspectiveMatrix(float fovy,
                                 float aspect,
                                 float near,
                                 float far);

    void UpdateViewMatrix(glm::vec3 in_camera_position, 
                          glm::vec3 in_camera_looking_direction, 
                          glm::vec3 in_camera_up);

    std::array<math::Plane, 6> GetWorldSpacePlanesOfFrustum() const;

private:
    glm::mat4x4 perspectiveMatrix;
    glm::mat4x4 viewMatrix;
};