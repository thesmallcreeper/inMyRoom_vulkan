#include "PrimitivesShaders.h"

#include <fstream>
#include <cassert>

PrimitivesShaders::PrimitivesShaders(std::vector<ShaderSetFamilyInitInfo> in_shadersSetFamilyInitInfos)
{
    for (auto thisInitInfo : in_shadersSetFamilyInitInfos)
    {
        ShaderSetFamilySourceStrings thisShaderSetSourceString;

        if (thisInitInfo.fragmentShaderSourceFilename.size())
        {
            std::ifstream source_file("shaders/" + thisInitInfo.fragmentShaderSourceFilename);
            std::string source_string((std::istreambuf_iterator<char>(source_file)),
                (std::istreambuf_iterator<char>()));
            thisShaderSetSourceString.fragmentShaderSourceString = source_string;
        }

        if (thisInitInfo.geometryShaderSourceFilename.size())
        {
            std::ifstream source_file("shaders/" + thisInitInfo.geometryShaderSourceFilename);
            std::string source_string((std::istreambuf_iterator<char>(source_file)),
                (std::istreambuf_iterator<char>()));
            thisShaderSetSourceString.geometryShaderSourceString = source_string;
        }

        if (thisInitInfo.tessControlShaderSourceFilename.size())
        {
            std::ifstream source_file("shaders/" + thisInitInfo.tessControlShaderSourceFilename);
            std::string source_string((std::istreambuf_iterator<char>(source_file)),
                (std::istreambuf_iterator<char>()));
            thisShaderSetSourceString.tessControlShaderSourceString = source_string;
        }

        if (thisInitInfo.tessEvaluationShaderSourceFilename.size())
        {
            std::ifstream source_file("shaders/" + thisInitInfo.tessEvaluationShaderSourceFilename);
            std::string source_string((std::istreambuf_iterator<char>(source_file)),
                (std::istreambuf_iterator<char>()));
            thisShaderSetSourceString.tessEvaluationShaderSourceString = source_string;
        }

        if (thisInitInfo.vertexShaderSourceFilename.size())
        {
            std::ifstream source_file("shaders/" + thisInitInfo.vertexShaderSourceFilename);
            std::string source_string((std::istreambuf_iterator<char>(source_file)),
                (std::istreambuf_iterator<char>()));
            thisShaderSetSourceString.vertexShaderSourceString = source_string;
        }

        shadersSetFamilyNameToShadersSetFamilySourceStrings_umap.emplace(thisInitInfo.shadersSetFamilyName, thisShaderSetSourceString);
    }
}

PrimitivesShaders::~PrimitivesShaders()
{
    shadersSets.clear();
    shaderModulesStageEntryPoints_ptrs.clear();

}

size_t PrimitivesShaders::getShaderSetIndex(ShadersSpecs in_shaderSpecs, Anvil::BaseDevice* in_device_ptr)
{
    auto search = shaderSpecsToShaderSetIndex_umap.find(in_shaderSpecs);
    if (search != shaderSpecsToShaderSetIndex_umap.end())
        return search->second;
    else
    {
        ShadersSet newShaderSet = createShadersSet(in_shaderSpecs, in_device_ptr);
        shadersSets.emplace_back(newShaderSet);
        shaderSpecsToShaderSetIndex_umap.emplace(in_shaderSpecs, shadersSets.size() - 1);
        return shadersSets.size() - 1;
    }
}

