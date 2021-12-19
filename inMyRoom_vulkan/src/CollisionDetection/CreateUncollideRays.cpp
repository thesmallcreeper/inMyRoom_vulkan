#include "CollisionDetection/CreateUncollideRays.h"
#include "Geometry/Plane.h"

#include <bitset>
#include <numeric>
#include <unordered_map>
#include <unordered_set>

CreateUncollideRays::CreateUncollideRays()
{
}

struct TriangleCandidateRays
{
    std::bitset<3> point_is_not_outside = -1;
    glm::vec3 weighted_avg_of_points = glm::vec3(0.f);
    float weight = 0.f;

    bool IsNull() const
    {
        return weight == 0.f;
    }

    bool IsPointNotOutside(size_t index) const
    {
        return point_is_not_outside[index];
    }

    bool ShouldFallbackToAvgPoint() const
    {
        return point_is_not_outside == std::bitset<3>(0);
    }

    glm::vec3 GetWeightedAvg() const
    {
        return weighted_avg_of_points / weight;
    }

    static void MergeWithMap(std::unordered_map<uint32_t, TriangleCandidateRays>& un_map,
                             uint32_t index,
                             const TriangleCandidateRays& triangle_candidate_ray)
    {
        const auto& emplace_result = un_map.emplace(index, triangle_candidate_ray);

        if(emplace_result.second == false)
        {
            emplace_result.first->second.Merge(triangle_candidate_ray);
        }
    }

private:
    void Merge(const TriangleCandidateRays& rhs)
    {
        point_is_not_outside &= rhs.point_is_not_outside;
        weighted_avg_of_points += rhs.weighted_avg_of_points;
        weight += rhs.weight;
    }
};

