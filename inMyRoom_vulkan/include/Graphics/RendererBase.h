#pragma once

#include "vulkan/vulkan.hpp"
#include "vk_mem_alloc.hpp"

#include "Geometry/ViewportFrustum.h"
#include "ECS/ECStypes.h"

class RendererBase
{
public:
    RendererBase(class Graphics* in_graphics_ptr,
                 vk::Device in_device,
                 vma::Allocator in_vma_allocator)
            : graphics_ptr(in_graphics_ptr),
              device(in_device),
              vma_allocator(in_vma_allocator) {};
    virtual ~RendererBase() = default;

    virtual void DrawFrame(ViewportFrustum viewport,
                           std::vector<ModelMatrices>&& matrices,
                           std::vector<DrawInfo>&& draw_infos) {}

    virtual void ToggleViewportFreeze() { viewportFreeze = !viewportFreeze; }
    virtual bool IsFreezed() const {return viewportFreeze;}

protected:
    class Graphics* const graphics_ptr;

    vk::Device device;
    vma::Allocator vma_allocator;

    bool viewportFreeze = false;
};