#include "Graphics/ShadersSetsFamiliesCache.h"

#include <fstream>
#include <iostream>
#include <cassert>

#include "const_maps.h"


ShadersSetsFamiliesCache::ShadersSetsFamiliesCache(vk::Device in_device,
                                                   std::string shaders_folder)
    :
    device(in_device),
    shadersFolder(std::move(shaders_folder))
{
}

ShadersSetsFamiliesCache::~ShadersSetsFamiliesCache()
{
    for ( auto& this_pair: spirvToShaderModule_umap)
    {
        device.destroyShaderModule(this_pair.second);
    }
}

void ShadersSetsFamiliesCache::AddShadersSetsFamily(const ShadersSetsFamilyInitInfo& shadersSetsFamilyInitInfos)
{
    ShadersSetsFamilySourceStrings this_shaderSetSourceString;

    if (shadersSetsFamilyInitInfos.fragmentShaderSourceFilename.size()) {
        std::ifstream source_file(shadersFolder + "/" + shadersSetsFamilyInitInfos.fragmentShaderSourceFilename);
        std::string source_string((std::istreambuf_iterator<char>(source_file)), (std::istreambuf_iterator<char>()));
        this_shaderSetSourceString.fragmentShaderSourceString = source_string;
    }

    if (shadersSetsFamilyInitInfos.geometryShaderSourceFilename.size()) {
        std::ifstream source_file(shadersFolder + "/" + shadersSetsFamilyInitInfos.geometryShaderSourceFilename);
        std::string source_string((std::istreambuf_iterator<char>(source_file)), (std::istreambuf_iterator<char>()));
        this_shaderSetSourceString.geometryShaderSourceString = source_string;
    }

    if (shadersSetsFamilyInitInfos.tessControlShaderSourceFilename.size()) {
        std::ifstream source_file(shadersFolder + "/" + shadersSetsFamilyInitInfos.tessControlShaderSourceFilename);
        std::string source_string((std::istreambuf_iterator<char>(source_file)), (std::istreambuf_iterator<char>()));
        this_shaderSetSourceString.tessControlShaderSourceString = source_string;
    }

    if (shadersSetsFamilyInitInfos.tessEvaluationShaderSourceFilename.size()) {
        std::ifstream source_file(shadersFolder + "/" + shadersSetsFamilyInitInfos.tessEvaluationShaderSourceFilename);
        std::string source_string((std::istreambuf_iterator<char>(source_file)), (std::istreambuf_iterator<char>()));
        this_shaderSetSourceString.tessEvaluationShaderSourceString = source_string;
    }

    if (shadersSetsFamilyInitInfos.vertexShaderSourceFilename.size()) {
        std::ifstream source_file(shadersFolder + "/" + shadersSetsFamilyInitInfos.vertexShaderSourceFilename);
        std::string source_string((std::istreambuf_iterator<char>(source_file)), (std::istreambuf_iterator<char>()));
        this_shaderSetSourceString.vertexShaderSourceString = source_string;
    }

    if (shadersSetsFamilyInitInfos.computeShaderSourceFilename.size()) {
        std::ifstream source_file(shadersFolder + "/" + shadersSetsFamilyInitInfos.computeShaderSourceFilename);
        std::string source_string((std::istreambuf_iterator<char>(source_file)), (std::istreambuf_iterator<char>()));
        this_shaderSetSourceString.computeShaderSourceString = source_string;
    }

    this_shaderSetSourceString.abortShaderIfDefinitionFound = shadersSetsFamilyInitInfos.abortShaderIfDefinitionFound;

    shadersSetFamilyNameToShadersSetFamilySourceStrings_umap.emplace(shadersSetsFamilyInitInfos.shadersSetFamilyName, this_shaderSetSourceString);
}

ShadersSet ShadersSetsFamiliesCache::GetShadersSet(const ShadersSpecs& shaderSpecs)
{
    auto search = shaderSpecsToShadersSet_umap.find(shaderSpecs);
    if (search != shaderSpecsToShadersSet_umap.end()) {
        return search->second;
    } else {
        ShadersSet newShaderSet = CreateShadersSet(shaderSpecs);
        shaderSpecsToShadersSet_umap.emplace(shaderSpecs, newShaderSet);

        return newShaderSet;
    }
}

