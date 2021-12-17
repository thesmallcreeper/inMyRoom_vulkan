#include "Geometry/Paralgram.h"


Paralgram Paralgram::MultiplyBy4x4Matrix(const glm::mat4x4& in_matrix, const Paralgram& rhs)
{
    Paralgram return_paralgram;

    return_paralgram.center = glm::vec3(in_matrix * glm::vec4(rhs.center, 1.f));

    return_paralgram.sideDirections.u = glm::vec3(in_matrix * glm::vec4(rhs.sideDirections.u, 0.f));
    return_paralgram.sideDirections.v = glm::vec3(in_matrix * glm::vec4(rhs.sideDirections.v, 0.f));
    return_paralgram.sideDirections.w = glm::vec3(in_matrix * glm::vec4(rhs.sideDirections.w, 0.f));

    return return_paralgram;
}

bool Paralgram::IntersectParalgramsBoolean(const Paralgram& lhs, const Paralgram& rhs)
{
    // lhs based tests
    {   // 1. lhs U axis face
        glm::vec3 lhs_u_face = glm::cross(lhs.sideDirections.v, lhs.sideDirections.w);

        std::pair<float, float> lhs_projection = lhs.GetMinMaxProjectionToAxis(lhs_u_face);
        std::pair<float, float> rhs_projection = rhs.GetMinMaxProjectionToAxis(lhs_u_face);

        if (!DoMinMaxProjectionsToAxisIntersept(lhs_projection, rhs_projection))
            return false;
    }

    {   // 2. lhs V axis face
        glm::vec3 lhs_v_face = glm::cross(lhs.sideDirections.u, lhs.sideDirections.w);

        std::pair<float, float> lhs_projection = lhs.GetMinMaxProjectionToAxis(lhs_v_face);
        std::pair<float, float> rhs_projection = rhs.GetMinMaxProjectionToAxis(lhs_v_face);

        if (!DoMinMaxProjectionsToAxisIntersept(lhs_projection, rhs_projection))
            return false;
    }

    {   // 3. lhs W axis face
        glm::vec3 lhs_w_face = glm::cross(lhs.sideDirections.u, lhs.sideDirections.v);

        std::pair<float, float> lhs_projection = lhs.GetMinMaxProjectionToAxis(lhs_w_face);
        std::pair<float, float> rhs_projection = rhs.GetMinMaxProjectionToAxis(lhs_w_face);

        if (!DoMinMaxProjectionsToAxisIntersept(lhs_projection, rhs_projection))
            return false;
    }

    // rhs based tests
    {   // 4. rhs U axis face
        glm::vec3 rhs_u_face = glm::cross(rhs.sideDirections.v, rhs.sideDirections.w);

        std::pair<float, float> lhs_projection = lhs.GetMinMaxProjectionToAxis(rhs_u_face);
        std::pair<float, float> rhs_projection = rhs.GetMinMaxProjectionToAxis(rhs_u_face);

        if (!DoMinMaxProjectionsToAxisIntersept(lhs_projection, rhs_projection))
            return false;
    }

    {   // 5. rhs V axis face
        glm::vec3 rhs_v_face = glm::cross(rhs.sideDirections.u, rhs.sideDirections.w);

        std::pair<float, float> lhs_projection = lhs.GetMinMaxProjectionToAxis(rhs_v_face);
        std::pair<float, float> rhs_projection = rhs.GetMinMaxProjectionToAxis(rhs_v_face);

        if (!DoMinMaxProjectionsToAxisIntersept(lhs_projection, rhs_projection))
            return false;
    }

    {   // 6. rhs W axis face
        glm::vec3 rhs_w_face = glm::cross(rhs.sideDirections.u, rhs.sideDirections.v);

        std::pair<float, float> lhs_projection = lhs.GetMinMaxProjectionToAxis(rhs_w_face);
        std::pair<float, float> rhs_projection = rhs.GetMinMaxProjectionToAxis(rhs_w_face);

        if (!DoMinMaxProjectionsToAxisIntersept(lhs_projection, rhs_projection))
            return false;
    }

    // cross product based tests
    {   // 7. UxU
        glm::vec3 cross_product_u_u = glm::cross(lhs.sideDirections.u, rhs.sideDirections.u);

        std::pair<float, float> lhs_projection = lhs.GetMinMaxProjectionToAxis(cross_product_u_u);
        std::pair<float, float> rhs_projection = rhs.GetMinMaxProjectionToAxis(cross_product_u_u);

        if (!DoMinMaxProjectionsToAxisIntersept(lhs_projection, rhs_projection))
            return false;
    }

    {   // 8. UxV
        glm::vec3 cross_product_u_v = glm::cross(lhs.sideDirections.u, rhs.sideDirections.v);

        std::pair<float, float> lhs_projection = lhs.GetMinMaxProjectionToAxis(cross_product_u_v);
        std::pair<float, float> rhs_projection = rhs.GetMinMaxProjectionToAxis(cross_product_u_v);

        if (!DoMinMaxProjectionsToAxisIntersept(lhs_projection, rhs_projection))
            return false;
    }

    {   // 9. UxW
        glm::vec3 cross_product_u_w = glm::cross(lhs.sideDirections.u, rhs.sideDirections.w);

        std::pair<float, float> lhs_projection = lhs.GetMinMaxProjectionToAxis(cross_product_u_w);
        std::pair<float, float> rhs_projection = rhs.GetMinMaxProjectionToAxis(cross_product_u_w);

        if (!DoMinMaxProjectionsToAxisIntersept(lhs_projection, rhs_projection))
            return false;
    }

    {   // 10. VxU
        glm::vec3 cross_product_v_u = glm::cross(lhs.sideDirections.v, rhs.sideDirections.u);

        std::pair<float, float> lhs_projection = lhs.GetMinMaxProjectionToAxis(cross_product_v_u);
        std::pair<float, float> rhs_projection = rhs.GetMinMaxProjectionToAxis(cross_product_v_u);

        if (!DoMinMaxProjectionsToAxisIntersept(lhs_projection, rhs_projection))
            return false;
    }

    {   // 11. VxV
        glm::vec3 cross_product_v_v = glm::cross(lhs.sideDirections.v, rhs.sideDirections.v);

        std::pair<float, float> lhs_projection = lhs.GetMinMaxProjectionToAxis(cross_product_v_v);
        std::pair<float, float> rhs_projection = rhs.GetMinMaxProjectionToAxis(cross_product_v_v);

        if (!DoMinMaxProjectionsToAxisIntersept(lhs_projection, rhs_projection))
            return false;
    }

    {   // 12. VxW
        glm::vec3 cross_product_v_w = glm::cross(lhs.sideDirections.v, rhs.sideDirections.w);

        std::pair<float, float> lhs_projection = lhs.GetMinMaxProjectionToAxis(cross_product_v_w);
        std::pair<float, float> rhs_projection = rhs.GetMinMaxProjectionToAxis(cross_product_v_w);

        if (!DoMinMaxProjectionsToAxisIntersept(lhs_projection, rhs_projection))
            return false;
    }

    {   // 13. WxU
        glm::vec3 cross_product_w_u = glm::cross(lhs.sideDirections.w, rhs.sideDirections.u);

        std::pair<float, float> lhs_projection = lhs.GetMinMaxProjectionToAxis(cross_product_w_u);
        std::pair<float, float> rhs_projection = rhs.GetMinMaxProjectionToAxis(cross_product_w_u);

        if (!DoMinMaxProjectionsToAxisIntersept(lhs_projection, rhs_projection))
            return false;
    }

    {   // 14. WxV
        glm::vec3 cross_product_w_v = glm::cross(lhs.sideDirections.w, rhs.sideDirections.v);

        std::pair<float, float> lhs_projection = lhs.GetMinMaxProjectionToAxis(cross_product_w_v);
        std::pair<float, float> rhs_projection = rhs.GetMinMaxProjectionToAxis(cross_product_w_v);

        if (!DoMinMaxProjectionsToAxisIntersept(lhs_projection, rhs_projection))
            return false;
    }

    {   // 15. WxW
        glm::vec3 cross_product_w_w = glm::cross(lhs.sideDirections.w, rhs.sideDirections.w);

        std::pair<float, float> lhs_projection = lhs.GetMinMaxProjectionToAxis(cross_product_w_w);
        std::pair<float, float> rhs_projection = rhs.GetMinMaxProjectionToAxis(cross_product_w_w);

        if (!DoMinMaxProjectionsToAxisIntersept(lhs_projection, rhs_projection))
            return false;
    }

    return true;
}

