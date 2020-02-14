#pragma once

#include <vector>

#include "Geometry/Ray.h"

#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "glm/mat4x4.hpp"

#include "glTFenum.h"

struct TrianglesInterseptionInfo
{
    bool doIntersept;
    bool areCoplanar;
    glm::vec3 source;
    glm::vec3 target;
};

struct TriangleRayInterseptionInfo
{
    bool doIntersept;
    float distanceFromOrigin;
};

class Triangle
{
public:
    static Triangle MultiplyBy4x4Matrix(const glm::mat4x4& in_matrix, const Triangle& rhs);

    static Triangle CreateTriangle(const glm::vec3 p0, const glm::vec3 p1, const glm::vec3 p2);

    static std::vector<Triangle> CreateTriangleList(const std::vector<glm::vec3>& points,
                                                    const std::vector<uint32_t>& indices,
                                                    const glTFmode triangleMode);

    static TrianglesInterseptionInfo InterseptTrianglesInfo(const Triangle& lhs, const Triangle& rhs);
    static TriangleRayInterseptionInfo InterseptTriangleWithRayInfo(const Triangle& triange, const Ray& ray);

    std::pair<float, float> GetMinMaxProjectionToAxis(const glm::vec4& in_axis) const;
public:
    glm::vec4 GetP0() const;
    glm::vec4 GetP1() const;
    glm::vec4 GetP2() const;

private:
    glm::vec4 p0;
    glm::vec4 p1;
    glm::vec4 p2;

};

inline Triangle operator* (const glm::mat4x4& in_matrix, const Triangle& rhs)
{
    return Triangle::MultiplyBy4x4Matrix(in_matrix, rhs);
}

/* Triangle/triangle intersection test routine,
 * by Tomas Moller, 1997.
 * See article "A Fast Triangle-Triangle Intersection Test",
 * Journal of Graphics Tools, 2(2), 1997
 * updated: 2001-06-20 (added line of intersection)
 */
int tri_tri_intersect(float V0[3], float V1[3], float V2[3],
                      float U0[3], float U1[3], float U2[3]);
/*
 * parameters: vertices of triangle 1: V0,V1,V2
 *             vertices of triangle 2: U0,U1,U2
 * result    : returns 1 if the triangles intersect, otherwise 0
 *
 * Here is a version withouts divisions (a little faster)
 */
   int NoDivTriTriIsect(float V0[3],float V1[3],float V2[3],
                        float U0[3],float U1[3],float U2[3]);
/*
 * This version computes the line of intersection as well (if they are not coplanar):
 */
   int tri_tri_intersect_with_isectline(float V0[3],float V1[3],float V2[3],
  				                        float U0[3],float U1[3],float U2[3],
                                        int *coplanar,
  				                        float isectpt1[3],float isectpt2[3]);
/* coplanar returns whether the tris are coplanar
 * isectpt1, isectpt2 are the endpoints of the line of intersection
 */