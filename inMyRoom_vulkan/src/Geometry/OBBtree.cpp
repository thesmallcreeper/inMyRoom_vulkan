#include "Geometry/OBBtree.h"

#include <algorithm>
#include <iterator>
#include <cassert>


OBBtree::OBBtreeSplitBuildNode::OBBtreeSplitBuildNode(OBBtreeSplitBuildNode* parent_ptr, bool is_right_child, std::vector<Triangle>&& in_triangles)
    :isLeaf(true),
     isRightChild(is_right_child),
     parent_ptr(parent_ptr)
{
    triangles = in_triangles;
    OBBvolume = OBB::CreateOBBfromTriangles(triangles);

    if(triangles.size() > maxNumberOfTriangles)
        SplitOBBandCreateChildren();
}

OBBtree::OBBtreeSplitBuildNode::OBBtreeSplitBuildNode(const OBBtreeSplitBuildNode& other)
{
    *this = other;
}

OBBtree::OBBtreeSplitBuildNode& OBBtree::OBBtreeSplitBuildNode::operator=(const OBBtreeSplitBuildNode& other)
{
    isRightChild = other.isRightChild;
    index_in_vector = other.index_in_vector;

    OBBvolume = other.OBBvolume;
    isLeaf = other.isLeaf;
    parent_ptr = other.parent_ptr;
    if (!other.isLeaf)
    {
        leftChild_uptr = std::make_unique<OBBtree::OBBtreeSplitBuildNode>(*other.leftChild_uptr);
        rightChild_uptr = std::make_unique<OBBtree::OBBtreeSplitBuildNode>(*other.rightChild_uptr);
    }
    triangles = other.triangles;

    return *this;
}