CDentriesUncollideRays CreateUncollideRays::ExecuteCreateUncollideRays(const CDentriesPairTrianglesPairs& entriesPairTrianglesPairs) const
{
    const glm::mat4 second_to_first_space_matrix = glm::inverse(entriesPairTrianglesPairs.firstEntry.currentGlobalMatrix) *
                                                   entriesPairTrianglesPairs.secondEntry.currentGlobalMatrix;

    const glm::mat3 second_to_first_space_normal_matrix = Triangle::GetNormalCorrectedMatrixUnormalized(second_to_first_space_matrix);

    const OBBtree* first_OBBtree_ptr = entriesPairTrianglesPairs.firstEntry.OBBtree_ptr;
    const OBBtree* second_OBBtree_ptr = entriesPairTrianglesPairs.secondEntry.OBBtree_ptr;

    std::unordered_map<uint32_t, TriangleCandidateRays> first_candidateTriangleRays;
    std::unordered_map<uint32_t, TriangleCandidateRays> second_candidateTriangleRays;

    // Find intersect triangles and flag points that are not outside from the other object
    for(const auto& this_trianglesPair : entriesPairTrianglesPairs.OBBtreesIntersectInfoObj.candidateTriangleRangeCombinations)
    {
        std::array<TriangleCandidateRays, OBBtree::GetMaxNumberOfTrianglesPerNode()> this_first_candidateRays;
        std::array<TriangleCandidateRays, OBBtree::GetMaxNumberOfTrianglesPerNode()> this_second_candidateRays;

        for(size_t i = 0; i != this_trianglesPair.first_obbtree_count; ++i)
        {
            for(size_t j = 0; j != this_trianglesPair.second_obbtree_count; ++j)
            {
                TrianglePosition firsts_triangle = first_OBBtree_ptr->GetTrianglePosition(i + this_trianglesPair.first_obbtree_offset);
                TrianglePosition seconds_triangle = second_to_first_space_matrix * second_OBBtree_ptr->GetTrianglePosition(j + this_trianglesPair.second_obbtree_offset);

                TrianglesIntersectionInfo this_intersect = Triangle::IntersectTriangles(firsts_triangle, seconds_triangle);

                if(this_intersect.doIntersept && not this_intersect.areCoplanar)
                {
                    Plane firsts_triangle_plane = Plane::CreatePlaneFromTriangle(firsts_triangle);
                    Plane seconds_triangle_plane = Plane::CreatePlaneFromTriangle(seconds_triangle);

                    float weight = glm::length(this_intersect.source - this_intersect.target);
                    glm::vec3 average_point_of_intersect = weight * (this_intersect.source + this_intersect.target) / 2.f;

                    this_first_candidateRays[i].weighted_avg_of_points += average_point_of_intersect;
                    this_first_candidateRays[i].weight += weight;

                    this_second_candidateRays[j].weighted_avg_of_points += average_point_of_intersect;
                    this_second_candidateRays[j].weight += weight;

                    for(size_t point_index = 0; point_index != 3; ++point_index)
                    {
                        if(seconds_triangle_plane.IntersectPoint(firsts_triangle.GetP(point_index)) == PlaneIntersectResult::OUTSIDE)
                        {
                            this_first_candidateRays[i].point_is_not_outside.set(point_index, false);
                        }
                        if(firsts_triangle_plane.IntersectPoint(seconds_triangle.GetP(point_index)) == PlaneIntersectResult::OUTSIDE)
                        {
                            this_second_candidateRays[j].point_is_not_outside.set(point_index, false);
                        }
                    }
                }
            }
        }

        for (size_t i = 0; i != this_trianglesPair.first_obbtree_count; ++i)
        {
            if(not this_first_candidateRays[i].IsNull())
                TriangleCandidateRays::MergeWithMap(first_candidateTriangleRays, uint32_t(i + this_trianglesPair.first_obbtree_offset), this_first_candidateRays[i]);
        }

        for (size_t j = 0; j != this_trianglesPair.second_obbtree_count; ++j)
        {
            if (not this_second_candidateRays[j].IsNull())
                TriangleCandidateRays::MergeWithMap(second_candidateTriangleRays, uint32_t(j + this_trianglesPair.second_obbtree_offset), this_second_candidateRays[j]);
        }
    }

    // Create ray once for each point of triangle that is not outside. If no point is not outside then fallback to average point
    auto find_rays_lambda = [&](const std::unordered_map<uint32_t, TriangleCandidateRays>& candidateTriangleRays,
                                const OBBtree* OBBtree_ptr,
                                const bool should_multiply_with_matrix)
    {
        std::vector<Ray> return_rays;
        std::unordered_set<uint32_t> emplaced_point_indices;

        for(const auto& this_index_candidate_rays_pair: candidateTriangleRays)
        {
            TrianglePosition this_triangle_position = should_multiply_with_matrix ? second_to_first_space_matrix * OBBtree_ptr->GetTrianglePosition(this_index_candidate_rays_pair.first)
                                                                                  : OBBtree_ptr->GetTrianglePosition(this_index_candidate_rays_pair.first);
            TriangleNormal this_triangle_normal_localspace = OBBtree_ptr->GetTriangleNormal(this_index_candidate_rays_pair.first);

            if(not this_index_candidate_rays_pair.second.ShouldFallbackToAvgPoint())
            {
                TriangleIndices this_triangle_indices = OBBtree_ptr->GetTriangleIndices(this_index_candidate_rays_pair.first);
                
                for(size_t point_index = 0; point_index != 3; ++point_index)
                {
                    if(this_index_candidate_rays_pair.second.IsPointNotOutside(point_index))
                    {
                        const auto& emplace_result = emplaced_point_indices.emplace(this_triangle_indices.GetI(point_index));

                        if (emplace_result.second == true)
                        {
                            glm::vec3 pos = this_triangle_position.GetP(point_index);
                            glm::vec3 normal = should_multiply_with_matrix ? this_triangle_normal_localspace.GetNormal(point_index, second_to_first_space_normal_matrix)
                                                                           : this_triangle_normal_localspace.GetNormal(point_index);
                            return_rays.emplace_back(pos, -normal);
                        }
                    }
                }
            }
            else
            {
                glm::vec3 pos_triangle_coord = this_index_candidate_rays_pair.second.GetWeightedAvg();
                glm::vec2 barycentric = this_triangle_position.GetBarycentricOfPoint(pos_triangle_coord);

                glm::vec3 pos = pos_triangle_coord;
                glm::vec3 normal = should_multiply_with_matrix ? this_triangle_normal_localspace.GetNormal(barycentric, second_to_first_space_normal_matrix)
                                                               : this_triangle_normal_localspace.GetNormal(barycentric);

                return_rays.emplace_back(pos, -normal);
            }
        }

        return return_rays;
    };

    CDentriesUncollideRays return_uncollideRays;
    return_uncollideRays.firstEntry = entriesPairTrianglesPairs.firstEntry;
    return_uncollideRays.secondEntry = entriesPairTrianglesPairs.secondEntry;

    return_uncollideRays.rays_from_first_to_second = find_rays_lambda(first_candidateTriangleRays, first_OBBtree_ptr, false);
    return_uncollideRays.average_point_first_modelspace = std::accumulate(return_uncollideRays.rays_from_first_to_second.begin(),
                                                                          return_uncollideRays.rays_from_first_to_second.end(),
                                                                          glm::vec3(0.f),
                                                                          [](const glm::vec3& sum_so_far, const Ray& ray) {return sum_so_far + ray.GetOrigin();});
    return_uncollideRays.average_point_first_modelspace /= float(return_uncollideRays.rays_from_first_to_second.size());

    return_uncollideRays.rays_from_second_to_first = find_rays_lambda(second_candidateTriangleRays, second_OBBtree_ptr, true);
    return_uncollideRays.average_point_second_modelspace = std::accumulate(return_uncollideRays.rays_from_second_to_first.begin(),
                                                                           return_uncollideRays.rays_from_second_to_first.end(),
                                                                           glm::vec3(0.f),
                                                                           [](const glm::vec3& sum_so_far, const Ray& ray) {return sum_so_far + ray.GetOrigin(); });
    return_uncollideRays.average_point_second_modelspace /= float(return_uncollideRays.rays_from_second_to_first.size());
    return_uncollideRays.average_point_second_modelspace = glm::vec3(glm::inverse(second_to_first_space_matrix) *
                                                                     glm::vec4(return_uncollideRays.average_point_second_modelspace, 1.f));

    return return_uncollideRays;
}
