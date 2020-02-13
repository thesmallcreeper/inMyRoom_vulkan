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

/*
 *  Triangle-Triangle Overlap Test Routines
 *  July, 2002
 *  Updated December 2003
 *
 *  This file contains C implementation of algorithms for
 *  performing two and three-dimensional triangle-triangle intersection test
 *  The algorithms and underlying theory are described in
 *
 * "Fast and Robust Triangle-Triangle Overlap Test
 *  Using Orientation Predicates"  P. Guigue - O. Devillers
 *
 *  Journal of Graphics Tools, 8(1), 2003
 *
 *  Several geometric predicates are defined.  Their parameters are all
 *  points.  Each point is an array of two or three float precision
 *  floating point numbers. The geometric predicates implemented in
 *  this file are:
 *
 *    int tri_tri_overlap_test_3d(p1,q1,r1,p2,q2,r2)
 *    int tri_tri_overlap_test_2d(p1,q1,r1,p2,q2,r2)
 *
 *    int tri_tri_intersection_test_3d(p1,q1,r1,p2,q2,r2,
 *                                     coplanar,source,target)
 *
 *       is a version that computes the segment of intersection when
 *       the triangles overlap (and are not coplanar)
 *
 *    each function returns 1 if the triangles (including their
 *    boundary) intersect, otherwise 0
 *
 *
 *  Other information are available from the Web page
 *  http:<i>//www.acm.org/jgt/papers/GuigueDevillers03/
 *
 */

 // modified by Aaron to better detect coplanarity

#define ZERO_TEST(x)  (x == 0)
//#define ZERO_TEST(x)  ((x) > -0.001 && (x) < .001)

/* function prototype */

int tri_tri_overlap_test_3d(float p1[3], float q1[3], float r1[3],
                            float p2[3], float q2[3], float r2[3]);


int coplanar_tri_tri3d(float  p1[3], float  q1[3], float  r1[3],
                       float  p2[3], float  q2[3], float  r2[3],
                       float  N1[3], float  N2[3]);


int tri_tri_overlap_test_2d(float p1[2], float q1[2], float r1[2],
                            float p2[2], float q2[2], float r2[2]);


int tri_tri_intersection_test_3d(float p1[3], float q1[3], float r1[3],
                                 float p2[3], float q2[3], float r2[3],
                                 int* coplanar,
                                 float source[3], float target[3]);

/* coplanar returns whether the triangles are coplanar
 *  source and target are the endpoints of the segment of
 *  intersection if it exists)
 */