void OBBtree::OBBtreeSplitBuildNode::SplitOBBandCreateChildren()
{
    std::vector<std::pair<float, glm::vec3>> halfLength_and_OBB_axis_pairs;
    halfLength_and_OBB_axis_pairs.emplace_back(std::make_pair(glm::length(OBBvolume.GetSideDirectionU()), glm::normalize(OBBvolume.GetSideDirectionU())));
    halfLength_and_OBB_axis_pairs.emplace_back(std::make_pair(glm::length(OBBvolume.GetSideDirectionV()), glm::normalize(OBBvolume.GetSideDirectionV())));
    halfLength_and_OBB_axis_pairs.emplace_back(std::make_pair(glm::length(OBBvolume.GetSideDirectionW()), glm::normalize(OBBvolume.GetSideDirectionW())));

    std::sort(halfLength_and_OBB_axis_pairs.begin(), halfLength_and_OBB_axis_pairs.end(),
              [](std::pair<float, glm::vec3> a, std::pair<float, glm::vec3> b) { return a.first > b.first; });

    std::vector<Triangle> left_child_triangles;
    std::vector<Triangle> right_child_triangles;

    {
        size_t chosen_pair = 0;
        do
        {
            left_child_triangles.clear();
            right_child_triangles.clear();

            glm::vec3 axis = halfLength_and_OBB_axis_pairs[chosen_pair].second;
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
    leftChild_uptr = std::make_unique<OBBtree::OBBtreeSplitBuildNode>(this, false, std::move(left_child_triangles));
    rightChild_uptr = std::make_unique<OBBtree::OBBtreeSplitBuildNode>(this, true, std::move(right_child_triangles));

    // left child should the highest ray-hit chance
    if (rightChild_uptr->GetOBB().GetSurface() > leftChild_uptr->GetOBB().GetSurface())
    {
        std::swap(rightChild_uptr, leftChild_uptr);
        rightChild_uptr->isRightChild = true;
        leftChild_uptr->isRightChild = false;
    }
}


OBB OBBtree::OBBtreeSplitBuildNode::GetOBB() const
{
    return OBBvolume;
}

bool OBBtree::OBBtreeSplitBuildNode::IsLeaf() const
{
    return isLeaf;
}

bool OBBtree::OBBtreeSplitBuildNode::IsRightChild() const
{
    return isRightChild;
}

std::vector<Triangle> OBBtree::OBBtreeSplitBuildNode::GetTriangles() const
{
    return triangles;
}

const std::vector<Triangle>& OBBtree::OBBtreeSplitBuildNode::GetTrianglesRef() const
{
    return triangles;
}

OBBtree::OBBtreeSplitBuildNode* OBBtree::OBBtreeSplitBuildNode::GetParentPtr()
{
    return parent_ptr;
}

OBBtree::OBBtreeSplitBuildNode* OBBtree::OBBtreeSplitBuildNode::GetLeftChildPtr()
{
    return leftChild_uptr.get();
}

OBBtree::OBBtreeSplitBuildNode* OBBtree::OBBtreeSplitBuildNode::GetRightChildPtr()
{
    return rightChild_uptr.get();
}

size_t OBBtree::OBBtreeSplitBuildNode::NodesAndLeavesCount() const
{
    if(isLeaf)
    {
        return 1;
    }
    else
    {
        size_t left_child_result = leftChild_uptr->NodesAndLeavesCount();
        size_t right_child_result = rightChild_uptr->NodesAndLeavesCount();
        return left_child_result + right_child_result + 1;
    }
}

size_t OBBtree::OBBtreeSplitBuildNode::LeavesCount() const
{
    if(isLeaf)
    {
        return 1;
    }
    else
    {
        size_t left_child_result = leftChild_uptr->LeavesCount();
        size_t right_child_result = rightChild_uptr->LeavesCount();
        return left_child_result + right_child_result;
    }
}

void OBBtree::OBBtreeNode::InitializeTreeBuildNode(OBBtreeSplitBuildNode* obbtree_split_build_node_ptr, std::vector<OBBtreeNode>& obbTreeNodes,
                                                   std::vector<TrianglePosition>& triangles_position,
                                                   std::vector<TriangleNormal>& triangles_normal,
                                                   std::vector<TriangleIndices>& triangles_indices)
{
    if(not obbtree_split_build_node_ptr->IsLeaf())
    {
        size_t obbNode_index = obbTreeNodes.size();
        obbtree_split_build_node_ptr->index_in_vector = obbNode_index;
        obbTreeNodes.emplace_back();

        size_t parent_index = obbtree_split_build_node_ptr->GetParentPtr()->index_in_vector;

        if(not obbtree_split_build_node_ptr->IsRightChild())
        {
            obbTreeNodes[parent_index].left_child_index_or_triangle_offset = uint32_t(obbNode_index);
            obbTreeNodes[parent_index].left_triangles_count = 0;
            obbTreeNodes[parent_index].left_child_obb = obbtree_split_build_node_ptr->GetOBB();
        }
        else
        {
            obbTreeNodes[parent_index].right_child_index_or_triangle_offset = uint32_t(obbNode_index);
            obbTreeNodes[parent_index].right_triangles_count = 0;
            obbTreeNodes[parent_index].right_child_obb = obbtree_split_build_node_ptr->GetOBB();
        }
    }
    else
    {
        uint32_t triangles_offset = uint32_t(triangles_position.size());
        uint16_t this_node_triangles_count = uint16_t(obbtree_split_build_node_ptr->GetTrianglesRef().size());
        for(const Triangle& this_triangle: obbtree_split_build_node_ptr->GetTrianglesRef())
        {
            triangles_position.emplace_back(this_triangle.GetTrianglePosition());
            triangles_normal.emplace_back(this_triangle.GetTriangleNormal());
            triangles_indices.emplace_back(this_triangle.GetTriangleIndices());
        }

        size_t parent_index = obbtree_split_build_node_ptr->GetParentPtr()->index_in_vector;

        if(not obbtree_split_build_node_ptr->IsRightChild())
        {
            assert(obbTreeNodes[parent_index].left_child_index_or_triangle_offset == uint32_t(-1) && obbTreeNodes[parent_index].left_triangles_count == uint16_t(-1));
            obbTreeNodes[parent_index].left_child_index_or_triangle_offset = triangles_offset;
            obbTreeNodes[parent_index].left_triangles_count = this_node_triangles_count;
            obbTreeNodes[parent_index].left_child_obb = obbtree_split_build_node_ptr->GetOBB();
        }
        else
        {
            assert(obbTreeNodes[parent_index].right_child_index_or_triangle_offset == uint32_t(-1) && obbTreeNodes[parent_index].right_triangles_count == uint16_t(-1));
            obbTreeNodes[parent_index].right_child_index_or_triangle_offset = triangles_offset;
            obbTreeNodes[parent_index].right_triangles_count = this_node_triangles_count;
            obbTreeNodes[parent_index].right_child_obb = obbtree_split_build_node_ptr->GetOBB();
        }
    }
}

OBBtree::OBBtreeTraveler::OBBtreeTraveler(const OBB* root_obb, const std::vector<OBBtree::OBBtreeNode>& obb_tree_nodes, size_t in_triangles_count)
    : OBBtreeNodes_ref(obb_tree_nodes)
{
    obb_ptr = root_obb;

    if(obb_tree_nodes.size())
    {
        isLeaf = false;

        obbTreeNode_ptr = &obb_tree_nodes[0];
    }
    else
    {
        isLeaf = true;
        
        triangles_offset = 0;
        triangles_count = in_triangles_count;
    }
}

bool OBBtree::OBBtreeTraveler::IsLeaf() const
{
    return isLeaf;
}

OBBtree::OBBtreeTraveler OBBtree::OBBtreeTraveler::GetLeftChildTraveler() const
{
    assert(IsLeaf() == false);

    OBBtreeTraveler return_traveler{OBBtreeNodes_ref};
    return_traveler.obb_ptr = &obbTreeNode_ptr->left_child_obb;

    if(obbTreeNode_ptr->left_triangles_count == 0) [[likely]]
    {
        return_traveler.isLeaf = false;
        return_traveler.obbTreeNode_ptr = &OBBtreeNodes_ref[size_t(obbTreeNode_ptr->left_child_index_or_triangle_offset)];
    }
    else
    {
        return_traveler.isLeaf = true;
        return_traveler.triangles_offset = size_t(obbTreeNode_ptr->left_child_index_or_triangle_offset);
        return_traveler.triangles_count = size_t(obbTreeNode_ptr->left_triangles_count);
    }

    return return_traveler;
}

OBBtree::OBBtreeTraveler OBBtree::OBBtreeTraveler::GetRightChildTraveler() const
{
    assert(IsLeaf() == false);

    OBBtreeTraveler return_traveler{OBBtreeNodes_ref};
    return_traveler.obb_ptr = &obbTreeNode_ptr->right_child_obb;

    if(obbTreeNode_ptr->right_triangles_count == 0) [[likely]]
    {
        return_traveler.isLeaf = false;
        return_traveler.obbTreeNode_ptr = &OBBtreeNodes_ref[size_t(obbTreeNode_ptr->right_child_index_or_triangle_offset)];
    }
    else
    {
        return_traveler.isLeaf = true;
        return_traveler.triangles_offset = size_t(obbTreeNode_ptr->right_child_index_or_triangle_offset);
        return_traveler.triangles_count = size_t(obbTreeNode_ptr->right_triangles_count);
    }

    return return_traveler;
}

OBB OBBtree::OBBtreeTraveler::GetOBB() const
{
    return *obb_ptr;
}

size_t OBBtree::OBBtreeTraveler::GetTrianglesOffset() const
{
    assert(IsLeaf() == true);
    return triangles_offset;
}

size_t OBBtree::OBBtreeTraveler::GetTrianglesCount() const
{
    assert(IsLeaf() == true);
    return triangles_count;
}

OBBtree::OBBtree(std::vector<Triangle>&& in_triangles)
{
    size_t triangles_count = in_triangles.size();

    std::unique_ptr<OBBtree::OBBtreeSplitBuildNode> obb_split_build_node_root = std::make_unique<OBBtree::OBBtreeSplitBuildNode>(nullptr, false, std::move(in_triangles));
    root_obb = obb_split_build_node_root->GetOBB();

    size_t nodes_and_leaves_count = obb_split_build_node_root->NodesAndLeavesCount();
    size_t leaves_count = obb_split_build_node_root->LeavesCount();

    size_t inner_nodes = nodes_and_leaves_count - leaves_count;

    if(not obb_split_build_node_root->IsLeaf())
    {
        OBBtreeNodes.reserve(inner_nodes);
        triangles_position.reserve(triangles_count);
        triangles_normal.reserve(triangles_count);
        triangles_indices.reserve(triangles_count);

        OBBtreeNodes.emplace_back();
        obb_split_build_node_root->index_in_vector = 0;

        ConstructDFSrecursive(obb_split_build_node_root->GetLeftChildPtr());
        ConstructDFSrecursive(obb_split_build_node_root->GetRightChildPtr());
    }
    else
    {
        assert(inner_nodes == 0);

        for(const Triangle& this_triangle: in_triangles)
        {
            triangles_position.emplace_back(this_triangle.GetTrianglePosition());
            triangles_normal.emplace_back(this_triangle.GetTriangleNormal());
            triangles_indices.emplace_back(this_triangle.GetTriangleIndices());
        }
    }

}

OBB OBBtree::GetRootOBB() const
{
    return root_obb;
}

OBBtree::OBBtreeTraveler OBBtree::GetRootTraveler() const
{
    return OBBtreeTraveler(&root_obb, OBBtreeNodes, triangles_position.size());
}

TrianglePosition OBBtree::GetTrianglePosition(size_t index) const
{
    return triangles_position[index];
}

TriangleNormal OBBtree::GetTriangleNormal(size_t index) const
{
    return triangles_normal[index];
}

TriangleIndices OBBtree::GetTriangleIndices(size_t index) const
{
    return triangles_indices[index];
}

void OBBtree::ConstructDFSrecursive(OBBtree::OBBtreeSplitBuildNode * obbtree_split_build_node_ptr)
{
    OBBtreeNode::InitializeTreeBuildNode(obbtree_split_build_node_ptr, OBBtreeNodes, triangles_position, triangles_normal, triangles_indices);

    if(not obbtree_split_build_node_ptr->IsLeaf())
    {
        ConstructDFSrecursive(obbtree_split_build_node_ptr->GetLeftChildPtr());
        ConstructDFSrecursive(obbtree_split_build_node_ptr->GetRightChildPtr());
    }
}

void OBBtree::IntersectOBBtrees(const OBBtree& first_tree,
                                const OBBtree& second_tree,
                                const glm::mat4x4& second_tree_matrix,
                                OBBtreesIntersectInfo& intesection_info)
{
    intesection_info.candidateTriangleRangeCombinations.clear();

    OBBtreeTraveler first_tree_traveler = first_tree.GetRootTraveler();
    OBBtreeTraveler second_tree_traveler = second_tree.GetRootTraveler();

    IntersectOBBtreesRecursive(first_tree_traveler,
                               second_tree_traveler,
                               second_tree_matrix,
                               intesection_info);
}


// TODO likely and reorder
void OBBtree::IntersectOBBtreesRecursive(const OBBtreeTraveler& first_tree_traveler,
                                         const OBBtreeTraveler& second_tree_traveler,
                                         const glm::mat4x4& second_tree_matrix,
                                         OBBtreesIntersectInfo& intesection_info)
{
    Paralgram first_paralgram = first_tree_traveler.GetOBB();
    Paralgram second_paralgram = second_tree_matrix * second_tree_traveler.GetOBB();

    if (Paralgram::IntersectParalgramsBoolean(first_paralgram, second_paralgram))
    {
        if (not first_tree_traveler.IsLeaf() && not second_tree_traveler.IsLeaf()) [[likely]]
        {
            if (first_paralgram.GetSurface() >= second_paralgram.GetSurface())
            {
                IntersectOBBtreesRecursive(first_tree_traveler.GetLeftChildTraveler(),
                                           second_tree_traveler,
                                           second_tree_matrix,
                                           intesection_info);
                IntersectOBBtreesRecursive(first_tree_traveler.GetRightChildTraveler(),
                                           second_tree_traveler,
                                           second_tree_matrix,
                                           intesection_info);
            }
            else
            {
                IntersectOBBtreesRecursive(first_tree_traveler,
                                           second_tree_traveler.GetLeftChildTraveler(),
                                           second_tree_matrix,
                                           intesection_info);
                IntersectOBBtreesRecursive(first_tree_traveler,
                                           second_tree_traveler.GetRightChildTraveler(),
                                           second_tree_matrix,
                                           intesection_info);
            }
        }
        else if (first_tree_traveler.IsLeaf() && not second_tree_traveler.IsLeaf())
        {
            IntersectOBBtreesRecursive(first_tree_traveler,
                                       second_tree_traveler.GetLeftChildTraveler(),
                                       second_tree_matrix,
                                       intesection_info);
            IntersectOBBtreesRecursive(first_tree_traveler,
                                       second_tree_traveler.GetRightChildTraveler(),
                                       second_tree_matrix,
                                       intesection_info);
        }
        else if (not first_tree_traveler.IsLeaf() && second_tree_traveler.IsLeaf())
        {
            IntersectOBBtreesRecursive(first_tree_traveler.GetLeftChildTraveler(),
                                       second_tree_traveler,
                                       second_tree_matrix,
                                       intesection_info);
            IntersectOBBtreesRecursive(first_tree_traveler.GetRightChildTraveler(),
                                       second_tree_traveler,
                                       second_tree_matrix,
                                       intesection_info);
        }
        else
        {
            intesection_info.candidateTriangleRangeCombinations.emplace_back(first_tree_traveler.GetTrianglesOffset(), first_tree_traveler.GetTrianglesCount(),
                                                                             second_tree_traveler.GetTrianglesOffset(), second_tree_traveler.GetTrianglesCount());
        }
    }
}