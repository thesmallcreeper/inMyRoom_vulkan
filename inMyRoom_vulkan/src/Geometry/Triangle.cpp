#include "Geometry/Triangle.h"

#include "glm/gtx/intersect.hpp"

Triangle Triangle::MultiplyBy4x4Matrix(const glm::mat4x4& in_matrix, const Triangle& rhs)
{
    Triangle return_triangle;

    return_triangle.p0 = in_matrix * rhs.p0;
    return_triangle.p1 = in_matrix * rhs.p1;
    return_triangle.p2 = in_matrix * rhs.p2;

    return return_triangle;
}

Triangle Triangle::CreateTriangle(const glm::vec3 in_p0, const glm::vec3 in_p1, const glm::vec3 in_p2)
{
    Triangle return_triangle;

    return_triangle.p0 = glm::vec4(in_p0, 1.f);
    return_triangle.p1 = glm::vec4(in_p1, 1.f);
    return_triangle.p2 = glm::vec4(in_p2, 1.f);

    return return_triangle;
}

std::vector<Triangle> Triangle::CreateTriangleList(const std::vector<glm::vec3>& points, const std::vector<uint32_t>& indices, const glTFmode triangleMode)
{
    std::vector<Triangle> return_vector;

    switch (triangleMode)
    {
        case glTFmode::points:
        {
            for (size_t i = 0; i < indices.size(); i++)
                return_vector.emplace_back(CreateTriangle(points[indices[i]], points[indices[i]], points[indices[i]]));
        }
        case glTFmode::line:
        {
            for (size_t i = 0; i < indices.size() / 2; i++)
                return_vector.emplace_back(CreateTriangle(points[indices[2 * i]], points[indices[2 * i]], points[indices[2 * i + 1]]));
        }
        case glTFmode::line_strip:
        {
            for (size_t i = 0; i < indices.size() - 1; i++)
                return_vector.emplace_back(CreateTriangle(points[indices[i]], points[indices[i]], points[indices[i + 1]]));
        }
        case glTFmode::triangles:
        {
            for (size_t i = 0; i < indices.size() / 3; i++)
                return_vector.emplace_back(CreateTriangle(points[indices[3 * i]], points[indices[3 * i + 1]], points[indices[3 * i + 2]]));
        }
        case glTFmode::triangle_strip:
        {
            for (size_t i = 0; i < indices.size() - 2; i++)
                return_vector.emplace_back(CreateTriangle(points[indices[i]], points[indices[i + (1 + i % 2)]], points[indices[i + (2 - i % 2)]]));
        }
        case glTFmode::triangle_fan:
        {
            for (size_t i = 0; i < indices.size() - 2; i++)
                return_vector.emplace_back(CreateTriangle(points[indices[i + 1]], points[indices[i + 2]], points[indices[0]]));
        }
    }

    return return_vector;
}

TrianglesInterseptionInfo Triangle::InterseptTrianglesInfo(const Triangle& lhs, const Triangle& rhs)
{
    TrianglesInterseptionInfo return_info;

    int _doIntersept_int;
    int _areCoplanar_int;

    _doIntersept_int = tri_tri_intersection_test_3d(reinterpret_cast<float*>(&lhs.GetP0()), reinterpret_cast<float*>(&rhs.GetP0()),
                                                    reinterpret_cast<float*>(&lhs.GetP1()), reinterpret_cast<float*>(&rhs.GetP1()),
                                                    reinterpret_cast<float*>(&lhs.GetP2()), reinterpret_cast<float*>(&rhs.GetP2()),
                                                    &_areCoplanar_int,
                                                    reinterpret_cast<float*>(&return_info.source),
                                                    reinterpret_cast<float*>(&return_info.target));

    return_info.doIntersept = static_cast<bool>(_doIntersept_int);
    return_info.areCoplanar = static_cast<bool>(_areCoplanar_int);

    return return_info;
}

TriangleRayInterseptionInfo Triangle::InterseptTriangleWithRayInfo(const Triangle& triange, const Ray& ray)
{
    TriangleRayInterseptionInfo return_info;

    glm::vec3 _p0 = glm::vec3(triange.p0);
    glm::vec3 _p1 = glm::vec3(triange.p1);
    glm::vec3 _p2 = glm::vec3(triange.p2);
    glm::vec3 origin = glm::vec3(ray.GetOrigin());
    glm::vec3 direction = glm::vec3(ray.GetDirection());

    glm::vec3 inter_position;

    return_info.doIntersept = glm::intersectLineTriangle(origin, direction, _p0, _p1, _p2, inter_position);
    return_info.distanceFromOrigin = glm::dot(direction, inter_position - origin);

    return return_info;
}

