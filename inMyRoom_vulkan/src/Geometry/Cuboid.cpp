#include "Geometry/Cuboid.h"


Cuboid Cuboid::MultiplyBy4x4Matrix(const glm::mat4x4& in_matrix, const Cuboid& rhs)
{
    Cuboid return_cuboid;

    return_cuboid.center = in_matrix * rhs.center;

    return_cuboid.sideDirections.u = glm::normalize(in_matrix * rhs.sideDirections.u * rhs.halfLengths.x);
    return_cuboid.halfLengths.x = glm::length(in_matrix * rhs.sideDirections.u * rhs.halfLengths.x);
    return_cuboid.sideDirections.v = glm::normalize(in_matrix * rhs.sideDirections.v * rhs.halfLengths.y);
    return_cuboid.halfLengths.y = glm::length(in_matrix * rhs.sideDirections.v * rhs.halfLengths.y);
    return_cuboid.sideDirections.w = glm::normalize(in_matrix * rhs.sideDirections.w * rhs.halfLengths.z);
    return_cuboid.halfLengths.z = glm::length(in_matrix * rhs.sideDirections.w * rhs.halfLengths.z);

    return return_cuboid;
}

bool Cuboid::IntersectCuboidsBoolean(const Cuboid& lhs, const Cuboid& rhs)
{
    // lhs based tests
    {   // 1. lhs U axis
        glm::vec4 axis = lhs.sideDirections.u;

        std::pair<float, float> lhs_projection = std::make_pair(lhs.GetCenterProjectionToAxis(axis) - lhs.halfLengths.x,    //min
                                                                lhs.GetCenterProjectionToAxis(axis) + lhs.halfLengths.x);   //max

        std::pair<float, float> rhs_projection = rhs.GetMinMaxProjectionToAxis(axis);

        if (!DoMinMaxProjectionsToAxisIntersept(lhs_projection, rhs_projection))
            return false;
    }

    {   // 2. lhs V axis
        glm::vec4 axis = lhs.sideDirections.v;

        std::pair<float, float> lhs_projection = std::make_pair(lhs.GetCenterProjectionToAxis(axis) - lhs.halfLengths.y,    //min
                                                                lhs.GetCenterProjectionToAxis(axis) + lhs.halfLengths.y);   //max

        std::pair<float, float> rhs_projection = rhs.GetMinMaxProjectionToAxis(axis);

        if (!DoMinMaxProjectionsToAxisIntersept(lhs_projection, rhs_projection))
            return false;
    }

    {   // 3. lhs W axis
        glm::vec4 axis = lhs.sideDirections.w;

        std::pair<float, float> lhs_projection = std::make_pair(lhs.GetCenterProjectionToAxis(axis) - lhs.halfLengths.z,    //min
                                                                lhs.GetCenterProjectionToAxis(axis) + lhs.halfLengths.z);   //max

        std::pair<float, float> rhs_projection = rhs.GetMinMaxProjectionToAxis(axis);

        if (!DoMinMaxProjectionsToAxisIntersept(lhs_projection, rhs_projection))
            return false;
    }

    // rhs based tests
    {   // 4. rhs U axis
        glm::vec4 axis = rhs.sideDirections.u;

        std::pair<float, float> lhs_projection = lhs.GetMinMaxProjectionToAxis(axis);

        std::pair<float, float> rhs_projection = std::make_pair(rhs.GetCenterProjectionToAxis(axis) - rhs.halfLengths.x,    //min
                                                                rhs.GetCenterProjectionToAxis(axis) + rhs.halfLengths.x);   //max

        if (!DoMinMaxProjectionsToAxisIntersept(lhs_projection, rhs_projection))
            return false;
    }

    {   // 5. rhs V axis
        glm::vec4 axis = rhs.sideDirections.v;

        std::pair<float, float> lhs_projection = lhs.GetMinMaxProjectionToAxis(axis);

        std::pair<float, float> rhs_projection = std::make_pair(rhs.GetCenterProjectionToAxis(axis) - rhs.halfLengths.y,    //min
                                                                rhs.GetCenterProjectionToAxis(axis) + rhs.halfLengths.y);   //max

        if (!DoMinMaxProjectionsToAxisIntersept(lhs_projection, rhs_projection))
            return false;
    }

    {   // 6. rhs W axis
        glm::vec4 axis = rhs.sideDirections.w;

        std::pair<float, float> lhs_projection = lhs.GetMinMaxProjectionToAxis(axis);

        std::pair<float, float> rhs_projection = std::make_pair(rhs.GetCenterProjectionToAxis(axis) - rhs.halfLengths.z,    //min
                                                                rhs.GetCenterProjectionToAxis(axis) + rhs.halfLengths.z);   //max

        if (!DoMinMaxProjectionsToAxisIntersept(lhs_projection, rhs_projection))
            return false;
    }

    // cross product based tests
    {   // 7. UxU
        glm::vec4 cross_product_u_u = glm::vec4(glm::cross(glm::vec3(lhs.sideDirections.u), glm::vec3(rhs.sideDirections.u)), 0.f);
        if (cross_product_u_u != glm::vec4(0.f, 0.f, 0.f, 0.f))
        {
            glm::vec4 axis = glm::normalize(cross_product_u_u);

            std::pair<float, float> lhs_projection = lhs.GetMinMaxProjectionToAxis(axis);
            std::pair<float, float> rhs_projection = rhs.GetMinMaxProjectionToAxis(axis);

            if (!DoMinMaxProjectionsToAxisIntersept(lhs_projection, rhs_projection))
                return false;
        }
    }

    {   // 8. UxV
        glm::vec4 cross_product_u_v = glm::vec4(glm::cross(glm::vec3(lhs.sideDirections.u), glm::vec3(rhs.sideDirections.v)), 0.f);
        if (cross_product_u_v != glm::vec4(0.f, 0.f, 0.f, 0.f))
        {
            glm::vec4 axis = glm::normalize(cross_product_u_v);

            std::pair<float, float> lhs_projection = lhs.GetMinMaxProjectionToAxis(axis);
            std::pair<float, float> rhs_projection = rhs.GetMinMaxProjectionToAxis(axis);

            if (!DoMinMaxProjectionsToAxisIntersept(lhs_projection, rhs_projection))
                return false;
        }
    }

    {   // 9. UxW
        glm::vec4 cross_product_u_w = glm::vec4(glm::cross(glm::vec3(lhs.sideDirections.u), glm::vec3(rhs.sideDirections.w)), 0.f);
        if (cross_product_u_w != glm::vec4(0.f, 0.f, 0.f, 0.f))
        {
            glm::vec4 axis = glm::normalize(cross_product_u_w);

            std::pair<float, float> lhs_projection = lhs.GetMinMaxProjectionToAxis(axis);
            std::pair<float, float> rhs_projection = rhs.GetMinMaxProjectionToAxis(axis);

            if (!DoMinMaxProjectionsToAxisIntersept(lhs_projection, rhs_projection))
                return false;
        }
    }

    {   // 10. VxU
        glm::vec4 cross_product_v_u = glm::vec4(glm::cross(glm::vec3(lhs.sideDirections.v), glm::vec3(rhs.sideDirections.u)), 0.f);
        if (cross_product_v_u != glm::vec4(0.f, 0.f, 0.f, 0.f))
        {
            glm::vec4 axis = glm::normalize(cross_product_v_u);

            std::pair<float, float> lhs_projection = lhs.GetMinMaxProjectionToAxis(axis);
            std::pair<float, float> rhs_projection = rhs.GetMinMaxProjectionToAxis(axis);

            if (!DoMinMaxProjectionsToAxisIntersept(lhs_projection, rhs_projection))
                return false;
        }
    }

    {   // 11. VxV
        glm::vec4 cross_product_v_v = glm::vec4(glm::cross(glm::vec3(lhs.sideDirections.v), glm::vec3(rhs.sideDirections.v)), 0.f);
        if (cross_product_v_v != glm::vec4(0.f, 0.f, 0.f, 0.f))
        {
            glm::vec4 axis = glm::normalize(cross_product_v_v);

            std::pair<float, float> lhs_projection = lhs.GetMinMaxProjectionToAxis(axis);
            std::pair<float, float> rhs_projection = rhs.GetMinMaxProjectionToAxis(axis);

            if (!DoMinMaxProjectionsToAxisIntersept(lhs_projection, rhs_projection))
                return false;
        }
    }

    {   // 12. VxW
        glm::vec4 cross_product_v_w = glm::vec4(glm::cross(glm::vec3(lhs.sideDirections.v), glm::vec3(rhs.sideDirections.w)), 0.f);
        if (cross_product_v_w != glm::vec4(0.f, 0.f, 0.f, 0.f))
        {
            glm::vec4 axis = glm::normalize(cross_product_v_w);

            std::pair<float, float> lhs_projection = lhs.GetMinMaxProjectionToAxis(axis);
            std::pair<float, float> rhs_projection = rhs.GetMinMaxProjectionToAxis(axis);

            if (!DoMinMaxProjectionsToAxisIntersept(lhs_projection, rhs_projection))
                return false;
        }
    }

    {   // 13. WxU
        glm::vec4 cross_product_w_u = glm::vec4(glm::cross(glm::vec3(lhs.sideDirections.w), glm::vec3(rhs.sideDirections.u)), 0.f);
        if (cross_product_w_u != glm::vec4(0.f, 0.f, 0.f, 0.f))
        {
            glm::vec4 axis = glm::normalize(cross_product_w_u);

            std::pair<float, float> lhs_projection = lhs.GetMinMaxProjectionToAxis(axis);
            std::pair<float, float> rhs_projection = rhs.GetMinMaxProjectionToAxis(axis);

            if (!DoMinMaxProjectionsToAxisIntersept(lhs_projection, rhs_projection))
                return false;
        }
    }

    {   // 14. WxV
        glm::vec4 cross_product_w_v = glm::vec4(glm::cross(glm::vec3(lhs.sideDirections.w), glm::vec3(rhs.sideDirections.v)), 0.f);
        if (cross_product_w_v != glm::vec4(0.f, 0.f, 0.f, 0.f))
        {
            glm::vec4 axis = glm::normalize(cross_product_w_v);

            std::pair<float, float> lhs_projection = lhs.GetMinMaxProjectionToAxis(axis);
            std::pair<float, float> rhs_projection = rhs.GetMinMaxProjectionToAxis(axis);

            if (!DoMinMaxProjectionsToAxisIntersept(lhs_projection, rhs_projection))
                return false;
        }
    }

    {   // 15. WxW
        glm::vec4 cross_product_w_w = glm::vec4(glm::cross(glm::vec3(lhs.sideDirections.w), glm::vec3(rhs.sideDirections.w)), 0.f);
        if (cross_product_w_w != glm::vec4(0.f, 0.f, 0.f, 0.f))
        {
            glm::vec4 axis = glm::normalize(cross_product_w_w);

            std::pair<float, float> lhs_projection = lhs.GetMinMaxProjectionToAxis(axis);
            std::pair<float, float> rhs_projection = rhs.GetMinMaxProjectionToAxis(axis);

            if (!DoMinMaxProjectionsToAxisIntersept(lhs_projection, rhs_projection))
                return false;
        }
    }

    return true;
}

