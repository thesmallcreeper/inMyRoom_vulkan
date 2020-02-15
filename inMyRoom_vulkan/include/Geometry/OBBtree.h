#pragma once

#include <vector>
#include <memory>

#include "Geometry/OBB.h"
#include "Geometry/Triangle.h"

class OBBtree
{
public:
    OBBtree();
    OBBtree(std::vector<Triangle>&& in_triangles);

    OBBtree(const OBBtree& other);
    OBBtree& operator=(const OBBtree& other);

public:
    OBB GetOBB() const;

    bool IsLeaf() const;
    std::vector<Triangle> GetTriangles() const;
    const std::vector<Triangle>& GetTrianglesRef() const;
    const OBBtree* GetLeftChildPtr() const;
    const OBBtree* GetRightChildPtr() const;

private:
    void SplitOBBandCreateChildren();

private:
    OBB OBBvolume;
    bool isLeaf;
    std::unique_ptr<OBBtree> leftChild_uptr;
    std::unique_ptr<OBBtree> rightChild_uptr;
    std::vector<Triangle> triangles;

    static const size_t maxNumberOfTriangles = 4;
};