std::pair<float, float> Triangle::GetMinMaxProjectionToAxis(const glm::vec4& in_axis) const
{
    float min = +std::numeric_limits<float>::infinity();
    float max = -std::numeric_limits<float>::infinity();

    {
        float projection_p0 = glm::dot(in_axis, p0);

        if (projection_p0 < min) min = projection_p0;
        if (projection_p0 > max) max = projection_p0;
    }
    {
        float projection_p1 = glm::dot(in_axis, p1);

        if (projection_p1 < min) min = projection_p1;
        if (projection_p1 > max) max = projection_p1;
    }
    {
        float projection_p2 = glm::dot(in_axis, p2);

        if (projection_p2 < min) min = projection_p2;
        if (projection_p2 > max) max = projection_p2;
    }

    return std::make_pair(min, max);
}

glm::vec4 Triangle::GetP0() const
{
    return p0;
}

glm::vec4 Triangle::GetP1() const
{
    return p1;
}

glm::vec4 Triangle::GetP2() const
{
    return p2;
}


// below triangle-triangle intersept actual code
 /* some 3D macros */

#define CROSS(dest,v1,v2)                       \
dest[0]=v1[1]*v2[2]-v1[2]*v2[1]; \
dest[1]=v1[2]*v2[0]-v1[0]*v2[2]; \
dest[2]=v1[0]*v2[1]-v1[1]*v2[0];

#define DOT(v1,v2) (v1[0]*v2[0]+v1[1]*v2[1]+v1[2]*v2[2])

#define SUB(dest,v1,v2) dest[0]=v1[0]-v2[0]; \
dest[1]=v1[1]-v2[1]; \
dest[2]=v1[2]-v2[2]; 

#define SCALAR(dest,alpha,v) dest[0] = alpha * v[0]; \
dest[1] = alpha * v[1]; \
dest[2] = alpha * v[2];

#define CHECK_MIN_MAX(p1,q1,r1,p2,q2,r2) {\
SUB(v1,p2,q1)\
SUB(v2,p1,q1)\
CROSS(N1,v1,v2)\
SUB(v1,q2,q1)\
if (DOT(v1,N1) > 0.0f) return 0;\
SUB(v1,p2,p1)\
SUB(v2,r1,p1)\
CROSS(N1,v1,v2)\
SUB(v1,r2,p1) \
if (DOT(v1,N1) > 0.0f) return 0;\
else return 1; }



/* Permutation in a canonical form of T2's vertices */

#define TRI_TRI_3D(p1,q1,r1,p2,q2,r2,dp2,dq2,dr2) { \
if (dp2 > 0.0f) { \
if (dq2 > 0.0f) CHECK_MIN_MAX(p1,r1,q1,r2,p2,q2) \
else if (dr2 > 0.0f) CHECK_MIN_MAX(p1,r1,q1,q2,r2,p2)\
else CHECK_MIN_MAX(p1,q1,r1,p2,q2,r2) }\
else if (dp2 < 0.0f) { \
if (dq2 < 0.0f) CHECK_MIN_MAX(p1,q1,r1,r2,p2,q2)\
else if (dr2 < 0.0f) CHECK_MIN_MAX(p1,q1,r1,q2,r2,p2)\
else CHECK_MIN_MAX(p1,r1,q1,p2,q2,r2)\
} else { \
if (dq2 < 0.0f) { \
if (dr2 >= 0.0f)  CHECK_MIN_MAX(p1,r1,q1,q2,r2,p2)\
else CHECK_MIN_MAX(p1,q1,r1,p2,q2,r2)\
} \
else if (dq2 > 0.0f) { \
if (dr2 > 0.0f) CHECK_MIN_MAX(p1,r1,q1,p2,q2,r2)\
else  CHECK_MIN_MAX(p1,q1,r1,q2,r2,p2)\
} \
else  { \
if (dr2 > 0.0f) CHECK_MIN_MAX(p1,q1,r1,r2,p2,q2)\
else if (dr2 < 0.0f) CHECK_MIN_MAX(p1,r1,q1,r2,p2,q2)\
else return coplanar_tri_tri3d(p1,q1,r1,p2,q2,r2,N1,N2);\
}}}



