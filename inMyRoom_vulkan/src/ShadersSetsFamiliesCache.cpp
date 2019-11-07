#include "ShadersSetsFamiliesCache.h"

#include <fstream>
#include <cassert>


ShadersSetsFamiliesCache::ShadersSetsFamiliesCache(Anvil::BaseDevice* const in_device_ptr)
    :
    device_ptr(in_device_ptr)
{
}

void ShadersSetsFamiliesCache::AddShadersSetsFamily(ShadersSetsFamilyInitInfo in_shadersSetsFamilyInitInfos)
{
    ShadersSetsFamilySourceStrings thisShaderSetSourceString;

    if (in_shadersSetsFamilyInitInfos.fragmentShaderSourceFilename.size())
    {
        std::ifstream source_file("shaders/" + in_shadersSetsFamilyInitInfos.fragmentShaderSourceFilename);
        std::string source_string((std::istreambuf_iterator<char>(source_file)), (std::istreambuf_iterator<char>()));
        thisShaderSetSourceString.fragmentShaderSourceString = source_string;
    }

    if (in_shadersSetsFamilyInitInfos.geometryShaderSourceFilename.size())
    {
        std::ifstream source_file("shaders/" + in_shadersSetsFamilyInitInfos.geometryShaderSourceFilename);
        std::string source_string((std::istreambuf_iterator<char>(source_file)), (std::istreambuf_iterator<char>()));
        thisShaderSetSourceString.geometryShaderSourceString = source_string;
    }

    if (in_shadersSetsFamilyInitInfos.tessControlShaderSourceFilename.size())
    {
        std::ifstream source_file("shaders/" + in_shadersSetsFamilyInitInfos.tessControlShaderSourceFilename);
        std::string source_string((std::istreambuf_iterator<char>(source_file)), (std::istreambuf_iterator<char>()));
        thisShaderSetSourceString.tessControlShaderSourceString = source_string;
    }

    if (in_shadersSetsFamilyInitInfos.tessEvaluationShaderSourceFilename.size())
    {
        std::ifstream source_file("shaders/" + in_shadersSetsFamilyInitInfos.tessEvaluationShaderSourceFilename);
        std::string source_string((std::istreambuf_iterator<char>(source_file)), (std::istreambuf_iterator<char>()));
        thisShaderSetSourceString.tessEvaluationShaderSourceString = source_string;
    }

    if (in_shadersSetsFamilyInitInfos.vertexShaderSourceFilename.size())
    {
        std::ifstream source_file("shaders/" + in_shadersSetsFamilyInitInfos.vertexShaderSourceFilename);
        std::string source_string((std::istreambuf_iterator<char>(source_file)), (std::istreambuf_iterator<char>()));
        thisShaderSetSourceString.vertexShaderSourceString = source_string;
    }

    if (in_shadersSetsFamilyInitInfos.computeShaderSourceFilename.size())
    {
        std::ifstream source_file("shaders/" + in_shadersSetsFamilyInitInfos.computeShaderSourceFilename);
        std::string source_string((std::istreambuf_iterator<char>(source_file)), (std::istreambuf_iterator<char>()));
        thisShaderSetSourceString.computeShaderSourceString = source_string;
    }

    shadersSetFamilyNameToShadersSetFamilySourceStrings_umap.emplace(in_shadersSetsFamilyInitInfos.shadersSetFamilyName, thisShaderSetSourceString);
}

ShadersSet ShadersSetsFamiliesCache::GetShadersSet(ShadersSpecs in_shaderSpecs)
{
    auto search = shaderSpecsToShadersSetIndex_umap.find(in_shaderSpecs);
    if (search != shaderSpecsToShadersSetIndex_umap.end())
        return shadersSets[search->second];
    else
    {
        ShadersSet newShaderSet = CreateShadersSet(in_shaderSpecs);

        shadersSets.emplace_back(newShaderSet);
        shaderSpecsToShadersSetIndex_umap.emplace(in_shaderSpecs, shadersSets.size() - 1);

        return newShaderSet;
    }
}

