#pragma once

#include <vector>

#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "glm/mat4x4.hpp"

#include "glTFenum.h"

struct TrianglesIntersectionInfo
{
    bool doIntersept;
    bool areCoplanar;
    glm::vec3 source;
    glm::vec3 target;
};

class TrianglePosition
{
public:
    TrianglePosition() {};
    explicit TrianglePosition(glm::vec3 in_p0, glm::vec3 in_p1, glm::vec3 in_p2);

    std::pair<float, float> GetMinMaxProjectionToAxis(const glm::vec3& in_axis) const;
    glm::vec3 GetTriangleNormal() const;

    static TrianglePosition MultiplyBy4x4Matrix(const glm::mat4x4& in_matrix, const TrianglePosition& rhs);
    static TrianglesIntersectionInfo IntersectTriangles(const TrianglePosition& lhs, const TrianglePosition& rhs);

protected:
    static std::vector<TrianglePosition> CreateTrianglePositionList(const std::vector<glm::vec3>& points,
                                                                    const std::vector<uint32_t>& indices,
                                                                    const glTFmode triangleMode);
public:
    glm::vec3 GetP0() const;
    glm::vec3 GetP1() const;
    glm::vec3 GetP2() const;
private:
    glm::vec3 p0;
    glm::vec3 p1;
    glm::vec3 p2;
};

inline TrianglePosition operator* (const glm::mat4x4& in_matrix, const TrianglePosition& rhs)
{
    return TrianglePosition::MultiplyBy4x4Matrix(in_matrix, rhs);
}


class TriangleNormal
{
public:
    TriangleNormal() {};
    explicit TriangleNormal(glm::vec3 in_n0, glm::vec3 in_n1, glm::vec3 in_n2);
    explicit TriangleNormal(glm::vec3 n);

    glm::vec3 GetNormal(glm::vec2 baryCoords, const glm::mat3x3& corrected_matrix) const;

    static glm::mat3x3 GetNormalCorrectedMatrix(const glm::mat4x4& in_matrix);

protected:
    static std::vector<TriangleNormal> CreateTriangleNormalList(const std::vector<glm::vec3>& points,           // If no normal then fallback to triangles normal
                                                                const std::vector<glm::vec3>& normals,
                                                                const std::vector<uint32_t>& indices,
                                                                const glTFmode triangleMode);
public:
    glm::vec3 GetN0() const;
    glm::vec3 GetN1() const;
    glm::vec3 GetN2() const;
private:
    glm::vec3 n0;
    glm::vec3 n1;
    glm::vec3 n2;
};


class Triangle
    :
    public TrianglePosition,
    public TriangleNormal
{
public:
    Triangle() {};
    explicit Triangle(TrianglePosition in_position, TriangleNormal in_normal);

    TrianglePosition GetTrianglePosition() const;
    TriangleNormal GetTriangleNormal() const;

    static std::vector<Triangle> CreateTriangleList(const std::vector<glm::vec3>& points,
                                                    const std::vector<glm::vec3>& normals,
                                                    const std::vector<uint32_t>& indices,
                                                    const glTFmode triangleMode);

    // static std::pair<std::vector<TrianglePosition>, std::vector<TriangleNormal>> SplitTriangleVector(const std::vector<Triangle>& triangles);
};

// Copy pasta for triangle-triangle intersect

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