/*
 *
 *  Three-dimensional Triangle-Triangle Overlap Test
 *
 */


int tri_tri_overlap_test_3d(float p1[3], float q1[3], float r1[3],

                            float p2[3], float q2[3], float r2[3])
{
    float dp1, dq1, dr1, dp2, dq2, dr2;
    float v1[3], v2[3];
    float N1[3], N2[3];

    /* Compute distance signs  of p1, q1 and r1 to the plane of
     triangle(p2,q2,r2) */


    SUB(v1, p2, r2)
        SUB(v2, q2, r2)
        CROSS(N2, v1, v2)

        SUB(v1, p1, r2)
        dp1 = DOT(v1, N2);
    SUB(v1, q1, r2)
        dq1 = DOT(v1, N2);
    SUB(v1, r1, r2)
        dr1 = DOT(v1, N2);

    if (((dp1 * dq1) > 0.0f) && ((dp1 * dr1) > 0.0f))  return 0;

    /* Compute distance signs  of p2, q2 and r2 to the plane of
     triangle(p1,q1,r1) */


    SUB(v1, q1, p1)
        SUB(v2, r1, p1)
        CROSS(N1, v1, v2)

        SUB(v1, p2, r1)
        dp2 = DOT(v1, N1);
    SUB(v1, q2, r1)
        dq2 = DOT(v1, N1);
    SUB(v1, r2, r1)
        dr2 = DOT(v1, N1);

    if (((dp2 * dq2) > 0.0f) && ((dp2 * dr2) > 0.0f)) return 0;

    /* Permutation in a canonical form of T1's vertices */




    if (dp1 > 0.0f) {
        if (dq1 > 0.0f) TRI_TRI_3D(r1, p1, q1, p2, r2, q2, dp2, dr2, dq2)
        else if (dr1 > 0.0f) TRI_TRI_3D(q1, r1, p1, p2, r2, q2, dp2, dr2, dq2)
        else TRI_TRI_3D(p1, q1, r1, p2, q2, r2, dp2, dq2, dr2)
    }
    else if (dp1 < 0.0f) {
        if (dq1 < 0.0f) TRI_TRI_3D(r1, p1, q1, p2, q2, r2, dp2, dq2, dr2)
        else if (dr1 < 0.0f) TRI_TRI_3D(q1, r1, p1, p2, q2, r2, dp2, dq2, dr2)
        else TRI_TRI_3D(p1, q1, r1, p2, r2, q2, dp2, dr2, dq2)
    }
    else {
        if (dq1 < 0.0f) {
            if (dr1 >= 0.0f) TRI_TRI_3D(q1, r1, p1, p2, r2, q2, dp2, dr2, dq2)
            else TRI_TRI_3D(p1, q1, r1, p2, q2, r2, dp2, dq2, dr2)
        }
        else if (dq1 > 0.0f) {
            if (dr1 > 0.0f) TRI_TRI_3D(p1, q1, r1, p2, r2, q2, dp2, dr2, dq2)
            else TRI_TRI_3D(q1, r1, p1, p2, q2, r2, dp2, dq2, dr2)
        }
        else {
            if (dr1 > 0.0f) TRI_TRI_3D(r1, p1, q1, p2, q2, r2, dp2, dq2, dr2)
            else if (dr1 < 0.0f) TRI_TRI_3D(r1, p1, q1, p2, r2, q2, dp2, dr2, dq2)
            else return coplanar_tri_tri3d(p1, q1, r1, p2, q2, r2, N1, N2);
        }
    }
};



