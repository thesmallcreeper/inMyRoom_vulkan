#include "Geometry/OBBtree.h"

#include <algorithm>
#include <cassert>

OBBtree::OBBtree()
    :isLeaf(true)
{
}

OBBtree::OBBtree(std::vector<Triangle>&& in_triangles)
    :isLeaf(true)
{
    triangles = in_triangles;
    OBBvolume = OBB::CreateOBBfromTriangles(triangles);

    if(triangles.size() > maxNumberOfTriangles)
        SplitOBBandCreateChildren();
}

OBBtree::OBBtree(const OBBtree& other)
{
    *this = other;
}

OBBtree& OBBtree::operator=(const OBBtree& other)
{
    OBBvolume = other.OBBvolume;
    isLeaf = other.isLeaf;
    if (!other.isLeaf)
    {
        leftChild_uptr = std::make_unique<OBBtree>(*other.leftChild_uptr);
        rightChild_uptr = std::make_unique<OBBtree>(*other.rightChild_uptr);
    }
    triangles = other.triangles;

    return *this;
}

void OBBtree::SplitOBBandCreateChildren()
{
    std::vector<std::pair<float, glm::vec4>> halfLength_and_OBB_axis_pairs;
    halfLength_and_OBB_axis_pairs.emplace_back(std::make_pair(OBBvolume.GetHalfLengths().x, OBBvolume.GetSideDirectionU()));
    halfLength_and_OBB_axis_pairs.emplace_back(std::make_pair(OBBvolume.GetHalfLengths().y, OBBvolume.GetSideDirectionV()));
    halfLength_and_OBB_axis_pairs.emplace_back(std::make_pair(OBBvolume.GetHalfLengths().z, OBBvolume.GetSideDirectionW()));

    std::sort(halfLength_and_OBB_axis_pairs.begin(), halfLength_and_OBB_axis_pairs.end(),
              [](std::pair<float, glm::vec4> a, std::pair<float, glm::vec4> b) { return a.first > b.first; });

    std::vector<Triangle> left_child_triangles;
    std::vector<Triangle> right_child_triangles;

    {
        size_t chosen_pair = 0;
        do
        {
            left_child_triangles.clear();
            right_child_triangles.clear();

            glm::vec4 axis = halfLength_and_OBB_axis_pairs[chosen_pair].second;
            float projection_of_center_of_OBB_to_axis = OBBvolume.GetCenterProjectionToAxis(axis);

            for (const Triangle& this_triangle : triangles)
            {
                std::pair<float, float> this_triangle_min_max_projection = this_triangle.GetMinMaxProjectionToAxis(axis);

                float projection_mean = (this_triangle_min_max_projection.first + this_triangle_min_max_projection.second) / 2.f;
                if (projection_mean <= projection_of_center_of_OBB_to_axis)
                    left_child_triangles.emplace_back(this_triangle);
                else
                    right_child_triangles.emplace_back(this_triangle);
            }

            chosen_pair++;
        } while ((left_child_triangles.empty() || right_child_triangles.empty()) && chosen_pair < halfLength_and_OBB_axis_pairs.size());
    }

    if (left_child_triangles.empty() || right_child_triangles.empty())
    {
        left_child_triangles.clear();
        right_child_triangles.clear();

        for (size_t i = 0; i < triangles.size(); i++)
        {
            if (i < triangles.size() / 2)
                left_child_triangles.emplace_back(triangles[i]);
            else
                right_child_triangles.emplace_back(triangles[i]);
        }
    }

    triangles.clear();

    isLeaf = false;
    leftChild_uptr = std::make_unique<OBBtree>(std::move(left_child_triangles));
    rightChild_uptr = std::make_unique<OBBtree>(std::move(right_child_triangles));
}


OBB OBBtree::GetOBB() const
{
    return OBBvolume;
}

bool OBBtree::IsLeaf() const
{
    return isLeaf;
}

std::vector<Triangle> OBBtree::GetTriangles() const
{
    return triangles;
}

const OBBtree* OBBtree::GetLeftChildPtr() const
{
    return leftChild_uptr.get();
}

const OBBtree* OBBtree::GetRightChildPtr() const
{
    return rightChild_uptr.get();
}
