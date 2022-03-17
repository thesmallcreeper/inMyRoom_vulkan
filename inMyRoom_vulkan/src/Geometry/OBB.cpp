#include "Geometry/OBB.h"

#include <unordered_set>
#include <numeric>
#include "glm/gtc/type_ptr.hpp"
#include "eig3.h"
#include "hash_combine.h"

template<>
struct std::hash<glm::vec3>
{
    inline std::size_t operator()(glm::vec3 const& vec3) const noexcept
    {
        std::size_t s = 0;
        hash_combine(s, vec3.x);
        hash_combine(s, vec3.y);
        hash_combine(s, vec3.z);

        return s;
    }
};

OBB::OBB(const AABB &aabb)
{
    center = (aabb.max_coords + aabb.min_coords) / 2.f;

    glm::vec3 lengths = aabb.max_coords - center;
    sideDirections.u = glm::vec3(1.f, 0.f, 0.f) * lengths.x;
    sideDirections.v = glm::vec3(0.f, 1.f, 0.f) * lengths.y;
    sideDirections.w = glm::vec3(0.f, 0.f, 1.f) * lengths.z;
}

OBB OBB::CreateOBBfromPoints(const std::vector<glm::vec3>& points)
{
    std::unordered_set<glm::vec3> points_uset;
    for (const auto& this_point: points) {
        points_uset.emplace(this_point);
        if (points_uset.size() > 3) {
            break;
        }
    }

    size_t unique_points_count = points_uset.size();
    if (unique_points_count == 0) {
        return OBB::EmptyOBB();
    } else if (unique_points_count == 1) {
        OBB obb = OBB::EmptyOBB();
        obb.center = points[0];

        return obb;
    } else {
        glm::dvec3 sum = std::accumulate(points.begin(), points.end(), glm::dvec3(0., 0., 0.),
                                         [](glm::dvec3 sum, glm::vec3 in) -> glm::dvec3 {return sum + glm::dvec3(in);});
        glm::dvec3 mean = sum / double(points.size());

        double covariance_xx = std::inner_product(points.begin(), points.end(), points.begin(), 0.,
                                                  [](double sum, double in) -> double {return sum + in;},
                                                  [mean](glm::vec3 lhs, glm::vec3 rhs) -> double {return (lhs.x - mean.x)*(rhs.x - mean.x);});
        double covariance_yy = std::inner_product(points.begin(), points.end(), points.begin(), 0.,
                                                  [](double sum, double in) -> double {return sum + in;},
                                                  [mean](glm::vec3 lhs, glm::vec3 rhs) -> double {return (lhs.y - mean.y)*(rhs.y - mean.y);});
        double covariance_zz = std::inner_product(points.begin(), points.end(), points.begin(), 0.,
                                                  [](double sum, double in) -> double {return sum + in;},
                                                  [mean](glm::vec3 lhs, glm::vec3 rhs) -> double {return (lhs.z - mean.z)*(rhs.z - mean.z);});
        double covariance_xy = std::inner_product(points.begin(), points.end(), points.begin(), 0.,
                                                  [](double sum, double in) -> double {return sum + in;},
                                                  [mean](glm::vec3 lhs, glm::vec3 rhs) -> double {return (lhs.x - mean.x)*(rhs.y - mean.y);});
        double covariance_xz = std::inner_product(points.begin(), points.end(), points.begin(), 0.,
                                                  [](double sum, double in) -> double {return sum + in;},
                                                  [mean](glm::vec3 lhs, glm::vec3 rhs) -> double {return (lhs.x - mean.x)*(rhs.z - mean.z);});
        double covariance_yz = std::inner_product(points.begin(), points.end(), points.begin(), 0.,
                                                  [](double sum, double in) -> double {return sum + in;},
                                                  [mean](glm::vec3 lhs, glm::vec3 rhs) -> double {return (lhs.y - mean.y)*(rhs.z - mean.z);});

        glm::dmat3x3 cov_mat = {covariance_xx, covariance_xy, covariance_xz,
                                covariance_xy, covariance_yy, covariance_yz,
                                covariance_xz, covariance_yz, covariance_zz};
        cov_mat /= double(points.size());

        glm::dmat3x3 eigenvectors = {};
        glm::dvec3 eigenvalues = {};
        eigen_decomposition((double(*)[3])(glm::value_ptr(cov_mat)),
                            (double(*)[3])(glm::value_ptr(eigenvectors)),
                            glm::value_ptr(eigenvalues));

        return CreateAABBfromPoints(points,
                                    eigenvectors[0], eigenvectors[1], eigenvectors[2]);
    }
}