int coplanar_tri_tri3d(float p1[3], float q1[3], float r1[3],
                       float p2[3], float q2[3], float r2[3],
                       float normal_1[3], float normal_2[3]) {

    float P1[2], Q1[2], R1[2];
    float P2[2], Q2[2], R2[2];

    float n_x, n_y, n_z;

    n_x = ((normal_1[0] < 0) ? -normal_1[0] : normal_1[0]);
    n_y = ((normal_1[1] < 0) ? -normal_1[1] : normal_1[1]);
    n_z = ((normal_1[2] < 0) ? -normal_1[2] : normal_1[2]);


    /* Projection of the triangles in 3D onto 2D such that the area of
     the projection is maximized. */


    if ((n_x > n_z) && (n_x >= n_y)) {
        // Project onto plane YZ

        P1[0] = q1[2]; P1[1] = q1[1];
        Q1[0] = p1[2]; Q1[1] = p1[1];
        R1[0] = r1[2]; R1[1] = r1[1];

        P2[0] = q2[2]; P2[1] = q2[1];
        Q2[0] = p2[2]; Q2[1] = p2[1];
        R2[0] = r2[2]; R2[1] = r2[1];

    }
    else if ((n_y > n_z) && (n_y >= n_x)) {
        // Project onto plane XZ

        P1[0] = q1[0]; P1[1] = q1[2];
        Q1[0] = p1[0]; Q1[1] = p1[2];
        R1[0] = r1[0]; R1[1] = r1[2];

        P2[0] = q2[0]; P2[1] = q2[2];
        Q2[0] = p2[0]; Q2[1] = p2[2];
        R2[0] = r2[0]; R2[1] = r2[2];

    }
    else {
        // Project onto plane XY

        P1[0] = p1[0]; P1[1] = p1[1];
        Q1[0] = q1[0]; Q1[1] = q1[1];
        R1[0] = r1[0]; R1[1] = r1[1];

        P2[0] = p2[0]; P2[1] = p2[1];
        Q2[0] = q2[0]; Q2[1] = q2[1];
        R2[0] = r2[0]; R2[1] = r2[1];
    }

    return tri_tri_overlap_test_2d(P1, Q1, R1, P2, Q2, R2);

};



/*
 *
 *  Three-dimensional Triangle-Triangle Intersection
 *
 */

 /*
  This macro is called when the triangles surely intersect
  It constructs the segment of intersection of the two triangles
  if they are not coplanar.
  */

#define CONSTRUCT_INTERSECTION(p1,q1,r1,p2,q2,r2) { \
SUB(v1,q1,p1) \
SUB(v2,r2,p1) \
CROSS(N,v1,v2) \
SUB(v,p2,p1) \
if (DOT(v,N) > 0.0f) {\
SUB(v1,r1,p1) \
CROSS(N,v1,v2) \
if (DOT(v,N) <= 0.0f) { \
SUB(v2,q2,p1) \
CROSS(N,v1,v2) \
if (DOT(v,N) > 0.0f) { \
SUB(v1,p1,p2) \
SUB(v2,p1,r1) \
alpha = DOT(v1,N2) / DOT(v2,N2); \
SCALAR(v1,alpha,v2) \
SUB(source,p1,v1) \
SUB(v1,p2,p1) \
SUB(v2,p2,r2) \
alpha = DOT(v1,N1) / DOT(v2,N1); \
SCALAR(v1,alpha,v2) \
SUB(target,p2,v1) \
return 1; \
} else { \
SUB(v1,p2,p1) \
SUB(v2,p2,q2) \
alpha = DOT(v1,N1) / DOT(v2,N1); \
SCALAR(v1,alpha,v2) \
SUB(source,p2,v1) \
SUB(v1,p2,p1) \
SUB(v2,p2,r2) \
alpha = DOT(v1,N1) / DOT(v2,N1); \
SCALAR(v1,alpha,v2) \
SUB(target,p2,v1) \
return 1; \
} \
} else { \
return 0; \
} \
} else { \
SUB(v2,q2,p1) \
CROSS(N,v1,v2) \
if (DOT(v,N) < 0.0f) { \
return 0; \
} else { \
SUB(v1,r1,p1) \
CROSS(N,v1,v2) \
if (DOT(v,N) >= 0.0f) { \
SUB(v1,p1,p2) \
SUB(v2,p1,r1) \
alpha = DOT(v1,N2) / DOT(v2,N2); \
SCALAR(v1,alpha,v2) \
SUB(source,p1,v1) \
SUB(v1,p1,p2) \
SUB(v2,p1,q1) \
alpha = DOT(v1,N2) / DOT(v2,N2); \
SCALAR(v1,alpha,v2) \
SUB(target,p1,v1) \
return 1; \
} else { \
SUB(v1,p2,p1) \
SUB(v2,p2,q2) \
alpha = DOT(v1,N1) / DOT(v2,N1); \
SCALAR(v1,alpha,v2) \
SUB(source,p2,v1) \
SUB(v1,p1,p2) \
SUB(v2,p1,q1) \
alpha = DOT(v1,N2) / DOT(v2,N2); \
SCALAR(v1,alpha,v2) \
SUB(target,p1,v1) \
return 1; \
}}}} 



