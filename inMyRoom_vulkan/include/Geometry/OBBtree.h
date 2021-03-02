#pragma once

#include <vector>
#include <memory>

#include "Geometry/OBB.h"

struct OBBtreesIntersectInfo
{
    struct CandidateTriangleRangeCombination
    {
        size_t first_obbtree_offset = 0;
        size_t first_obbtree_count = 0;
        size_t second_obbtree_offset = 0;
        size_t second_obbtree_count = 0;
    };

    const class OBBtree* first_obb_tree = nullptr;
    const class OBBtree* second_obb_tree = nullptr;

    std::vector<CandidateTriangleRangeCombination> candidateTriangleRangeCompinations;
};

class OBBtree
{
private:
    class OBBtreeSplitBuildNode
    {
    public:
        OBBtreeSplitBuildNode(OBBtreeSplitBuildNode* parent_ptr, bool is_right_child, std::vector<Triangle>&& in_triangles);

        OBBtreeSplitBuildNode(const OBBtreeSplitBuildNode& other);
        OBBtreeSplitBuildNode& operator=(const OBBtreeSplitBuildNode& other);

        OBB GetOBB() const;
        bool IsLeaf() const;
        bool IsRightChild() const;

        std::vector<Triangle> GetTriangles() const;
        const std::vector<Triangle>& GetTrianglesRef() const;
        OBBtreeSplitBuildNode* GetParentPtr();
        OBBtreeSplitBuildNode* GetLeftChildPtr();
        OBBtreeSplitBuildNode* GetRightChildPtr();

        size_t NodesAndLeavesCount() const;
        size_t LeavesCount() const;
    private:
        void SplitOBBandCreateChildren();

    public:
        size_t index_in_vector = -1;

    private:
        OBB OBBvolume;
        bool isLeaf;
        bool isRightChild;
        OBBtreeSplitBuildNode* parent_ptr = nullptr;
        std::unique_ptr<OBBtreeSplitBuildNode> leftChild_uptr;
        std::unique_ptr<OBBtreeSplitBuildNode> rightChild_uptr;
        std::vector<Triangle> triangles;

        static const size_t maxNumberOfTriangles = 4;
    };

    class OBBtreeNode
    {
    public:
        static void InitializeTreeBuildNode(OBBtreeSplitBuildNode* obbtree_split_build_node_ptr, std::vector<OBBtreeNode>& obbTreeNodes,
                                            std::vector<TrianglePosition>& triangles_position, std::vector<TriangleNormal>& triangles_normal);

        OBB left_child_obb;
        OBB right_child_obb;
        uint32_t left_child_index_or_triangle_offset = -1;
        uint32_t right_child_index_or_triangle_offset = -1;
        uint16_t left_triangles_count = -1;
        uint16_t right_triangles_count = -1;
    };

public:
    class OBBtreeTraveler
    {
    public:
        OBBtreeTraveler(const std::vector<OBBtreeNode>& obb_tree_nodes) : OBBtreeNodes_ref(obb_tree_nodes) {};
        OBBtreeTraveler(const OBB* root_obb, const std::vector<OBBtreeNode>& obb_tree_nodes, size_t in_triangles_count);

        bool IsLeaf() const;
        OBBtreeTraveler GetLeftChildTraveler() const;
        OBBtreeTraveler GetRightChildTraveler() const;
        OBB GetOBB() const;
        size_t GetTrianglesOffset() const;
        size_t GetTrianglesCount() const;

    private:
        bool isLeaf = false;
        const OBB* obb_ptr = nullptr;

        const OBBtreeNode* obbTreeNode_ptr = nullptr;

        size_t triangles_offset = 0;
        size_t triangles_count = 0;

        const std::vector<OBBtreeNode>& OBBtreeNodes_ref;
    };

public:
    OBBtree() {};
    OBBtree(std::vector<Triangle>&& in_triangles);

    OBB GetRootOBB() const;
    OBBtreeTraveler GetRootTraveler() const;
    TrianglePosition GetTrianglePosition(size_t index) const;
    TriangleNormal GetTriangleNormal(size_t index) const;

    static void IntersectOBBtrees(const OBBtree& first_tree,
                                  const OBBtree& second_tree,
                                  const glm::mat4x4& second_tree_matrix,
                                  OBBtreesIntersectInfo& intesection_info);

private:
    void ConstructDFSrecursive(OBBtreeSplitBuildNode* obbtree_split_build_node_ptr);

    static void IntersectOBBtreesRecursive(const OBBtreeTraveler& first_tree_traveler,
                                           const OBBtreeTraveler& second_tree_traveler,
                                           const glm::mat4x4& second_tree_matrix,
                                           OBBtreesIntersectInfo& intesection_info);

private:
    OBB root_obb;

    std::vector<OBBtreeNode> OBBtreeNodes;
    std::vector<TrianglePosition> triangles_position;
    std::vector<TriangleNormal> triangles_normal;
};