std::pair<float, float> Paralgram::GetMinMaxProjectionToAxis(const glm::vec3& in_axis) const
{
    float center_projection = GetCenterProjectionToAxis(in_axis);

    float direction_U_abs_projection = std::abs(glm::dot(in_axis, sideDirections.u));
    float direction_V_abs_projection = std::abs(glm::dot(in_axis, sideDirections.v));
    float direction_W_abs_projection = std::abs(glm::dot(in_axis, sideDirections.w));

    float directions_projections_sum = direction_U_abs_projection + direction_V_abs_projection + direction_W_abs_projection;

    std::pair<float, float> return_pair;
    return_pair.first = center_projection - directions_projections_sum;
    return_pair.second = center_projection + directions_projections_sum;

    return return_pair;
}

float Paralgram::GetCenterProjectionToAxis(const glm::vec3& in_axis) const
{
    float center_projection = glm::dot(center, in_axis);
    return center_projection;
}

bool Paralgram::DoMinMaxProjectionsToAxisIntersept(const std::pair<float, float>& lhs, const std::pair<float, float>& rhs)
{
    return (lhs.second >= rhs.first) && (rhs.second >= lhs.first);
}

float Paralgram::GetSurface() const
{
    float u_v_surface = glm::length(glm::cross(glm::vec3(sideDirections.u), glm::vec3(sideDirections.v)));
    float u_w_surface = glm::length(glm::cross(glm::vec3(sideDirections.u), glm::vec3(sideDirections.w)));
    float v_w_surface = glm::length(glm::cross(glm::vec3(sideDirections.v), glm::vec3(sideDirections.w)));

    return 2.f * (u_v_surface + u_w_surface + v_w_surface);
}