#define TRI_TRI_INTER_3D(p1,q1,r1,p2,q2,r2,dp2,dq2,dr2) { \
if (dp2 > 0.0f) { \
if (dq2 > 0.0f) CONSTRUCT_INTERSECTION(p1,r1,q1,r2,p2,q2) \
else if (dr2 > 0.0f) CONSTRUCT_INTERSECTION(p1,r1,q1,q2,r2,p2)\
else CONSTRUCT_INTERSECTION(p1,q1,r1,p2,q2,r2) }\
else if (dp2 < 0.0f) { \
if (dq2 < 0.0f) CONSTRUCT_INTERSECTION(p1,q1,r1,r2,p2,q2)\
else if (dr2 < 0.0f) CONSTRUCT_INTERSECTION(p1,q1,r1,q2,r2,p2)\
else CONSTRUCT_INTERSECTION(p1,r1,q1,p2,q2,r2)\
} else { \
if (dq2 < 0.0f) { \
if (dr2 >= 0.0f)  CONSTRUCT_INTERSECTION(p1,r1,q1,q2,r2,p2)\
else CONSTRUCT_INTERSECTION(p1,q1,r1,p2,q2,r2)\
} \
else if (dq2 > 0.0f) { \
if (dr2 > 0.0f) CONSTRUCT_INTERSECTION(p1,r1,q1,p2,q2,r2)\
else  CONSTRUCT_INTERSECTION(p1,q1,r1,q2,r2,p2)\
} \
else  { \
if (dr2 > 0.0f) CONSTRUCT_INTERSECTION(p1,q1,r1,r2,p2,q2)\
else if (dr2 < 0.0f) CONSTRUCT_INTERSECTION(p1,r1,q1,r2,p2,q2)\
else { \
*coplanar = 1; \
return coplanar_tri_tri3d(p1,q1,r1,p2,q2,r2,N1,N2);\
} \
}} }


  /*
   The following version computes the segment of intersection of the
   two triangles if it exists.
   coplanar returns whether the triangles are coplanar
   source and target are the endpoints of the line segment of intersection
   */

int tri_tri_intersection_test_3d(float p1[3], float q1[3], float r1[3],
                                 float p2[3], float q2[3], float r2[3],
                                 int* coplanar,
                                 float source[3], float target[3])