std::pair<bool, float> Cuboid::IntersectCuboidWithRayBooleanDistance(const Cuboid& cuboid, const Ray& ray)
{
    // Code based on Realtime Rendering (RTR) 4th edition p. 960
    static const float epsilon = 1.0E-16;

    float min_distance = -std::numeric_limits<float>::infinity();
    float max_distance = +std::numeric_limits<float>::infinity();

    glm::vec4 _P = cuboid.center - ray.GetOrigin();

    {  // 1. U test
        float e = glm::dot(cuboid.sideDirections.u, _P);
        float f = glm::dot(cuboid.sideDirections.u, ray.GetDirection());

        if (std::abs(f) > epsilon)
        {
            float t1 = (e + cuboid.halfLengths.x) / f;
            float t2 = (e - cuboid.halfLengths.x) / f;
            if (t1 > t2) std::swap(t1, t2);

            if (t1 > min_distance) min_distance = t1;
            if (t2 < max_distance) max_distance = t2;

            if(min_distance > max_distance || max_distance < 0.f)
                return std::make_pair(false, 0.f);
        }
        else if (-e - cuboid.halfLengths.x > 0.f || -e + cuboid.halfLengths.x < 0.f)
            return std::make_pair(false, 0.f);
    }

    {  // 2. V test
        float e = glm::dot(cuboid.sideDirections.v, _P);
        float f = glm::dot(cuboid.sideDirections.v, ray.GetDirection());

        if (std::abs(f) > epsilon)
        {
            float t1 = (e + cuboid.halfLengths.y) / f;
            float t2 = (e - cuboid.halfLengths.y) / f;
            if (t1 > t2) std::swap(t1, t2);

            if (t1 > min_distance) min_distance = t1;
            if (t2 < max_distance) max_distance = t2;

            if (min_distance > max_distance || max_distance < 0.f)
                return std::make_pair(false, 0.f);
        }
        else if (-e - cuboid.halfLengths.y > 0.f || -e + cuboid.halfLengths.y < 0.f)
            return std::make_pair(false, 0.f);
    }

    {  // 3. W test
        float e = glm::dot(cuboid.sideDirections.w, _P);
        float f = glm::dot(cuboid.sideDirections.w, ray.GetDirection());

        if (std::abs(f) > epsilon)
        {
            float t1 = (e + cuboid.halfLengths.z) / f;
            float t2 = (e - cuboid.halfLengths.z) / f;
            if (t1 > t2) std::swap(t1, t2);

            if (t1 > min_distance) min_distance = t1;
            if (t2 < max_distance) max_distance = t2;

            if (min_distance > max_distance || max_distance < 0.f)
                return std::make_pair(false, 0.f);
        }
        else if (-e - cuboid.halfLengths.z > 0.f || -e + cuboid.halfLengths.z < 0.f)
            return std::make_pair(false, 0.f);
    }

    if (min_distance > 0.f)
        return std::make_pair(true, min_distance);
    else
        return std::make_pair(true, max_distance);
}

