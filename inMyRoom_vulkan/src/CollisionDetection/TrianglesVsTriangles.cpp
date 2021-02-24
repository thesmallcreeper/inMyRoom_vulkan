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

        for (const OBBtreesIntersectInfo::CandidateTriangleRangeCombination& this_triangleRangeCompination : this_CSentriesPairTrianglesPairs.OBBtreesIntersectInfo.candidateTriangleRangeCompinations)
        {
            for (size_t i = this_triangleRangeCompination.first_obbtree_offset; i != this_triangleRangeCompination.first_obbtree_offset + this_triangleRangeCompination.first_obbtree_count; ++i)
            {
                Triangle this_first_triangle = this_CSentriesPairTrianglesPairs.firstEntry.currentGlobalMatrix * this_CSentriesPairTrianglesPairs.OBBtreesIntersectInfo.first_obb_tree->GetTriangle(i);

                for (size_t j = this_triangleRangeCompination.second_obbtree_offset; j != this_triangleRangeCompination.second_obbtree_offset + this_triangleRangeCompination.second_obbtree_count; ++j)
                {
                    Triangle this_second_triangle = this_CSentriesPairTrianglesPairs.secondEntry.currentGlobalMatrix * this_CSentriesPairTrianglesPairs.OBBtreesIntersectInfo.second_obb_tree->GetTriangle(j);

                    TrianglesIntersectionInfo this_interseption = Triangle::IntersectTriangles(this_first_triangle, this_second_triangle);

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

