#pragma once

#include "vulkan/vulkan.hpp"
#include "vk_mem_alloc.hpp"

#include "Geometry/ViewportFrustum.h"
#include "ECS/ECStypes.h"

class RendererBase
{
protected:
#include "common/structs/PrimitiveInstanceParameters.h"
public:
    RendererBase(class Graphics* in_graphics_ptr,
                 vk::Device in_device,
                 vma::Allocator in_vma_allocator)
            : graphics_ptr(in_graphics_ptr),
              device(in_device),
              vma_allocator(in_vma_allocator) {};
    virtual ~RendererBase() = default;

    virtual void DrawFrame(const ViewportFrustum& viewport,
                           std::vector<ModelMatrices>&& matrices,
                           std::vector<LightInfo>&& light_infos,
                           std::vector<DrawInfo>&& draw_infos) {}

    virtual void ToggleViewportFreeze() { viewportFreeze = !viewportFreeze; }
    virtual bool IsFreezed() const {return viewportFreeze;}

protected:
    std::vector<PrimitiveInstanceParameters> CreatePrimitivesInstanceParameters();

protected:
    class Graphics* const graphics_ptr;

    ViewportFrustum         viewport;
    std::vector<ModelMatrices> matrices;
    std::vector<LightInfo>  lightInfos;
    std::vector<DrawInfo>   drawInfos;

    vk::Device device;
    vma::Allocator vma_allocator;

    bool viewportFreeze = false;
};