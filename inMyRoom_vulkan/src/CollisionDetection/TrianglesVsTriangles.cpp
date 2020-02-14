#include "CollisionDetection/TrianglesVsTriangles.h"

TrianglesVsTriangles::TrianglesVsTriangles()
{
}

std::vector<CSentriesPairCollisionCenter> TrianglesVsTriangles::ExecuteTrianglesVsTriangles(const std::vector<CSentriesPairTrianglesPairs>& CSentriesPairsTrianglesPairs) const
{
    std::vector<CSentriesPairCollisionCenter> return_vector;

    for (const CSentriesPairTrianglesPairs& this_CSentriesPairTrianglesPairs : CSentriesPairsTrianglesPairs)
    {
        glm::vec3 collision_point_weighter_sum = glm::vec3(0.f, 0.f, 0.f);
        float weight = 0.f;

        for (const std::pair<Triangles, Triangles>& this_trianglePair : this_CSentriesPairTrianglesPairs.trianglesPairs)
        {
            for (const Triangle& this_first_triangle_model_space : this_trianglePair.first)
            {
                Triangle this_first_triangle = this_CSentriesPairTrianglesPairs.firstEntry.currentGlobalMatrix * this_first_triangle_model_space;

                for (const Triangle& this_second_triangle_model_space : this_trianglePair.second)
                {
                    Triangle this_second_triangle = this_CSentriesPairTrianglesPairs.secondEntry.currentGlobalMatrix * this_second_triangle_model_space;

                    TrianglesInterseptionInfo this_interseption = Triangle::InterseptTrianglesInfo(this_first_triangle, this_second_triangle);

                    if (this_interseption.doIntersept && !this_interseption.areCoplanar)
                    {
                        float this_weight = glm::length(this_interseption.source - this_interseption.target);
                        collision_point_weighter_sum += this_weight * (this_interseption.source + this_interseption.target) / 2.f;
                        weight += this_weight;
                    }
                }
            }
        }

        if (weight != 0.f)
        {
            CSentriesPairCollisionCenter this_collisionCenter;
            this_collisionCenter.firstEntry = this_CSentriesPairTrianglesPairs.firstEntry;
            this_collisionCenter.secondEntry = this_CSentriesPairTrianglesPairs.secondEntry;
            this_collisionCenter.collisionPoint = collision_point_weighter_sum / weight;

            return_vector.emplace_back(this_collisionCenter);
        }
    }

    return return_vector;
}