ShadersSet ShadersSetsFamiliesCache::CreateShadersSet(ShadersSpecs in_shaderSpecs)
{
    auto search = shadersSetFamilyNameToShadersSetFamilySourceStrings_umap.find(in_shaderSpecs.shadersSetFamilyName);
    assert(search != shadersSetFamilyNameToShadersSetFamilySourceStrings_umap.end());

    ShadersSetsFamilySourceStrings thisShaderSetFamilySourceString = search->second;

    ShadersSet thisShaderSet;

    if (thisShaderSetFamilySourceString.fragmentShaderSourceString.size())
    {
        auto shader_ptr = Anvil::GLSLShaderToSPIRVGenerator::create(device_ptr,
                                                                    Anvil::GLSLShaderToSPIRVGenerator::MODE_USE_SPECIFIED_SOURCE,
                                                                    thisShaderSetFamilySourceString.fragmentShaderSourceString.c_str(),
                                                                    Anvil::ShaderStage::FRAGMENT);


        for (const auto& thisEmptyDefinition : in_shaderSpecs.emptyDefinition)
            shader_ptr->add_empty_definition(thisEmptyDefinition);

        for (const auto& thisDefinitionValuePair : in_shaderSpecs.definitionValuePairs)
            shader_ptr->add_definition_value_pair(thisDefinitionValuePair.first,
                                                  thisDefinitionValuePair.second);

        auto module_ptr = Anvil::ShaderModule::create_from_spirv_generator(device_ptr,
                                                                           shader_ptr.get());

        module_ptr->set_name("Fragment shader module of " + in_shaderSpecs.shadersSetFamilyName);

        std::unique_ptr<Anvil::ShaderModuleStageEntryPoint> entryPoint_ptr(new Anvil::ShaderModuleStageEntryPoint("main",
                                                                                                                  std::move(module_ptr),
                                                                                                                  Anvil::ShaderStage::FRAGMENT));

        thisShaderSet.fragmentShaderModule_ptr = entryPoint_ptr.get();
        shaderModulesStageEntryPoints_uptrs.emplace_back(std::move(entryPoint_ptr));
    }

    if (thisShaderSetFamilySourceString.geometryShaderSourceString.size())
    {
        auto shader_ptr = Anvil::GLSLShaderToSPIRVGenerator::create(device_ptr,
                                                                    Anvil::GLSLShaderToSPIRVGenerator::MODE_USE_SPECIFIED_SOURCE,
                                                                    thisShaderSetFamilySourceString.geometryShaderSourceString.c_str(),
                                                                    Anvil::ShaderStage::GEOMETRY);


        for (const auto& thisEmptyDefinition : in_shaderSpecs.emptyDefinition)
            shader_ptr->add_empty_definition(thisEmptyDefinition);

        for (const auto& thisDefinitionValuePair : in_shaderSpecs.definitionValuePairs)
            shader_ptr->add_definition_value_pair(thisDefinitionValuePair.first,
                                                  thisDefinitionValuePair.second);

        auto module_ptr = Anvil::ShaderModule::create_from_spirv_generator(device_ptr,
                                                                           shader_ptr.get());

        module_ptr->set_name("Geometry shader module of " + in_shaderSpecs.shadersSetFamilyName);

        std::unique_ptr<Anvil::ShaderModuleStageEntryPoint> entryPoint_ptr(new Anvil::ShaderModuleStageEntryPoint("main",
                                                                                                                  std::move(module_ptr),
                                                                                                                  Anvil::ShaderStage::GEOMETRY));

        thisShaderSet.geometryShaderModule_ptr = entryPoint_ptr.get();
        shaderModulesStageEntryPoints_uptrs.emplace_back(std::move(entryPoint_ptr));
    }

    if (thisShaderSetFamilySourceString.tessControlShaderSourceString.size())
    {
        auto shader_ptr = Anvil::GLSLShaderToSPIRVGenerator::create(device_ptr,
                                                                    Anvil::GLSLShaderToSPIRVGenerator::MODE_USE_SPECIFIED_SOURCE,
                                                                    thisShaderSetFamilySourceString.tessControlShaderSourceString.c_str(),
                                                                    Anvil::ShaderStage::TESSELLATION_CONTROL);


        for (const auto& thisEmptyDefinition : in_shaderSpecs.emptyDefinition)
            shader_ptr->add_empty_definition(thisEmptyDefinition);

        for (const auto& thisDefinitionValuePair : in_shaderSpecs.definitionValuePairs)
            shader_ptr->add_definition_value_pair(thisDefinitionValuePair.first,
                                                  thisDefinitionValuePair.second);

        auto module_ptr = Anvil::ShaderModule::create_from_spirv_generator(device_ptr,
                                                                           shader_ptr.get());

        module_ptr->set_name("Tesselation control shader module of " + in_shaderSpecs.shadersSetFamilyName);

        std::unique_ptr<Anvil::ShaderModuleStageEntryPoint> entryPoint_ptr(new Anvil::ShaderModuleStageEntryPoint("main",
                                                                                                                  std::move(module_ptr),
                                                                                                                  Anvil::ShaderStage::TESSELLATION_CONTROL));

        thisShaderSet.tessControlShaderModule_ptr = entryPoint_ptr.get();
        shaderModulesStageEntryPoints_uptrs.emplace_back(std::move(entryPoint_ptr));
    }

    if (thisShaderSetFamilySourceString.tessEvaluationShaderSourceString.size())
    {
        auto shader_ptr = Anvil::GLSLShaderToSPIRVGenerator::create(device_ptr,
                                                                    Anvil::GLSLShaderToSPIRVGenerator::MODE_USE_SPECIFIED_SOURCE,
                                                                    thisShaderSetFamilySourceString.tessEvaluationShaderSourceString.c_str(),
                                                                    Anvil::ShaderStage::TESSELLATION_EVALUATION);


        for (const auto& thisEmptyDefinition : in_shaderSpecs.emptyDefinition)
            shader_ptr->add_empty_definition(thisEmptyDefinition);

        for (const auto& thisDefinitionValuePair : in_shaderSpecs.definitionValuePairs)
            shader_ptr->add_definition_value_pair(thisDefinitionValuePair.first,
                                                  thisDefinitionValuePair.second);

        auto module_ptr = Anvil::ShaderModule::create_from_spirv_generator(device_ptr,
                                                                           shader_ptr.get());

        module_ptr->set_name("Tesselation evaluation shader module of " + in_shaderSpecs.shadersSetFamilyName);

        std::unique_ptr<Anvil::ShaderModuleStageEntryPoint> entryPoint_ptr(new Anvil::ShaderModuleStageEntryPoint("main",
                                                                                                                  std::move(module_ptr),
                                                                                                                  Anvil::ShaderStage::TESSELLATION_EVALUATION));

        thisShaderSet.tessEvaluationShaderModule_ptr = entryPoint_ptr.get();
        shaderModulesStageEntryPoints_uptrs.emplace_back(std::move(entryPoint_ptr));
    }

    if (thisShaderSetFamilySourceString.vertexShaderSourceString.size())
    {
        auto shader_ptr = Anvil::GLSLShaderToSPIRVGenerator::create(device_ptr,
                                                                    Anvil::GLSLShaderToSPIRVGenerator::MODE_USE_SPECIFIED_SOURCE,
                                                                    thisShaderSetFamilySourceString.vertexShaderSourceString.c_str(),
                                                                    Anvil::ShaderStage::VERTEX);


        for (const auto& thisEmptyDefinition : in_shaderSpecs.emptyDefinition)
            shader_ptr->add_empty_definition(thisEmptyDefinition);

        for (const auto& thisDefinitionValuePair : in_shaderSpecs.definitionValuePairs)
            shader_ptr->add_definition_value_pair(thisDefinitionValuePair.first,
                                                  thisDefinitionValuePair.second);

        auto module_ptr = Anvil::ShaderModule::create_from_spirv_generator(device_ptr,
                                                                           shader_ptr.get());


        module_ptr->set_name("Vertex shader module of " + in_shaderSpecs.shadersSetFamilyName);

        std::unique_ptr<Anvil::ShaderModuleStageEntryPoint> entryPoint_ptr(new Anvil::ShaderModuleStageEntryPoint("main",
                                                                                                                  std::move(module_ptr),
                                                                                                                  Anvil::ShaderStage::VERTEX));

        thisShaderSet.vertexShaderModule_ptr = entryPoint_ptr.get();
        shaderModulesStageEntryPoints_uptrs.emplace_back(std::move(entryPoint_ptr));
    }

    if (thisShaderSetFamilySourceString.computeShaderSourceString.size())
    {
        auto shader_ptr = Anvil::GLSLShaderToSPIRVGenerator::create(device_ptr,
                                                                    Anvil::GLSLShaderToSPIRVGenerator::MODE_USE_SPECIFIED_SOURCE,
                                                                    thisShaderSetFamilySourceString.computeShaderSourceString.c_str(),
                                                                    Anvil::ShaderStage::COMPUTE);


        for (const auto& thisEmptyDefinition : in_shaderSpecs.emptyDefinition)
            shader_ptr->add_empty_definition(thisEmptyDefinition);

        for (const auto& thisDefinitionValuePair : in_shaderSpecs.definitionValuePairs)
            shader_ptr->add_definition_value_pair(thisDefinitionValuePair.first,
                                                  thisDefinitionValuePair.second);

        auto module_ptr = Anvil::ShaderModule::create_from_spirv_generator(device_ptr,
                                                                           shader_ptr.get());


        module_ptr->set_name("Compute shader module of " + in_shaderSpecs.shadersSetFamilyName);

        std::unique_ptr<Anvil::ShaderModuleStageEntryPoint> entryPoint_ptr(new Anvil::ShaderModuleStageEntryPoint("main",
                                                                                                                  std::move(module_ptr),
                                                                                                                  Anvil::ShaderStage::COMPUTE));

        thisShaderSet.computeShaderModule_ptr = entryPoint_ptr.get();
        shaderModulesStageEntryPoints_uptrs.emplace_back(std::move(entryPoint_ptr));
    }

    return thisShaderSet;
}