{
    float dp1, dq1, dr1, dp2, dq2, dr2;
    float v1[3], v2[3], v[3];
    float N1[3], N2[3], N[3];
    float alpha;

    // Compute distance signs  of p1, q1 and r1 
    // to the plane of triangle(p2,q2,r2)


    SUB(v1, p2, r2)
        SUB(v2, q2, r2)
        CROSS(N2, v1, v2)

        SUB(v1, p1, r2)
        dp1 = DOT(v1, N2);
    SUB(v1, q1, r2)
        dq1 = DOT(v1, N2);
    SUB(v1, r1, r2)
        dr1 = DOT(v1, N2);

    if (((dp1 * dq1) > 0.0f) && ((dp1 * dr1) > 0.0f))  return 0;

    // Compute distance signs  of p2, q2 and r2 
    // to the plane of triangle(p1,q1,r1)


    SUB(v1, q1, p1)
        SUB(v2, r1, p1)
        CROSS(N1, v1, v2)

        SUB(v1, p2, r1)
        dp2 = DOT(v1, N1);
    SUB(v1, q2, r1)
        dq2 = DOT(v1, N1);
    SUB(v1, r2, r1)
        dr2 = DOT(v1, N1);

    if (((dp2 * dq2) > 0.0f) && ((dp2 * dr2) > 0.0f)) return 0;

    // Permutation in a canonical form of T1's vertices


    //  printf("d1 = [%f %f %f], d2 = [%f %f %f]\n", dp1, dq1, dr1, dp2, dq2, dr2);
    /*
     // added by Aaron
     if (ZERO_TEST(dp1) || ZERO_TEST(dq1) ||ZERO_TEST(dr1) ||ZERO_TEST(dp2) ||ZERO_TEST(dq2) ||ZERO_TEST(dr2))
     {
     coplanar = 1;
     return 0;
     }
     */


    if (dp1 > 0.0f) {
        if (dq1 > 0.0f) TRI_TRI_INTER_3D(r1, p1, q1, p2, r2, q2, dp2, dr2, dq2)
        else if (dr1 > 0.0f) TRI_TRI_INTER_3D(q1, r1, p1, p2, r2, q2, dp2, dr2, dq2)

        else TRI_TRI_INTER_3D(p1, q1, r1, p2, q2, r2, dp2, dq2, dr2)
    }
    else if (dp1 < 0.0f) {
        if (dq1 < 0.0f) TRI_TRI_INTER_3D(r1, p1, q1, p2, q2, r2, dp2, dq2, dr2)
        else if (dr1 < 0.0f) TRI_TRI_INTER_3D(q1, r1, p1, p2, q2, r2, dp2, dq2, dr2)
        else TRI_TRI_INTER_3D(p1, q1, r1, p2, r2, q2, dp2, dr2, dq2)
    }
    else {
        if (dq1 < 0.0f) {
            if (dr1 >= 0.0f) TRI_TRI_INTER_3D(q1, r1, p1, p2, r2, q2, dp2, dr2, dq2)
            else TRI_TRI_INTER_3D(p1, q1, r1, p2, q2, r2, dp2, dq2, dr2)
        }
        else if (dq1 > 0.0f) {
            if (dr1 > 0.0f) TRI_TRI_INTER_3D(p1, q1, r1, p2, r2, q2, dp2, dr2, dq2)
            else TRI_TRI_INTER_3D(q1, r1, p1, p2, q2, r2, dp2, dq2, dr2)
        }
        else {
            if (dr1 > 0.0f) TRI_TRI_INTER_3D(r1, p1, q1, p2, q2, r2, dp2, dq2, dr2)
            else if (dr1 < 0.0f) TRI_TRI_INTER_3D(r1, p1, q1, p2, r2, q2, dp2, dr2, dq2)
            else {
                // triangles are co-planar

                *coplanar = 1;
                return coplanar_tri_tri3d(p1, q1, r1, p2, q2, r2, N1, N2);
            }
        }
    }
};





/*
 *
 *  Two dimensional Triangle-Triangle Overlap Test
 *
 */


 /* some 2D macros */

#define ORIENT_2D(a, b, c)  ((a[0]-c[0])*(b[1]-c[1])-(a[1]-c[1])*(b[0]-c[0]))


#define INTERSECTION_TEST_VERTEXA(P1, Q1, R1, P2, Q2, R2) {\
if (ORIENT_2D(R2,P2,Q1) >= 0.0f)\
if (ORIENT_2D(R2,Q2,Q1) <= 0.0f)\
if (ORIENT_2D(P1,P2,Q1) > 0.0f) {\
if (ORIENT_2D(P1,Q2,Q1) <= 0.0f) return 1; \
else return 0;} else {\
if (ORIENT_2D(P1,P2,R1) >= 0.0f)\
if (ORIENT_2D(Q1,R1,P2) >= 0.0f) return 1; \
else return 0;\
else return 0;}\
else \
if (ORIENT_2D(P1,Q2,Q1) <= 0.0f)\
if (ORIENT_2D(R2,Q2,R1) <= 0.0f)\
if (ORIENT_2D(Q1,R1,Q2) >= 0.0f) return 1; \
else return 0;\
else return 0;\
else return 0;\
else\
if (ORIENT_2D(R2,P2,R1) >= 0.0f) \
if (ORIENT_2D(Q1,R1,R2) >= 0.0f)\
if (ORIENT_2D(P1,P2,R1) >= 0.0f) return 1;\
else return 0;\
else \
if (ORIENT_2D(Q1,R1,Q2) >= 0.0f) {\
if (ORIENT_2D(R2,R1,Q2) >= 0.0f) return 1; \
else return 0; }\
else return 0; \
else  return 0; \
};