ShadersSet PrimitivesShaders::createShadersSet(ShadersSpecs in_shaderSpecs, Anvil::BaseDevice* in_device_ptr)
{
    auto search = shadersSetFamilyNameToShadersSetFamilySourceStrings_umap.find(in_shaderSpecs.shadersSetFamilyName);
    assert(search != shadersSetFamilyNameToShadersSetFamilySourceStrings_umap.end());

    ShaderSetFamilySourceStrings thisShaderSetFamilySourceString = search->second;

    ShadersSet thisShaderSet;

    if (thisShaderSetFamilySourceString.fragmentShaderSourceString.size())
    {
        auto shader_ptr = Anvil::GLSLShaderToSPIRVGenerator::create(in_device_ptr,
                                                                    Anvil::GLSLShaderToSPIRVGenerator::MODE_USE_SPECIFIED_SOURCE,
                                                                    thisShaderSetFamilySourceString.fragmentShaderSourceString.c_str(),
                                                                    Anvil::ShaderStage::FRAGMENT);


        for (const auto& thisEmptyDefinition : in_shaderSpecs.emptyDefinition)
            shader_ptr->add_empty_definition(thisEmptyDefinition);

        for (const auto& thisDefinitionValuePair : in_shaderSpecs.definitionValuePairs)
            shader_ptr->add_definition_value_pair(thisDefinitionValuePair.first,
                                                  thisDefinitionValuePair.second);

        auto module_ptr = Anvil::ShaderModule::create_from_spirv_generator(in_device_ptr,
                                                                           shader_ptr.get());

        module_ptr->set_name("Fragment shader module of " + in_shaderSpecs.shadersSetFamilyName);

        std::unique_ptr<Anvil::ShaderModuleStageEntryPoint> entryPoint_ptr(new Anvil::ShaderModuleStageEntryPoint("main",
                                                                                                                  std::move(module_ptr),
                                                                                                                  Anvil::ShaderStage::FRAGMENT));
        
        thisShaderSet.fragmentShaderModule_ptr = entryPoint_ptr.get();

        shaderModulesStageEntryPoints_ptrs.emplace_back(std::move(entryPoint_ptr));
    }

    if (thisShaderSetFamilySourceString.geometryShaderSourceString.size())
    {
        auto shader_ptr = Anvil::GLSLShaderToSPIRVGenerator::create(in_device_ptr,
                                                                    Anvil::GLSLShaderToSPIRVGenerator::MODE_USE_SPECIFIED_SOURCE,
                                                                    thisShaderSetFamilySourceString.geometryShaderSourceString.c_str(),
                                                                    Anvil::ShaderStage::GEOMETRY);


        for (const auto& thisEmptyDefinition : in_shaderSpecs.emptyDefinition)
            shader_ptr->add_empty_definition(thisEmptyDefinition);

        for (const auto& thisDefinitionValuePair : in_shaderSpecs.definitionValuePairs)
            shader_ptr->add_definition_value_pair(thisDefinitionValuePair.first,
                                                  thisDefinitionValuePair.second);

        auto module_ptr = Anvil::ShaderModule::create_from_spirv_generator(in_device_ptr,
                                                                           shader_ptr.get());

        module_ptr->set_name("Geometry shader module of " + in_shaderSpecs.shadersSetFamilyName);

        std::unique_ptr<Anvil::ShaderModuleStageEntryPoint> entryPoint_ptr(new Anvil::ShaderModuleStageEntryPoint("main",
                                                                                                                  std::move(module_ptr),
                                                                                                                  Anvil::ShaderStage::GEOMETRY));

        thisShaderSet.geometryShaderModule_ptr = entryPoint_ptr.get();

        shaderModulesStageEntryPoints_ptrs.emplace_back(std::move(entryPoint_ptr));
    }

    if (thisShaderSetFamilySourceString.tessControlShaderSourceString.size())
    {
        auto shader_ptr = Anvil::GLSLShaderToSPIRVGenerator::create(in_device_ptr,
                                                                    Anvil::GLSLShaderToSPIRVGenerator::MODE_USE_SPECIFIED_SOURCE,
                                                                    thisShaderSetFamilySourceString.tessControlShaderSourceString.c_str(),
                                                                    Anvil::ShaderStage::TESSELLATION_CONTROL);


        for (const auto& thisEmptyDefinition : in_shaderSpecs.emptyDefinition)
            shader_ptr->add_empty_definition(thisEmptyDefinition);

        for (const auto& thisDefinitionValuePair : in_shaderSpecs.definitionValuePairs)
            shader_ptr->add_definition_value_pair(thisDefinitionValuePair.first,
                                                  thisDefinitionValuePair.second);

        auto module_ptr = Anvil::ShaderModule::create_from_spirv_generator(in_device_ptr,
                                                                           shader_ptr.get());

        module_ptr->set_name("Tesselation control shader module of " + in_shaderSpecs.shadersSetFamilyName);

        std::unique_ptr<Anvil::ShaderModuleStageEntryPoint> entryPoint_ptr(new Anvil::ShaderModuleStageEntryPoint("main",
                                                                                                                  std::move(module_ptr),
                                                                                                                  Anvil::ShaderStage::TESSELLATION_CONTROL));

        thisShaderSet.tessControlShaderModule_ptr = entryPoint_ptr.get();

        shaderModulesStageEntryPoints_ptrs.emplace_back(std::move(entryPoint_ptr));
    }

    if (thisShaderSetFamilySourceString.tessEvaluationShaderSourceString.size())
    {
        auto shader_ptr = Anvil::GLSLShaderToSPIRVGenerator::create(in_device_ptr,
                                                                    Anvil::GLSLShaderToSPIRVGenerator::MODE_USE_SPECIFIED_SOURCE,
                                                                    thisShaderSetFamilySourceString.tessEvaluationShaderSourceString.c_str(),
                                                                    Anvil::ShaderStage::TESSELLATION_EVALUATION);


        for (const auto& thisEmptyDefinition : in_shaderSpecs.emptyDefinition)
            shader_ptr->add_empty_definition(thisEmptyDefinition);

        for (const auto& thisDefinitionValuePair : in_shaderSpecs.definitionValuePairs)
            shader_ptr->add_definition_value_pair(thisDefinitionValuePair.first,
                                                  thisDefinitionValuePair.second);

        auto module_ptr = Anvil::ShaderModule::create_from_spirv_generator(in_device_ptr,
                                                                           shader_ptr.get());

        module_ptr->set_name("Tesselation evaluation shader module of " + in_shaderSpecs.shadersSetFamilyName);

        std::unique_ptr<Anvil::ShaderModuleStageEntryPoint> entryPoint_ptr(new Anvil::ShaderModuleStageEntryPoint("main",
                                                                                                                  std::move(module_ptr),
                                                                                                                  Anvil::ShaderStage::TESSELLATION_EVALUATION));

        thisShaderSet.tessEvaluationShaderModule_ptr = entryPoint_ptr.get();

        shaderModulesStageEntryPoints_ptrs.emplace_back(std::move(entryPoint_ptr));
    }

    if (thisShaderSetFamilySourceString.vertexShaderSourceString.size())
    {
        auto shader_ptr = Anvil::GLSLShaderToSPIRVGenerator::create(in_device_ptr,
                                                                    Anvil::GLSLShaderToSPIRVGenerator::MODE_USE_SPECIFIED_SOURCE,
                                                                    thisShaderSetFamilySourceString.vertexShaderSourceString.c_str(),
                                                                    Anvil::ShaderStage::VERTEX);


        for (const auto& thisEmptyDefinition : in_shaderSpecs.emptyDefinition)
            shader_ptr->add_empty_definition(thisEmptyDefinition);

        for (const auto& thisDefinitionValuePair : in_shaderSpecs.definitionValuePairs)
            shader_ptr->add_definition_value_pair(thisDefinitionValuePair.first,
                                                  thisDefinitionValuePair.second);

        auto module_ptr = Anvil::ShaderModule::create_from_spirv_generator(in_device_ptr,
                                                                           shader_ptr.get());

        module_ptr->set_name("Vertex shader module of " + in_shaderSpecs.shadersSetFamilyName);

        std::unique_ptr<Anvil::ShaderModuleStageEntryPoint> entryPoint_ptr(new Anvil::ShaderModuleStageEntryPoint("main",
                                                                                                                  std::move(module_ptr),
                                                                                                                  Anvil::ShaderStage::VERTEX));

        thisShaderSet.vertexShaderModule_ptr = entryPoint_ptr.get();

        shaderModulesStageEntryPoints_ptrs.emplace_back(std::move(entryPoint_ptr));
    }

    return thisShaderSet;
}