ShadersSet ShadersSetsFamiliesCache::CreateShadersSet(const ShadersSpecs& shaderSpecs)
{
    auto search = shadersSetFamilyNameToShadersSetFamilySourceStrings_umap.find(shaderSpecs.shadersSetFamilyName);
    assert(search != shadersSetFamilyNameToShadersSetFamilySourceStrings_umap.end());

    const ShadersSetsFamilySourceStrings& this_shaderSetFamilySourceString = search->second;

    ShadersSet thisShaderSet;
    for (const auto& this_pair: shaderSpecs.definitionStringPairs) {
        auto search = this_shaderSetFamilySourceString.abortShaderIfDefinitionFound.find(this_pair.first);
        if (search != this_shaderSetFamilySourceString.abortShaderIfDefinitionFound.end()) {
            thisShaderSet.abortedDueToDefinition = true;
            break;
        }
    }

    if (not thisShaderSet.abortedDueToDefinition) {
        if (this_shaderSetFamilySourceString.fragmentShaderSourceString.size()) {
            std::vector<uint32_t> spirv_binary = CompileGLSLtoSPIRv(this_shaderSetFamilySourceString.fragmentShaderSourceString,
                                                                    shaderSpecs.shadersSetFamilyName,
                                                                    shaderSpecs.definitionStringPairs,
                                                                    vk::ShaderStageFlagBits::eFragment);

            thisShaderSet.fragmentShaderModule = GetShaderModule(std::move(spirv_binary),
                                                                 "Fragment shader module failed.");
        }

        if (this_shaderSetFamilySourceString.geometryShaderSourceString.size()) {
            std::vector<uint32_t> spirv_binary = CompileGLSLtoSPIRv(this_shaderSetFamilySourceString.geometryShaderSourceString,
                                                                    shaderSpecs.shadersSetFamilyName,
                                                                    shaderSpecs.definitionStringPairs,
                                                                    vk::ShaderStageFlagBits::eGeometry);

            thisShaderSet.geometryShaderModule = GetShaderModule(std::move(spirv_binary),
                                                                 "Geometry shader module failed.");
        }

        if (this_shaderSetFamilySourceString.tessControlShaderSourceString.size()) {
            std::vector<uint32_t> spirv_binary = CompileGLSLtoSPIRv(this_shaderSetFamilySourceString.tessControlShaderSourceString,
                                                                    shaderSpecs.shadersSetFamilyName,
                                                                    shaderSpecs.definitionStringPairs,
                                                                    vk::ShaderStageFlagBits::eTessellationControl);

            thisShaderSet.tessControlShaderModule = GetShaderModule(std::move(spirv_binary),
                                                                    "Tesselation Control shader module failed.");
        }

        if (this_shaderSetFamilySourceString.tessEvaluationShaderSourceString.size()) {
            std::vector<uint32_t> spirv_binary = CompileGLSLtoSPIRv(this_shaderSetFamilySourceString.tessEvaluationShaderSourceString,
                                                                    shaderSpecs.shadersSetFamilyName,
                                                                    shaderSpecs.definitionStringPairs,
                                                                    vk::ShaderStageFlagBits::eTessellationEvaluation);

            thisShaderSet.tessEvaluationShaderModule = GetShaderModule(std::move(spirv_binary),
                                                                       "Tesselation Evaluation shader module failed.");
        }

        if (this_shaderSetFamilySourceString.vertexShaderSourceString.size()) {
            std::vector<uint32_t> spirv_binary = CompileGLSLtoSPIRv(this_shaderSetFamilySourceString.vertexShaderSourceString,
                                                                    shaderSpecs.shadersSetFamilyName,
                                                                    shaderSpecs.definitionStringPairs,
                                                                    vk::ShaderStageFlagBits::eVertex);

            thisShaderSet.vertexShaderModule = GetShaderModule(std::move(spirv_binary),
                                                               "Tesselation Evaluation shader module failed.");
        }

        if (this_shaderSetFamilySourceString.computeShaderSourceString.size()) {
            std::vector<uint32_t> spirv_binary = CompileGLSLtoSPIRv(this_shaderSetFamilySourceString.computeShaderSourceString,
                                                                    shaderSpecs.shadersSetFamilyName,
                                                                    shaderSpecs.definitionStringPairs,
                                                                    vk::ShaderStageFlagBits::eCompute);

            thisShaderSet.computeShaderModule = GetShaderModule(std::move(spirv_binary),
                                                                "Compute shader module failed.");
        }
    }

    return thisShaderSet;
}

class IncluderInterfaceImplementation: public shaderc::CompileOptions::IncluderInterface
{
public:
    explicit IncluderInterfaceImplementation(std::string in_folder)
        :folder(std::move(in_folder))
    {
    }