std::pair<float, float> Cuboid::GetMinMaxProjectionToAxis(const glm::vec4& in_axis) const
{
    float center_projection = GetCenterProjectionToAxis(in_axis);

    float direction_U_coeff = std::abs(glm::dot(in_axis, sideDirections.u));
    float direction_U_abs_projection = direction_U_coeff * halfLengths.x;

    float direction_V_coeff = std::abs(glm::dot(in_axis, sideDirections.v));
    float direction_V_abs_projection = direction_V_coeff * halfLengths.y;

    float direction_W_coeff = std::abs(glm::dot(in_axis, sideDirections.w));
    float direction_W_abs_projection = direction_W_coeff * halfLengths.z;

    float directions_projections_sum = direction_U_abs_projection + direction_V_abs_projection + direction_W_abs_projection;

    std::pair<float, float> return_pair;
    return_pair.first = center_projection - directions_projections_sum;
    return_pair.second = center_projection + directions_projections_sum;

    return return_pair;
}

float Cuboid::GetCenterProjectionToAxis(const glm::vec4& in_axis) const
{
    float center_projection = glm::dot(center, in_axis);
    return center_projection;
}

bool Cuboid::DoMinMaxProjectionsToAxisIntersept(const std::pair<float, float>& lhs, const std::pair<float, float>& rhs)
{
    return (lhs.second >= rhs.first) && (rhs.second >= lhs.first);
}

float Cuboid::GetSurface() const
{
    float u_v_surface = glm::length(glm::cross(glm::vec3(sideDirections.u), glm::vec3(sideDirections.v))) * halfLengths.x * halfLengths.y;
    float u_w_surface = glm::length(glm::cross(glm::vec3(sideDirections.u), glm::vec3(sideDirections.w))) * halfLengths.x * halfLengths.z;
    float v_w_surface = glm::length(glm::cross(glm::vec3(sideDirections.v), glm::vec3(sideDirections.w))) * halfLengths.y * halfLengths.z;

    return 2.f * (u_v_surface + u_w_surface + v_w_surface);
}

glm::vec4 Cuboid::GetCenter() const
{
    return center;
}

glm::vec4 Cuboid::GetSideDirectionU() const
{
    return sideDirections.u;
}
glm::vec4 Cuboid::GetSideDirectionV() const
{
    return sideDirections.v;
}
glm::vec4 Cuboid::GetSideDirectionW() const
{
    return sideDirections.w;
}

glm::vec3 Cuboid::GetHalfLengths() const
{
    return halfLengths;
}