OBB OBB::CreateOBBfromTriangles(const std::vector<Triangle>& in_triangles)
{
    std::vector<glm::vec3> points;

    for (const Triangle& this_triangle : in_triangles)
    {
        points.emplace_back(glm::vec3(this_triangle.GetP(0)));
        points.emplace_back(glm::vec3(this_triangle.GetP(1)));
        points.emplace_back(glm::vec3(this_triangle.GetP(2)));
    }

    return CreateOBBfromPoints(points);
}

OBB OBB::CreateAABBfromPoints(const std::vector<glm::vec3>& points,
                              glm::dvec3 axis_u,
                              glm::dvec3 axis_v,
                              glm::dvec3 axis_w)
{
    OBB return_OBB;
    glm::dvec3 center = glm::dvec3(0., 0., 0.);

    {
        double min = std::numeric_limits<double>::infinity();
        double max = -std::numeric_limits<double>::infinity();
        for (const glm::vec3 this_point : points)
        {
            double this_projection = glm::dot(axis_u, glm::dvec3(this_point));
            min = std::min(this_projection, min);
            max = std::max(this_projection, max);
        }

        double delta = (max - min) + 2. * double(std::numeric_limits<float>::epsilon());
        double center_on_this_axis = (max + min) / 2.;

        center += center_on_this_axis * axis_u;
        return_OBB.sideDirections.u = (delta / 2.) * axis_u;
    }

    {
        double min = std::numeric_limits<double>::infinity();
        double max = -std::numeric_limits<double>::infinity();
        for (const glm::vec3 this_point : points)
        {
            double this_projection = glm::dot(axis_v, glm::dvec3(this_point));
            min = std::min(this_projection, min);
            max = std::max(this_projection, max);
        }

        double delta = (max - min) + 2. * double(std::numeric_limits<float>::epsilon());
        double center_on_this_axis = (max + min) / 2.;

        center += center_on_this_axis * axis_v;
        return_OBB.sideDirections.v = (delta / 2.) * axis_v;
    }

    {
        double min = std::numeric_limits<double>::infinity();
        double max = -std::numeric_limits<double>::infinity();
        for (const glm::vec3 this_point : points)
        {
            double this_projection = glm::dot(axis_w, glm::dvec3(this_point));
            min = std::min(this_projection, min);
            max = std::max(this_projection, max);
        }

        double delta = (max - min) + 2. * double(std::numeric_limits<float>::epsilon());
        double center_on_this_axis = (max + min) / 2.;

        center += center_on_this_axis * axis_w;
        return_OBB.sideDirections.w = (delta / 2.) * axis_w;
    }

    return_OBB.center = center;
    return return_OBB;
}

OBB OBB::EmptyOBB()
{
    OBB return_OBB;

    return_OBB.center = glm::vec3(0.f, 0.f, 0.f);
    return_OBB.sideDirections.u = glm::vec3(std::numeric_limits<float>::epsilon(), 0.f, 0.f);
    return_OBB.sideDirections.v = glm::vec3(0.f, std::numeric_limits<float>::epsilon(), 0.f);
    return_OBB.sideDirections.w = glm::vec3(0.f, 0.f, std::numeric_limits<float>::epsilon());

    return return_OBB;
}