    ~IncluderInterfaceImplementation() override = default;

    shaderc_include_result* GetInclude(const char* requested_source,
                                       shaderc_include_type type,
                                       const char* requesting_source,
                                       size_t include_depth) override
    {
        std::unique_ptr<std::string> include_name_uptr = std::make_unique<std::string>(folder + "/" +std::string(requesting_source));

        std::ifstream include_file(*include_name_uptr);
        std::unique_ptr<std::string> include_source_uptr
            = std::make_unique<std::string>((std::istreambuf_iterator<char>(include_file)), (std::istreambuf_iterator<char>()));

        std::unique_ptr<shaderc_include_result> include_result_uptr;
        include_result_uptr->source_name = include_name_uptr->c_str();
        include_result_uptr->source_name_length = include_name_uptr->size();
        include_result_uptr->content = include_source_uptr->c_str();
        include_result_uptr->content_length = include_source_uptr->size();

        shaderc_include_result* return_ptr = include_result_uptr.get();

        strings_uptrs.emplace_back(std::move(include_name_uptr));
        strings_uptrs.emplace_back(std::move(include_source_uptr));
        include_results_uptrs.emplace_back(std::move(include_result_uptr));

        return return_ptr;
    }

    // Handles shaderc_include_result_release_fn callbacks.
    void ReleaseInclude(shaderc_include_result* data) override {}
private:
    std::string folder;

    std::vector<std::unique_ptr<std::string>> strings_uptrs;
    std::vector<std::unique_ptr<shaderc_include_result>> include_results_uptrs;

};

std::vector<uint32_t>
    ShadersSetsFamiliesCache::CompileGLSLtoSPIRv(const std::string &shader_source,
                                                 const std::string &family_name,
                                                 const std::vector<std::pair<std::string, std::string>> &definition_pairs,
                                                 vk::ShaderStageFlagBits shader_stage)
{
    shaderc_shader_kind shaderc_shaderKind;
    {
        auto search = shaderStageToShadercShaderKind_map.find(shader_stage);
        assert(search != shaderStageToShadercShaderKind_map.end());
        shaderc_shaderKind = search->second;
    }

    shaderc::Compiler compiler;
    shaderc::CompileOptions options;

    // Add defines
    for (const auto& this_pair: definition_pairs) {
        options.AddMacroDefinition(this_pair.first, this_pair.second);
    }

    // Add includer
    std::unique_ptr<shaderc::CompileOptions::IncluderInterface> includer = std::make_unique<IncluderInterfaceImplementation>(shadersFolder);
    options.SetIncluder(std::move(includer));

    // Preprocess
    shaderc::PreprocessedSourceCompilationResult result =
        compiler.PreprocessGlsl(shader_source,
                                shaderc_shaderKind,
                                family_name.c_str(),
                                options);

    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        std::cerr << "Shader preprocess failed!\n";
        std::cerr << result.GetErrorMessage();
        std::terminate();
    }
    std::string preprocessed_source = {result.cbegin(), result.cend()};

    // Optimizations
    options.SetOptimizationLevel(shaderc_optimization_level_performance);

    // Compile to SPIRv
    shaderc::SpvCompilationResult module =
            compiler.CompileGlslToSpv(preprocessed_source,
                                      shaderc_shaderKind,
                                      family_name.c_str(),
                                      options);

    if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
        std::cerr << "Shader compilation failed!\n error code = " + std::to_string(module.GetCompilationStatus()) + "\n";
        std::cerr << module.GetErrorMessage();
        std::cerr << preprocessed_source;
        std::terminate();
    }

    return {module.cbegin(), module.cend()};
}

vk::ShaderModule ShadersSetsFamiliesCache::GetShaderModule(std::vector<uint32_t>&& spirv_binary,
                                                           const std::string &message_on_fail)
{
    auto search = spirvToShaderModule_umap.find(spirv_binary);
    if(search != spirvToShaderModule_umap.end()) {
        return search->second;
    } else {
        auto module_opt = device.createShaderModule(vk::ShaderModuleCreateInfo(vk::ShaderModuleCreateFlags(), spirv_binary));
        if (module_opt.result != vk::Result::eSuccess) {
            std::cerr << message_on_fail << "\n";
            std::terminate();
        }

        spirvToShaderModule_umap.emplace(std::move(spirv_binary), module_opt.value);

        return module_opt.value;
    }
}
