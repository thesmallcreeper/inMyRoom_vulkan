#include "Geometry/OBB.h"

#include "dito.h"

OBB OBB::CreateOBBfromPoints(const std::vector<glm::vec3>& in_points)
{
    DiTO::OBB<float> dito_OBB;
    const DiTO::Vector<float>* dito_points = reinterpret_cast<const DiTO::Vector<float>*>(in_points.data());

    DiTO_14(const_cast<DiTO::Vector<float>*>(dito_points), static_cast<int>(in_points.size()), dito_OBB);
    // const_casting was used in order to avoid editing dito.h/.cpp which indeed, according to its docs, does not modifies dito_points but does not make use of "const"

    OBB return_OBB;
    if (!std::isnan(dito_OBB.mid.x))
    {
        return_OBB.center = glm::vec4(dito_OBB.mid.x, dito_OBB.mid.y, dito_OBB.mid.z, 1);
        return_OBB.sideDirections.u = glm::vec4(dito_OBB.v0.x, dito_OBB.v0.y, dito_OBB.v0.z, 0);
        return_OBB.sideDirections.v = glm::vec4(dito_OBB.v1.x, dito_OBB.v1.y, dito_OBB.v1.z, 0);
        return_OBB.sideDirections.w = glm::vec4(dito_OBB.v2.x, dito_OBB.v2.y, dito_OBB.v2.z, 0);
        return_OBB.halfLengths = glm::vec3(dito_OBB.ext.x, dito_OBB.ext.y, dito_OBB.ext.z);
    }
    else
    {
        return_OBB = CreateAABBfromPoints(in_points);
    }
    return return_OBB;
}

OBB OBB::CreateOBBfromTriangles(const std::vector<Triangle>& in_triangles)
{
    std::vector<glm::vec3> points;

    for (const Triangle& this_triangle : in_triangles)
    {
        points.emplace_back(glm::vec3(this_triangle.GetP0()));
        points.emplace_back(glm::vec3(this_triangle.GetP1()));
        points.emplace_back(glm::vec3(this_triangle.GetP2()));
    }

    return CreateOBBfromPoints(points);
}

OBB OBB::CreateAABBfromPoints(const std::vector<glm::vec3>& in_points)
{
    OBB return_OBB;
    return_OBB.center = glm::vec4(0.f, 0.f, 0.f, 1.f);
    return_OBB.sideDirections.u = glm::vec4(1.f, 0.f, 0.f, 0.f);
    return_OBB.sideDirections.v = glm::vec4(0.f, 1.f, 0.f, 0.f);
    return_OBB.sideDirections.w = glm::vec4(0.f, 0.f, 1.f, 0.f);

    {
        float min = std::numeric_limits<float>::infinity();
        float max = -std::numeric_limits<float>::infinity();
        for (const glm::vec3 this_point : in_points)
        {
            float this_projection = glm::dot(glm::vec3(return_OBB.sideDirections.u), this_point);
            if (this_projection < min) min = this_projection;
            if (this_projection > max) max = this_projection;
        }

        float delta = max - min;
        float center_on_this_axis = (max + min) / 2.f;

        return_OBB.halfLengths.x = delta / 2.f;
        return_OBB.center.x = center_on_this_axis;
    }

    {
        float min = std::numeric_limits<float>::infinity();
        float max = -std::numeric_limits<float>::infinity();
        for (const glm::vec3 this_point : in_points)
        {
            float this_projection = glm::dot(glm::vec3(return_OBB.sideDirections.v), this_point);
            if (this_projection < min) min = this_projection;
            if (this_projection > max) max = this_projection;
        }

        float delta = max - min;
        float center_on_this_axis = (max + min) / 2.f;

        return_OBB.halfLengths.y = delta / 2.f;
        return_OBB.center.y = center_on_this_axis;
    }

    {
        float min = std::numeric_limits<float>::infinity();
        float max = -std::numeric_limits<float>::infinity();
        for (const glm::vec3 this_point : in_points)
        {
            float this_projection = glm::dot(glm::vec3(return_OBB.sideDirections.w), this_point);
            if (this_projection < min) min = this_projection;
            if (this_projection > max) max = this_projection;
        }

        float delta = max - min;
        float center_on_this_axis = (max + min) / 2.f;

        return_OBB.halfLengths.z = delta / 2.f;
        return_OBB.center.z = center_on_this_axis;
    }

    return return_OBB;
}
