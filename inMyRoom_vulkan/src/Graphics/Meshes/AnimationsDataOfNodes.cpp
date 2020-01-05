#include "Graphics/Meshes/AnimationsDataOfNodes.h"

#include <cassert>

AnimationsDataOfNodes::AnimationsDataOfNodes()
{
}

AnimationsDataOfNodes::~AnimationsDataOfNodes()
{
}

size_t AnimationsDataOfNodes::RegistAnimationsDataAndGetIndex()
{
    size_t return_index = animationsData.size();

    AnimationData this_animationData;
    animationsData.emplace_back(this_animationData);

    return return_index;
}

void AnimationsDataOfNodes::AddAnimationData(size_t index, const AnimationData& in_animationData)
{
    assert(index < animationsData.size());

    if (in_animationData.timeToTranslationKey_map.size())
    {
        assert(animationsData[index].timeToTranslationKey_map.empty());

        for (auto& this_data : in_animationData.timeToTranslationKey_map)
            animationsData[index].timeToTranslationKey_map.emplace(this_data);

        animationsData[index].timeToTranslation_interpolation = in_animationData.timeToTranslation_interpolation;
    }

    if (in_animationData.timeToRotationKey_map.size())
    {
        assert(animationsData[index].timeToRotationKey_map.empty());

        for (auto& this_data : in_animationData.timeToRotationKey_map)
            animationsData[index].timeToRotationKey_map.emplace(this_data);

        animationsData[index].timeToRotation_interpolation = in_animationData.timeToRotation_interpolation;
    }

    if (in_animationData.timeToScaleKey_map.size())
    {
        assert(animationsData[index].timeToScaleKey_map.empty());

        for (auto& this_data : in_animationData.timeToScaleKey_map)
            animationsData[index].timeToScaleKey_map.emplace(this_data);

        animationsData[index].timeToScale_interpolation = in_animationData.timeToScale_interpolation;
    }
}

const AnimationData& AnimationsDataOfNodes::GetAnimationDataRef(size_t index) const
{
    assert(index < animationsData.size());
    return animationsData[index];
}