#define INTERSECTION_TEST_VERTEX(P1, Q1, R1, P2, Q2, R2) {\
  if (ORIENT_2D(R2,P2,Q1) >= 0.0f)\
    if (ORIENT_2D(R2,Q2,Q1) <= 0.0f)\
      if (ORIENT_2D(P1,P2,Q1) > 0.0f) {\
        if (ORIENT_2D(P1,Q2,Q1) <= 0.0f) return 1; \
        else return 0;} else {\
        if (ORIENT_2D(P1,P2,R1) >= 0.0f)\
          if (ORIENT_2D(Q1,R1,P2) >= 0.0f) return 1; \
          else return 0;\
        else return 0;}\
    else \
      if (ORIENT_2D(P1,Q2,Q1) <= 0.0f)\
        if (ORIENT_2D(R2,Q2,R1) <= 0.0f)\
          if (ORIENT_2D(Q1,R1,Q2) >= 0.0f) return 1; \
          else return 0;\
        else return 0;\
      else return 0;\
  else\
    if (ORIENT_2D(R2,P2,R1) >= 0.0f) \
      if (ORIENT_2D(Q1,R1,R2) >= 0.0f)\
        if (ORIENT_2D(P1,P2,R1) >= 0.0f) return 1;\
        else return 0;\
      else \
        if (ORIENT_2D(Q1,R1,Q2) >= 0.0f) {\
          if (ORIENT_2D(R2,R1,Q2) >= 0.0f) return 1; \
          else return 0; }\
        else return 0; \
    else  return 0; \
 };


#define INTERSECTION_TEST_EDGE(P1, Q1, R1, P2, Q2, R2) { \
if (ORIENT_2D(R2,P2,Q1) >= 0.0f) {\
if (ORIENT_2D(P1,P2,Q1) >= 0.0f) { \
if (ORIENT_2D(P1,Q1,R2) >= 0.0f) return 1; \
else return 0;} else { \
if (ORIENT_2D(Q1,R1,P2) >= 0.0f){ \
if (ORIENT_2D(R1,P1,P2) >= 0.0f) return 1; else return 0;} \
else return 0; } \
} else {\
if (ORIENT_2D(R2,P2,R1) >= 0.0f) {\
if (ORIENT_2D(P1,P2,R1) >= 0.0f) {\
if (ORIENT_2D(P1,R1,R2) >= 0.0f) return 1;  \
else {\
if (ORIENT_2D(Q1,R1,R2) >= 0.0f) return 1; else return 0;}}\
else  return 0; }\
else return 0; }}



int ccw_tri_tri_intersection_2d(float p1[2], float q1[2], float r1[2],
                                float p2[2], float q2[2], float r2[2]) {
    if (ORIENT_2D(p2, q2, p1) >= 0.0f) {
        if (ORIENT_2D(q2, r2, p1) >= 0.0f) {
            if (ORIENT_2D(r2, p2, p1) >= 0.0f) return 1;
            else INTERSECTION_TEST_EDGE(p1, q1, r1, p2, q2, r2)
        }
        else {
            if (ORIENT_2D(r2, p2, p1) >= 0.0f)
                INTERSECTION_TEST_EDGE(p1, q1, r1, r2, p2, q2)
            else INTERSECTION_TEST_VERTEX(p1, q1, r1, p2, q2, r2)
        }
    }
    else {
        if (ORIENT_2D(q2, r2, p1) >= 0.0f) {
            if (ORIENT_2D(r2, p2, p1) >= 0.0f)
                INTERSECTION_TEST_EDGE(p1, q1, r1, q2, r2, p2)
            else  INTERSECTION_TEST_VERTEX(p1, q1, r1, q2, r2, p2)
        }
        else INTERSECTION_TEST_VERTEX(p1, q1, r1, r2, p2, q2)
    }
};


int tri_tri_overlap_test_2d(float p1[2], float q1[2], float r1[2],
                            float p2[2], float q2[2], float r2[2]) {
    if (ORIENT_2D(p1, q1, r1) < 0.0f)
        if (ORIENT_2D(p2, q2, r2) < 0.0f)
            return ccw_tri_tri_intersection_2d(p1, r1, q1, p2, r2, q2);
        else
            return ccw_tri_tri_intersection_2d(p1, r1, q1, p2, q2, r2);
    else
        if (ORIENT_2D(p2, q2, r2) < 0.0f)
            return ccw_tri_tri_intersection_2d(p1, q1, r1, p2, r2, q2);
        else
            return ccw_tri_tri_intersection_2d(p1, q1, r1, p2, q2, r2);

};
