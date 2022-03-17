#include "Graphics/Meshes/PrimitivesOfMeshes.h"

#include <cassert>
#include <algorithm>
#include <numeric>
#include <iostream>

#include "Graphics/HelperUtils.h"
#include "const_maps.h"

template<typename T_data,
         typename T_out, std::size_t bunch_out_size,
         typename T_in, std::size_t bunch_in_size>
std::vector<T_out> transformRange(const std::byte* begin, const std::byte* end,
                                  std::array<T_out, bunch_out_size> (*trans) (const std::array<T_in, bunch_in_size>&) )
{
    auto ptr = reinterpret_cast<const T_data*>(begin);
    auto ptr_end = reinterpret_cast<const T_data*>(end);
    assert((ptr_end - ptr) % bunch_in_size == 0);

    std::vector<T_out> ret_vec;
    while (ptr != ptr_end)
    {
        std::array<T_in, bunch_in_size> bunch = {};
        std::copy(ptr, ptr + bunch_in_size, bunch.data());

        if constexpr(std::is_integral_v<T_data> && std::is_floating_point_v<T_in>) {
            T_in divider = std::numeric_limits<T_data>::max();
            for(auto& this_ : bunch) {
                this_ /= divider;
                this_ = std::max(this_, T_in(-1.0));
            }
        }

        std::array<T_out, bunch_out_size> out_bunch = trans(bunch);
        std::copy(out_bunch.begin(), out_bunch.end(), std::back_inserter(ret_vec));

        ptr += bunch_in_size;
    }

    return ret_vec;
}

PrimitivesOfMeshes::PrimitiveInitializationData::PrimitiveInitializationData(const tinygltf::Model &model,
                                                                             const tinygltf::Primitive &primitive,
                                                                             const MaterialsOfPrimitives* materialsOfPrimitives_ptr)
{
    // Draw mode
    if (primitive.mode != -1) {
        drawMode = static_cast<glTFmode>(primitive.mode);
        if (drawMode == glTFmode::line_loop ) {
            std::cout << "Line loop is not supported, fallback to line strip.\n";
            drawMode = glTFmode::line_strip;
        }
    }

    // Indices
    if (primitive.indices != -1) {
        const tinygltf::Accessor& accessor = model.accessors[primitive.indices];
        std::pair<const std::byte*, const std::byte*> pair_begin_end_ptr = GetAccessorBeginEndPtrs(model, accessor);

        if (accessor.componentType == static_cast<int>(glTFcomponentType::type_unsigned_short))
            indices = transformRange<uint16_t>(pair_begin_end_ptr.first, pair_begin_end_ptr.second,
                                               +[](const std::array<uint32_t,1>& in) -> std::array<uint32_t,1> {return in;});
        else if (accessor.componentType == static_cast<int>(glTFcomponentType::type_unsigned_int))
            indices = transformRange<uint32_t>(pair_begin_end_ptr.first, pair_begin_end_ptr.second,
                                               +[](const std::array<uint32_t,1>& in) -> std::array<uint32_t,1> {return in;});
        else assert(0);
    }

    {// Position
        auto search = primitive.attributes.find("POSITION");
        assert(search != primitive.attributes.end());
        {
            std::vector<std::vector<float>> attributeAndTargets;

            const tinygltf::Accessor &accessor = model.accessors[search->second];
            std::pair<const std::byte *, const std::byte *> pair_begin_end_ptr = GetAccessorBeginEndPtrs(model, accessor);

            if (accessor.componentType == static_cast<int>(glTFcomponentType::type_float)) {
                std::vector attribute = transformRange<float>(pair_begin_end_ptr.first, pair_begin_end_ptr.second,
                                                              +[](const std::array<float, 3> &in) -> std::array<float, 4> {
                                                              std::array<float, 4> out = {};
                                                              out[0] = in[0]; out[1] = -in[1]; out[2] = -in[2]; out[3] = 1.f;
                                                              return out; });

                attributeAndTargets.emplace_back(std::move(attribute));
            } else
                assert (0);

            bool has_targets = primitive.targets.size() && primitive.targets[0].find("POSITION") != primitive.targets[0].end();
            if (has_targets) {
                for (const auto &this_target: primitive.targets) {
                    const tinygltf::Accessor &target_accessor = model.accessors[this_target.find("POSITION")->second];
                    std::pair<const std::byte *, const std::byte *> target_begin_end_ptr = GetAccessorBeginEndPtrs(
                            model, target_accessor);

                    if (target_accessor.componentType == static_cast<int>(glTFcomponentType::type_float)) {
                        std::vector target = transformRange<float>(target_begin_end_ptr.first,target_begin_end_ptr.second,
                                                                   +[](const std::array<float, 3> &in) -> std::array<float, 4> {
                                                                   std::array<float, 4> out = {};
                                                                   out[0] = in[0]; out[1] = -in[1]; out[2] = -in[2]; out[3] = 1.f;
                                                                   return out; });

                        attributeAndTargets.emplace_back(std::move(target));
                    }
                }
            }

            positionMorphTargets = int(attributeAndTargets.size() - 1);

            size_t components_count = attributeAndTargets[0].size() / 4;
            for (size_t i = 0; i != components_count; ++i) {
                for (const auto &this_vec: attributeAndTargets) {
                    position.emplace_back(this_vec[4 * i]);
                    position.emplace_back(this_vec[4 * i + 1]);
                    position.emplace_back(this_vec[4 * i + 2]);
                    position.emplace_back(this_vec[4 * i + 3]);
                }
            }
        }
    }


    {// Normal
        auto search = primitive.attributes.find("NORMAL");
        if (search != primitive.attributes.end()) {
            std::vector<std::vector<float>> attributeAndTargets;

            const tinygltf::Accessor &accessor = model.accessors[search->second];
            std::pair<const std::byte *, const std::byte *> pair_begin_end_ptr = GetAccessorBeginEndPtrs(model, accessor);

            if (accessor.componentType == static_cast<int>(glTFcomponentType::type_float)) {
                std::vector attribute = transformRange<float>(pair_begin_end_ptr.first, pair_begin_end_ptr.second,
                                                              +[](const std::array<float, 3> &in) -> std::array<float, 4> {
                                                              std::array<float, 4> out = {};
                                                              out[0] = in[0]; out[1] = -in[1]; out[2] = -in[2]; out[3] = 0.f;
                                                              return out;
                                                              });

                attributeAndTargets.emplace_back(std::move(attribute));
            } else
                assert (0);

            bool has_targets = primitive.targets.size() && primitive.targets[0].find("NORMAL") != primitive.targets[0].end();
            if (has_targets) {
                for (const auto &this_target: primitive.targets) {
                    const tinygltf::Accessor &target_accessor = model.accessors[this_target.find("NORMAL")->second];
                    std::pair<const std::byte *, const std::byte *> target_begin_end_ptr = GetAccessorBeginEndPtrs(model, target_accessor);

                    if (target_accessor.componentType == static_cast<int>(glTFcomponentType::type_float)) {
                        std::vector target = transformRange<float>(target_begin_end_ptr.first,target_begin_end_ptr.second,
                                                                   +[](const std::array<float, 3> &in) -> std::array<float, 4> {
                                                                    std::array<float, 4> out = {};
                                                                    out[0] = in[0]; out[1] = -in[1]; out[2] = -in[2]; out[3] = 0.f;
                                                                    return out;
                                                                   });

                        attributeAndTargets.emplace_back(std::move(target));
                    }
                }
            }

            normalMorphTargets = int(attributeAndTargets.size() - 1);

            size_t components_count = attributeAndTargets[0].size() / 4;
            for (size_t i = 0; i != components_count; ++i) {
                for (const auto &this_vec: attributeAndTargets) {
                    normal.emplace_back(this_vec[4 * i]);
                    normal.emplace_back(this_vec[4 * i + 1]);
                    normal.emplace_back(this_vec[4 * i + 2]);
                    normal.emplace_back(this_vec[4 * i + 3]);
                }
            }
        } else {
            // TODO: fallback
            assert(0);
        }
    }

    {// Tangent
        auto search = primitive.attributes.find("TANGENT");
        if (search != primitive.attributes.end()) {
            std::vector<std::vector<float>> attributeAndTargets;

            const tinygltf::Accessor &accessor = model.accessors[search->second];
            std::pair<const std::byte *, const std::byte *> pair_begin_end_ptr = GetAccessorBeginEndPtrs(model, accessor);

            if (accessor.componentType == static_cast<int>(glTFcomponentType::type_float)) {
                std::vector attribute = transformRange<float>(pair_begin_end_ptr.first, pair_begin_end_ptr.second,
                                                              +[](const std::array<float, 4> &in) -> std::array<float, 4> {
                                                               std::array<float, 4> out = {};
                                                               out[0] = in[0]; out[1] = -in[1]; out[2] = -in[2]; out[3] = in[3];
                                                               return out;
                                                              });

                attributeAndTargets.emplace_back(std::move(attribute));
            } else
                assert (0);

            bool has_targets = primitive.targets.size() && primitive.targets[0].find("TANGENT") != primitive.targets[0].end();
            if (has_targets) {
                for (const auto &this_target: primitive.targets) {
                    const tinygltf::Accessor &target_accessor = model.accessors[this_target.find("TANGENT")->second];
                    std::pair<const std::byte *, const std::byte *> target_begin_end_ptr = GetAccessorBeginEndPtrs(model, target_accessor);

                    if (target_accessor.componentType == static_cast<int>(glTFcomponentType::type_float)) {
                        std::vector target = transformRange<float>(target_begin_end_ptr.first,target_begin_end_ptr.second,
                                                                   +[](const std::array<float, 3> &in) -> std::array<float, 4> {
                                                                    std::array<float, 4> out = {};
                                                                    out[0] = in[0]; out[1] = -in[1]; out[2] = -in[2]; out[3] = 0.f;
                                                                    return out;
                                                                   });

                        attributeAndTargets.emplace_back(std::move(target));
                    }
                }
            }

            tangentMorphTargets = int(attributeAndTargets.size() - 1);

            size_t components_count = attributeAndTargets[0].size() / 4;
            for (size_t i = 0; i != components_count; ++i) {
                for (size_t vec_i = 0; vec_i != attributeAndTargets.size(); ++vec_i) {
                    if (vec_i == 0) {
                        tangent.emplace_back(attributeAndTargets[0][4 * i]);
                        tangent.emplace_back(attributeAndTargets[0][4 * i + 1]);
                        tangent.emplace_back(attributeAndTargets[0][4 * i + 2]);
                        tangent.emplace_back(attributeAndTargets[0][4 * i + 3]);
                    } else {
                        tangent.emplace_back(attributeAndTargets[vec_i][3 * i]);
                        tangent.emplace_back(attributeAndTargets[vec_i][3 * i + 1]);
                        tangent.emplace_back(attributeAndTargets[vec_i][3 * i + 2]);
                        tangent.emplace_back(attributeAndTargets[0][4 * i + 3]);
                    }
                }
            }
        } else {
            // TODO: fallback
            assert(0);
        }
    }

    {// Texcoords
        if (primitive.targets.size()) {
            auto target_upper_bound = primitive.targets[0].upper_bound("TEXCOORD_");
            if (target_upper_bound != primitive.targets[0].end() && target_upper_bound->first.starts_with("TEXCOORD_")) {
                texcoordsMorphTargets = int(primitive.targets.size());
            }
        }

        std::vector<std::vector<float>> texcoords_vec;
        while (true)
        {
            auto search = primitive.attributes.find("TEXCOORD_" + std::to_string(texcoordsCount));
            if (search != primitive.attributes.end()) {
                std::vector<std::vector<float>> attributeAndTargets;

                const tinygltf::Accessor &accessor = model.accessors[search->second];
                std::pair<const std::byte *, const std::byte *> pair_begin_end_ptr = GetAccessorBeginEndPtrs(model, accessor);

                if (accessor.componentType == static_cast<int>(glTFcomponentType::type_float)) {
                    std::vector attribute = transformRange<float>(pair_begin_end_ptr.first, pair_begin_end_ptr.second,
                                                                  +[](const std::array<float, 2> &in) -> std::array<float, 2> {
                                                                      std::array<float, 2> out = {};
                                                                      out[0] = in[0]; out[1] = in[1];
                                                                       return out;
                                                                  });

                    attributeAndTargets.emplace_back(std::move(attribute));
                } else if (accessor.componentType == static_cast<int>(glTFcomponentType::type_unsigned_byte)) {
                    std::vector attribute = transformRange<uint8_t>(pair_begin_end_ptr.first, pair_begin_end_ptr.second,
                                                                    +[](const std::array<float, 2> &in) -> std::array<float, 2> {
                                                                        std::array<float, 2> out = {};
                                                                        out[0] = in[0]; out[1] = in[1];
                                                                        return out;
                                                                    });

                    attributeAndTargets.emplace_back(std::move(attribute));
                } else if (accessor.componentType == static_cast<int>(glTFcomponentType::type_unsigned_short)) {
                    std::vector attribute = transformRange<uint16_t>(pair_begin_end_ptr.first, pair_begin_end_ptr.second,
                                                                     +[](const std::array<float, 2> &in) -> std::array<float, 2> {
                                                                         std::array<float, 2> out = {};
                                                                         out[0] = in[0]; out[1] = in[1];
                                                                         return out;
                                                                     });

                    attributeAndTargets.emplace_back(std::move(attribute));
                } else assert (0);

                bool this_has_targets = primitive.targets.size()
                        && primitive.targets[0].find("TEXCOORD_" + std::to_string(texcoordsCount)) != primitive.targets[0].end();
                if (this_has_targets) {
                    for (const auto &this_target: primitive.targets) {
                        const tinygltf::Accessor &target_accessor = model.accessors[this_target.find("TEXCOORD_" + std::to_string(texcoordsCount))->second];
                        std::pair<const std::byte *, const std::byte *> target_begin_end_ptr = GetAccessorBeginEndPtrs(
                                model, target_accessor);

                        if (target_accessor.componentType == static_cast<int>(glTFcomponentType::type_float)) {
                            std::vector target = transformRange<float>(target_begin_end_ptr.first, target_begin_end_ptr.second,
                                                                       +[](const std::array<float, 2> &in) -> std::array<float, 2> {
                                                                          std::array<float, 2> out = {};
                                                                          out[0] = in[0]; out[1] = in[1];
                                                                          return out;
                                                                       });

                            attributeAndTargets.emplace_back(std::move(target));
                        } else if (target_accessor.componentType == static_cast<int>(glTFcomponentType::type_unsigned_byte)) {
                            std::vector target = transformRange<uint8_t>(target_begin_end_ptr.first, target_begin_end_ptr.second,
                                                                         +[](const std::array<float, 2> &in) -> std::array<float, 2> {
                                                                            std::array<float, 2> out = {};
                                                                            out[0] = in[0]; out[1] = in[1];
                                                                            return out;
                                                                        });

                            attributeAndTargets.emplace_back(std::move(target));
                        } else if (target_accessor.componentType == static_cast<int>(glTFcomponentType::type_unsigned_short)) {
                            std::vector target = transformRange<uint16_t>(target_begin_end_ptr.first,target_begin_end_ptr.second,
                                                                          +[](const std::array<float, 2> &in) -> std::array<float, 2> {
                                                                              std::array<float, 2> out = {};
                                                                              out[0] = in[0]; out[1] = in[1];
                                                                              return out;
                                                                          });

                            attributeAndTargets.emplace_back(std::move(target));
                        } else if (target_accessor.componentType == static_cast<int>(glTFcomponentType::type_byte)) {
                            std::vector target = transformRange<int8_t>(target_begin_end_ptr.first,target_begin_end_ptr.second,
                                                                        +[](const std::array<float, 2> &in) -> std::array<float, 2> {
                                                                            std::array<float, 2> out = {};
                                                                            out[0] = in[0]; out[1] = in[1];
                                                                            return out;
                                                                        });

                            attributeAndTargets.emplace_back(std::move(target));
                        } else if (target_accessor.componentType == static_cast<int>(glTFcomponentType::type_short)) {
                            std::vector target = transformRange<int16_t>(target_begin_end_ptr.first,target_begin_end_ptr.second,
                                                                         +[](const std::array<float, 2> &in) -> std::array<float, 2> {
                                                                             std::array<float, 2> out = {};
                                                                             out[0] = in[0]; out[1] = in[1];
                                                                             return out;
                                                                         });

                            attributeAndTargets.emplace_back(std::move(target));
                        } else assert(0);
                    }
                } else if (texcoordsMorphTargets) {
                    for(size_t i = 0; i != texcoordsMorphTargets; ++i)
                        attributeAndTargets.emplace_back(attributeAndTargets[0]);
                }

                texcoords_vec.emplace_back();
                size_t components_count = attributeAndTargets[0].size() / 2;
                for (size_t i = 0; i != components_count; ++i) {
                    for (const auto &this_vec: attributeAndTargets) {
                        texcoords_vec.back().emplace_back(this_vec[2 * i]);
                        texcoords_vec.back().emplace_back(this_vec[2 * i + 1]);
                    }
                }

                texcoordsCount++;
            } else break;
        }

        if (texcoords_vec.size()) {
            size_t components_count = texcoords_vec[0].size() / (2 * (1+texcoordsMorphTargets));
            for (size_t i = 0; i != components_count; ++i) {
                for (const auto &this_vec: texcoords_vec) {
                    std::copy(this_vec.data()+ 2*i*(1+texcoordsMorphTargets),
                              this_vec.data()+ 2*(i+1)*(1+texcoordsMorphTargets),
                              std::back_inserter(texcoords));
                }
            }
        }

        // Align
        if(texcoords.size() % 4 == 2) {
            texcoords.emplace_back(0.f);
            texcoords.emplace_back(0.f);
        }
    }

    {// Color (only COLOR_0)
        auto search = primitive.attributes.find("COLOR_0");
        if (search != primitive.attributes.end()) {
            std::vector<std::vector<float>> attributeAndTargets;

            const tinygltf::Accessor &accessor = model.accessors[search->second];
            std::pair<const std::byte *, const std::byte *> pair_begin_end_ptr = GetAccessorBeginEndPtrs(model, accessor);

            if (accessor.type == static_cast<int>(glTFtype::type_vec3)) {
                if (accessor.componentType == static_cast<int>(glTFcomponentType::type_float)) {
                    std::vector attribute = transformRange<float>(pair_begin_end_ptr.first, pair_begin_end_ptr.second,
                                                                  +[](const std::array<float, 3> &in) -> std::array<float, 4> {
                                                                  std::array<float,4> out = {};
                                                                  out[0] = in[0]; out[1] = in[1]; out[2] = in[2]; out[3] = 1.f;
                                                                  return out;});

                    attributeAndTargets.emplace_back(std::move(attribute));
                } else if (accessor.componentType == static_cast<int>(glTFcomponentType::type_unsigned_byte)) {
                    std::vector attribute = transformRange<uint8_t>(pair_begin_end_ptr.first, pair_begin_end_ptr.second,
                                                                    +[](const std::array<float, 3> &in) -> std::array<float, 4> {
                                                                    std::array<float,4> out = {};
                                                                    out[0] = in[0]; out[1] = in[1]; out[2] = in[2]; out[3] = 1.f;
                                                                    return out;});

                    attributeAndTargets.emplace_back(std::move(attribute));
                } else if (accessor.componentType == static_cast<int>(glTFcomponentType::type_unsigned_short)) {
                    std::vector attribute = transformRange<uint16_t>(pair_begin_end_ptr.first, pair_begin_end_ptr.second,
                                                                     +[](const std::array<float, 3> &in) -> std::array<float, 4> {
                                                                     std::array<float,4> out = {};
                                                                     out[0] = in[0]; out[1] = in[1]; out[2] = in[2]; out[3] = 1.f;
                                                                     return out;});

                    attributeAndTargets.emplace_back(std::move(attribute));
                } else assert (0);
            } else if (accessor.type == static_cast<int>(glTFtype::type_vec4)) {
                if (accessor.componentType == static_cast<int>(glTFcomponentType::type_float)) {
                    std::vector attribute = transformRange<float>(pair_begin_end_ptr.first, pair_begin_end_ptr.second,
                                                                  +[](const std::array<float, 4> &in) -> std::array<float, 4> { return in;});

                    attributeAndTargets.emplace_back(std::move(attribute));
                } else if (accessor.componentType == static_cast<int>(glTFcomponentType::type_unsigned_byte)) {
                    std::vector attribute = transformRange<uint8_t>(pair_begin_end_ptr.first, pair_begin_end_ptr.second,
                                                                    +[](const std::array<float, 4> &in) -> std::array<float, 4> {return in;});

                    attributeAndTargets.emplace_back(std::move(attribute));
                } else if (accessor.componentType == static_cast<int>(glTFcomponentType::type_unsigned_short)) {
                    std::vector attribute = transformRange<uint16_t>(pair_begin_end_ptr.first, pair_begin_end_ptr.second,
                                                                     +[](const std::array<float, 4> &in) -> std::array<float, 4> {return in;});

                    attributeAndTargets.emplace_back(std::move(attribute));
                } else assert (0);
            } else assert(0);

            bool has_targets = primitive.targets.size() && primitive.targets[0].find("COLOR_0") != primitive.targets[0].end();
            if (has_targets) {
                for (const auto &this_target: primitive.targets) {
                    const tinygltf::Accessor &target_accessor = model.accessors[this_target.find("COLOR_0")->second];
                    std::pair<const std::byte *, const std::byte *> target_begin_end_ptr = GetAccessorBeginEndPtrs(model, target_accessor);

                    if (target_accessor.type == static_cast<int>(glTFtype::type_vec3)) {
                        if (target_accessor.componentType == static_cast<int>(glTFcomponentType::type_float)) {
                            std::vector target = transformRange<float>(target_begin_end_ptr.first, target_begin_end_ptr.second,
                                                                       +[](const std::array<float, 3> &in) -> std::array<float, 4> {
                                                                        std::array<float,4> out = {};
                                                                        out[0] = in[0]; out[1] = in[1]; out[2] = in[2]; out[3] = 0.f;
                                                                        return out;});

                            attributeAndTargets.emplace_back(std::move(target));
                        } else if (target_accessor.componentType == static_cast<int>(glTFcomponentType::type_unsigned_byte)) {
                            std::vector target = transformRange<uint8_t>(target_begin_end_ptr.first, target_begin_end_ptr.second,
                                                                         +[](const std::array<float, 3> &in) -> std::array<float, 4> {
                                                                          std::array<float,4> out = {};
                                                                          out[0] = in[0]; out[1] = in[1]; out[2] = in[2]; out[3] = 0.f;
                                                                          return out;});

                            attributeAndTargets.emplace_back(std::move(target));
                        } else if (target_accessor.componentType == static_cast<int>(glTFcomponentType::type_unsigned_short)) {
                            std::vector target = transformRange<uint16_t>(target_begin_end_ptr.first, target_begin_end_ptr.second,
                                                                          +[](const std::array<float, 3> &in) -> std::array<float, 4> {
                                                                           std::array<float,4> out = {};
                                                                           out[0] = in[0]; out[1] = in[1]; out[2] = in[2]; out[3] = 0.f;
                                                                           return out;});

                            attributeAndTargets.emplace_back(std::move(target));
                        } else if (target_accessor.componentType == static_cast<int>(glTFcomponentType::type_byte)) {
                            std::vector target = transformRange<int8_t>(target_begin_end_ptr.first, target_begin_end_ptr.second,
                                                                        +[](const std::array<float, 3> &in) -> std::array<float, 4> {
                                                                         std::array<float,4> out = {};
                                                                         out[0] = in[0]; out[1] = in[1]; out[2] = in[2]; out[3] = 0.f;
                                                                         return out;});

                            attributeAndTargets.emplace_back(std::move(target));
                        } else if (target_accessor.componentType == static_cast<int>(glTFcomponentType::type_short)) {
                            std::vector target = transformRange<int16_t>(target_begin_end_ptr.first, target_begin_end_ptr.second,
                                                                         +[](const std::array<float, 3> &in) -> std::array<float, 4> {
                                                                          std::array<float,4> out = {};
                                                                          out[0] = in[0]; out[1] = in[1]; out[2] = in[2]; out[3] = 0.f;
                                                                          return out;});

                            attributeAndTargets.emplace_back(std::move(target));
                        } else assert (0);
                    } else if (target_accessor.type == static_cast<int>(glTFtype::type_vec4)) {
                        if (target_accessor.componentType == static_cast<int>(glTFcomponentType::type_float)) {
                            std::vector target = transformRange<float>(target_begin_end_ptr.first, target_begin_end_ptr.second,
                                                                       +[](const std::array<float, 4> &in) -> std::array<float, 4> { return in;});

                            attributeAndTargets.emplace_back(std::move(target));
                        } else if (target_accessor.componentType == static_cast<int>(glTFcomponentType::type_unsigned_byte)) {
                            std::vector target = transformRange<uint8_t>(target_begin_end_ptr.first, target_begin_end_ptr.second,
                                                                         +[](const std::array<float, 4> &in) -> std::array<float, 4> {return in;});

                            attributeAndTargets.emplace_back(std::move(target));
                        } else if (target_accessor.componentType == static_cast<int>(glTFcomponentType::type_unsigned_short)) {
                            std::vector target = transformRange<uint16_t>(target_begin_end_ptr.first, target_begin_end_ptr.second,
                                                                          +[](const std::array<float, 4> &in) -> std::array<float, 4> {return in;});

                            attributeAndTargets.emplace_back(std::move(target));
                        } else if (target_accessor.componentType == static_cast<int>(glTFcomponentType::type_byte)) {
                            std::vector target = transformRange<int8_t>(target_begin_end_ptr.first, target_begin_end_ptr.second,
                                                                        +[](const std::array<float, 4> &in) -> std::array<float, 4> {return in;});

                            attributeAndTargets.emplace_back(std::move(target));
                        } else if (target_accessor.componentType == static_cast<int>(glTFcomponentType::type_short)) {
                            std::vector target = transformRange<int16_t>(target_begin_end_ptr.first, target_begin_end_ptr.second,
                                                                         +[](const std::array<float, 4> &in) -> std::array<float, 4> {return in;});

                            attributeAndTargets.emplace_back(std::move(target));
                        } else assert (0);
                    } else assert(0);
                }
            }

            colorMorphTargets = int(attributeAndTargets.size() - 1);

            size_t components_count = attributeAndTargets[0].size() / 4;
            for (size_t i = 0; i != components_count; ++i) {
                for (const auto &this_vec: attributeAndTargets) {
                    color.emplace_back(this_vec[4 * i]);
                    color.emplace_back(this_vec[4 * i + 1]);
                    color.emplace_back(this_vec[4 * i + 2]);
                    color.emplace_back(this_vec[4 * i + 3]);
                }
            }
        }
    }

    { // Joints
        std::vector<std::vector<uint16_t>> joints_vec;
        while (true) {
            auto search = primitive.attributes.find("JOINTS_" + std::to_string(jointsCount));
            if (search != primitive.attributes.end()) {
                std::vector<uint16_t> attribute;

                const tinygltf::Accessor &accessor = model.accessors[search->second];
                std::pair<const std::byte *, const std::byte *> pair_begin_end_ptr = GetAccessorBeginEndPtrs(model, accessor);

                if (accessor.componentType == static_cast<int>(glTFcomponentType::type_unsigned_byte)) {
                    attribute = transformRange<uint8_t>(pair_begin_end_ptr.first, pair_begin_end_ptr.second,
                                                        +[](const std::array<uint16_t, 4> &in) -> std::array<uint16_t, 4> { return in; });

                } else if (accessor.componentType == static_cast<int>(glTFcomponentType::type_unsigned_short)) {
                    attribute = transformRange<uint16_t>(pair_begin_end_ptr.first,
                                                         pair_begin_end_ptr.second,
                                                         +[](const std::array<uint16_t, 4> &in) -> std::array<uint16_t, 4> { return in; });
                } else
                    assert (0);

                joints_vec.emplace_back(std::move(attribute));
                jointsCount++;
            } else break;
        }

        if (joints_vec.size()) {
            size_t components_count = joints_vec[0].size() / 4;
            for (size_t i = 0; i != components_count; ++i) {
                for (const auto &this_vec: joints_vec) {
                    std::copy(this_vec.data() + 4*i,
                              this_vec.data() + 4*(i+1),
                              std::back_inserter(joints));
                }
            }
        }

        // Align
        if(joints.size() % 8 == 4) {
            joints.emplace_back(0);
            joints.emplace_back(0);
            joints.emplace_back(0);
            joints.emplace_back(0);
        }
    }

    { // Weight
        std::vector<std::vector<float>> weights_vec;
        while (true) {
            auto search = primitive.attributes.find("WEIGHTS_" + std::to_string(weightsCount));
            if (search != primitive.attributes.end()) {
                std::vector<float> attribute;

                const tinygltf::Accessor &accessor = model.accessors[search->second];
                std::pair<const std::byte *, const std::byte *> pair_begin_end_ptr = GetAccessorBeginEndPtrs(model, accessor);

                if (accessor.componentType == static_cast<int>(glTFcomponentType::type_float)) {
                    attribute = transformRange<float>(pair_begin_end_ptr.first, pair_begin_end_ptr.second,
                                                        +[](const std::array<float, 4> &in) -> std::array<float, 4> { return in; });

                } else if (accessor.componentType == static_cast<int>(glTFcomponentType::type_unsigned_byte)) {
                    attribute = transformRange<uint8_t>(pair_begin_end_ptr.first, pair_begin_end_ptr.second,
                                                        +[](const std::array<float, 4> &in) -> std::array<float, 4> { return in; });

                } else if (accessor.componentType == static_cast<int>(glTFcomponentType::type_unsigned_short)) {
                    attribute = transformRange<uint16_t>(pair_begin_end_ptr.first,
                                                         pair_begin_end_ptr.second,
                                                         +[](const std::array<float, 4> &in) -> std::array<float, 4> { return in; });
                } else
                    assert (0);

                weights_vec.emplace_back(std::move(attribute));
                weightsCount++;
            } else break;
        }

        if (weights_vec.size()) {
            size_t components_count = weights_vec[0].size() / 4;
            for (size_t i = 0; i != components_count; ++i) {
                for (const auto &this_vec: weights_vec) {
                    std::copy(this_vec.data() + 4*i,
                              this_vec.data() + 4*(i+1),
                              std::back_inserter(weights));
                }
            }
        }
    }

    assert(jointsCount == weightsCount);

    // Material
    if (primitive.material != -1)
        material = primitive.material + materialsOfPrimitives_ptr->GetMaterialIndexOffsetOfModel(model);
    else
        material = 0;       // Default material
}

PrimitivesOfMeshes::PrimitiveOBBtreeData
    PrimitivesOfMeshes::PrimitiveInitializationData::GetPrimitiveOBBtreeData() const
{
    PrimitivesOfMeshes::PrimitiveOBBtreeData return_data;

    if (positionMorphTargets == 0 && jointsCount == 0) {
        return_data.drawMode = drawMode;

        assert(position.size() % 4 == 0);
        for (auto it = position.begin(); it != position.end(); it += 4) {
            glm::vec3 vec(*it, *(it + 1), *(it + 2));
            return_data.points.emplace_back(vec);
        }

        if (normal.size()) {
            assert(normal.size() % 4 == 0);
            for (auto it = normal.begin(); it != normal.end(); it += 4) {
                glm::vec3 vec(*it, *(it + 1), *(it + 2));
                return_data.normals.emplace_back(vec);
            }
        }

        if (indices.size()) {
            return_data.indices = indices;
        } else {
            size_t indices_count = position.size();
            return_data.indices.resize(indices_count);
            std::iota(return_data.indices.begin(), return_data.indices.end(), 0);
        }
    } else {
        return_data.isSkinOrMorph = true;
    }

    return return_data;
}

size_t PrimitivesOfMeshes::PrimitiveInitializationData::IndicesBufferSize() const
{
    size_t size_bytes = 0;
    size_bytes += indices.size() * sizeof(uint32_t);

    return size_bytes;
}


size_t PrimitivesOfMeshes::PrimitiveInitializationData::VerticesBufferSize() const
{
    size_t size_bytes = 0;
    size_bytes += position.size()   * sizeof(float);
    size_bytes += normal.size()     * sizeof(float);
    size_bytes += tangent.size()    * sizeof(float);
    size_bytes += texcoords.size()  * sizeof(float);
    size_bytes += color.size()      * sizeof(float);
    size_bytes += joints.size()     * sizeof(uint16_t);
    size_bytes += weights.size()    * sizeof(float);

    return size_bytes;
}


std::pair<const std::byte *, const std::byte *>
PrimitivesOfMeshes::PrimitiveInitializationData::GetAccessorBeginEndPtrs(const tinygltf::Model &model,
                                                                         const tinygltf::Accessor &accessor)
{
    size_t size_of_each_component_in_byte;
    switch (static_cast<glTFcomponentType>(accessor.componentType))
    {
        default:
        case glTFcomponentType::type_byte:
        case glTFcomponentType::type_unsigned_byte:
            size_of_each_component_in_byte = sizeof(int8_t);
            break;
        case glTFcomponentType::type_short:
        case glTFcomponentType::type_unsigned_short:
            size_of_each_component_in_byte = sizeof(int16_t);
            break;
        case glTFcomponentType::type_int:
        case glTFcomponentType::type_unsigned_int:
        case glTFcomponentType::type_float:
            size_of_each_component_in_byte = sizeof(int32_t);
            break;
        case glTFcomponentType::type_double:
            size_of_each_component_in_byte = sizeof(int64_t);
            break;
    }

    size_t number_of_components_per_type;
    switch (static_cast<glTFtype>(accessor.type))
    {
        default:
        case glTFtype::type_scalar:
            number_of_components_per_type = 1;
            break;
        case glTFtype::type_vec2:
            number_of_components_per_type = 2;
            break;
        case glTFtype::type_vec3:
            number_of_components_per_type = 3;
            break;
        case glTFtype::type_vec4:
            number_of_components_per_type = 4;
            break;
    }

    size_t count_of_elements = accessor.count;

    size_t byte_offset = accessor.byteOffset;

    const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
    byte_offset += bufferView.byteOffset;

    const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

    auto begin_ptr = reinterpret_cast<const std::byte*>(&buffer.data[byte_offset]);
    auto end_ptr = reinterpret_cast<const std::byte*>(&buffer.data[byte_offset] + count_of_elements * number_of_components_per_type * size_of_each_component_in_byte);

    return {begin_ptr, end_ptr};
}



PrimitivesOfMeshes::PrimitivesOfMeshes(MaterialsOfPrimitives* in_materialsOfPrimitives_ptr,
                                       vk::Device in_device,
                                       vma::Allocator in_allocator)
    :
    materialsOfPrimitives_ptr(in_materialsOfPrimitives_ptr),
    device(in_device),
    vma_allocator(in_allocator)
{
    AddDefaultPrimitive();
}

PrimitivesOfMeshes::~PrimitivesOfMeshes()
{
    vma_allocator.destroyBuffer(buffer, allocation);
    vma_allocator.destroyBuffer(verticesBuffer, verticesAllocation);
}

void PrimitivesOfMeshes::AddDefaultPrimitive()
{
    PrimitiveInitializationData default_primitiveInitializationData;

    default_primitiveInitializationData.indices.emplace_back(0);
    default_primitiveInitializationData.indices.emplace_back(1);
    default_primitiveInitializationData.indices.emplace_back(2);

    for (size_t i = 0; i != 3; ++i) {
        default_primitiveInitializationData.position.emplace_back(0.f);
        default_primitiveInitializationData.position.emplace_back(0.f);
        default_primitiveInitializationData.position.emplace_back(0.f);
        default_primitiveInitializationData.position.emplace_back(1.f);

        default_primitiveInitializationData.normal.emplace_back(0.f);
        default_primitiveInitializationData.normal.emplace_back(0.f);
        default_primitiveInitializationData.normal.emplace_back(1.f);
        default_primitiveInitializationData.normal.emplace_back(0.f);

        default_primitiveInitializationData.normal.emplace_back(0.f);
        default_primitiveInitializationData.normal.emplace_back(1.f);
        default_primitiveInitializationData.normal.emplace_back(0.f);
        default_primitiveInitializationData.normal.emplace_back(0.f);

        default_primitiveInitializationData.texcoordsCount = 1;
        default_primitiveInitializationData.texcoords.emplace_back(0.5f);
        default_primitiveInitializationData.texcoords.emplace_back(0.5f);

        // Align
        default_primitiveInitializationData.texcoords.emplace_back(0.5f);
        default_primitiveInitializationData.texcoords.emplace_back(0.5f);

        default_primitiveInitializationData.color.emplace_back(1.0f);
        default_primitiveInitializationData.color.emplace_back(1.0f);
        default_primitiveInitializationData.color.emplace_back(1.0f);
        default_primitiveInitializationData.color.emplace_back(1.0f);
    }

    primitivesInitializationData.emplace_back(default_primitiveInitializationData);
}

size_t PrimitivesOfMeshes::AddPrimitive(const tinygltf::Model& model,
                                        const tinygltf::Primitive& primitive)
{
    size_t index = primitivesInitializationData.size();
    primitivesInitializationData.emplace_back(model, primitive, materialsOfPrimitives_ptr);

    if (recordingOBBtree) {
        recorderPrimitivesOBBtreeDatas.emplace_back(primitivesInitializationData.back().GetPrimitiveOBBtreeData());
    }

    return index;
}

void PrimitivesOfMeshes::StartRecordOBBtree()
{
    recordingOBBtree = true;
}

OBBtree PrimitivesOfMeshes::GetOBBtreeAndReset()
{
    std::vector<Triangle> triangles;

    for (PrimitiveOBBtreeData& this_primitiveCPUdata : recorderPrimitivesOBBtreeDatas)
    {
        if (not this_primitiveCPUdata.isSkinOrMorph)
        {
            std::vector<Triangle> this_triangles_list = Triangle::CreateTriangleList(this_primitiveCPUdata.points, this_primitiveCPUdata.normals,
                                                                                     this_primitiveCPUdata.indices, this_primitiveCPUdata.drawMode);

            std::copy(this_triangles_list.begin(),
                      this_triangles_list.end(),
                      std::back_inserter(triangles));
        }
    }

    OBBtree return_OBBtree(std::move(triangles));

    recorderPrimitivesOBBtreeDatas.clear();
    recordingOBBtree = false;

    return return_OBBtree;
}


void PrimitivesOfMeshes::FlashDevice(std::vector<std::pair<vk::Queue, uint32_t>> queues)
{
    assert(hasBeenFlashed == false);

    std::vector<uint32_t> share_families_indices;
    std::transform(queues.begin(), queues.end(), std::back_inserter(share_families_indices),
                   [](const auto& pair){return pair.second;});
    
    size_t indices_size_bytes = GetIndicesBufferSize();
    size_t vertices_size_bytes = GetVerticesBufferSize();

    InitializePrimitivesInfo();

    {
        vk::BufferCreateInfo buffer_create_info;
        buffer_create_info.size = indices_size_bytes + vertices_size_bytes;
        buffer_create_info.usage = vk::BufferUsageFlagBits::eIndexBuffer
                | vk::BufferUsageFlagBits::eVertexBuffer
                | vk::BufferUsageFlagBits::eTransferDst
                | vk::BufferUsageFlagBits::eStorageBuffer
                | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR
                | vk::BufferUsageFlagBits::eShaderDeviceAddress;
        buffer_create_info.sharingMode = vk::SharingMode::eConcurrent;
        buffer_create_info.setQueueFamilyIndices(share_families_indices);

        vma::AllocationCreateInfo allocation_create_info;
        allocation_create_info.usage = vma::MemoryUsage::eGpuOnly;

        auto createBuffer_result = vma_allocator.createBuffer(buffer_create_info, allocation_create_info);
        assert(createBuffer_result.result == vk::Result::eSuccess);
        buffer = createBuffer_result.value.first;
        allocation = createBuffer_result.value.second;
    }

    StagingBuffer staging_buffer(device, vma_allocator, indices_size_bytes + vertices_size_bytes);

    std::byte* dst_ptr = staging_buffer.GetDstPtr();

    CopyIndicesToBuffer(dst_ptr);
    CopyVerticesToBuffer(dst_ptr, indices_size_bytes);

    vk::CommandBuffer command_buffer = staging_buffer.BeginCommandRecord(queues[0]);
    vk::Buffer copy_buffer = staging_buffer.GetBuffer();

    vk::BufferCopy copy_region;
    copy_region.srcOffset = 0;
    copy_region.dstOffset = 0;
    copy_region.size = indices_size_bytes + vertices_size_bytes;
    command_buffer.copyBuffer(copy_buffer, buffer, 1, &copy_region);
    
    staging_buffer.EndAndSubmitCommands();

    FinishInitializePrimitivesInfo();
    hasBeenFlashed = true;
}


size_t PrimitivesOfMeshes::GetIndicesBufferSize() const {
    size_t size_bytes = 0;
    for (const auto& this_primitive: primitivesInitializationData) {
        size_bytes += this_primitive.IndicesBufferSize();
    }

    size_bytes += size_bytes % 16 == 0 ? 0 : (16 - size_bytes % 16) ;       // align
    return size_bytes;
}

size_t PrimitivesOfMeshes::GetVerticesBufferSize() const {
    size_t size_bytes = 0;
    for (const auto& this_primitive: primitivesInitializationData) {
        size_bytes += this_primitive.VerticesBufferSize();
    }

    return size_bytes;
}

void PrimitivesOfMeshes::InitializePrimitivesInfo()
{
    for (const auto& this_initializeData:primitivesInitializationData) {
        PrimitiveInfo this_info;

        {   // Primitive OBB
            std::vector<glm::vec3> points;

            assert(this_initializeData.position.size() % 4 == 0);
            for (size_t i = 0; i != this_initializeData.position.size() / 4; ++i) {
                points.emplace_back(this_initializeData.position[i * 4],
                                    this_initializeData.position[i * 4 + 1],
                                    this_initializeData.position[i * 4 + 2]);
            }
            this_info.primitiveOBB = OBB::CreateOBBfromPoints(points);
        }

        this_info.material = this_initializeData.material;
        this_info.materialTwoSided = materialsOfPrimitives_ptr->GetMaterialAbout(this_info.material).twoSided;

        this_info.verticesCount = this_initializeData.position.size() / 4;

        this_info.positionMorphTargets  = this_initializeData.positionMorphTargets;

        this_info.normalMorphTargets    = this_initializeData.normalMorphTargets;

        this_info.tangentMorphTargets   = this_initializeData.tangentMorphTargets;

        this_info.texcoordsCount        = this_initializeData.texcoordsCount;
        this_info.texcoordsMorphTargets = this_initializeData.texcoordsMorphTargets;

        this_info.colorMorphTargets     = this_initializeData.colorMorphTargets;

        this_info.jointsCount           = this_initializeData.jointsCount;

        this_info.weightsCount          = this_initializeData.weightsCount;

        primitivesInfo.emplace_back(this_info);
    }
}

void PrimitivesOfMeshes::FinishInitializePrimitivesInfo()
{
    primitivesInitializationData.clear();
}

void PrimitivesOfMeshes::CopyIndicesToBuffer(std::byte *ptr)
{
    // Create indices buffers for the primitives without one
    size_t max_vertex_iota_list = 0;
    size_t max_vertex_triangle_strip = 0;
    size_t max_vertex_triangle_fan = 0;
    for(const auto& this_initializeData : primitivesInitializationData) {
        if (this_initializeData.indices.empty()) {
            if (this_initializeData.drawMode == glTFmode::triangle_fan)
                max_vertex_triangle_fan = std::max(this_initializeData.position.size() / 4, max_vertex_triangle_fan);
            else if (this_initializeData.drawMode == glTFmode::line_strip)
                max_vertex_triangle_strip = std::max(this_initializeData.position.size() / 4, max_vertex_triangle_strip);
            else
                max_vertex_iota_list = std::max(this_initializeData.position.size() / 4, max_vertex_iota_list);
        }
    }

    size_t offset = 0;

    size_t iota_list_offset = offset;
    if (max_vertex_iota_list > 0) {
        std::vector<uint32_t> list_indices(max_vertex_iota_list);
        std::iota(list_indices.begin(), list_indices.end(), 0);

        size_t byte_size = list_indices.size() * sizeof(uint32_t);
        memcpy(ptr, list_indices.data(), byte_size);
        offset += byte_size;
    }

    size_t strip_list_offset = offset;
    if (max_vertex_triangle_strip > 0) {
        std::vector<uint32_t> list_strip_indices(max_vertex_triangle_strip);
        std::iota(list_strip_indices.begin(), list_strip_indices.end(), 0);

        std::vector<uint32_t> indices = TransformIndicesStripToList(list_strip_indices);

        size_t byte_size = indices.size() * sizeof(uint32_t);
        memcpy(ptr, indices.data(), byte_size);
        offset += byte_size;
    }

    size_t fan_list_offset = offset;
    if (max_vertex_triangle_fan > 0) {
        std::vector<uint32_t> list_fan_indices(max_vertex_triangle_fan);
        std::iota(list_fan_indices.begin(), list_fan_indices.end(), 0);

        std::vector<uint32_t> indices = TransformIndicesStripToList(list_fan_indices);

        size_t byte_size = indices.size() * sizeof(uint32_t);
        memcpy(ptr, indices.data(), byte_size);
        offset += byte_size;
    }

    for(size_t i = 0; i != primitivesInitializationData.size(); ++i) {
        const PrimitiveInitializationData& this_initializeData = primitivesInitializationData[i];
        PrimitiveInfo& this_info = primitivesInfo[i];

        if (this_initializeData.IndicesBufferSize()) {
            if (this_initializeData.drawMode == glTFmode::triangle_fan) {
                std::vector<uint32_t> indices = TransformIndicesFansToList(this_initializeData.indices);
                size_t indices_byte_size = indices.size() * sizeof(uint32_t);
                memcpy(ptr + offset, indices.data(), indices_byte_size);

                this_info.drawMode = vk::PrimitiveTopology::eTriangleList;
                this_info.indicesByteOffset = offset;
                this_info.indicesCount = indices.size();
                offset += indices_byte_size;
            } else if (this_initializeData.drawMode == glTFmode::triangle_strip) {
                std::vector<uint32_t> indices = TransformIndicesStripToList(this_initializeData.indices);
                size_t indices_byte_size = indices.size() * sizeof(uint32_t);
                memcpy(ptr + offset, indices.data(), indices_byte_size);

                this_info.drawMode = vk::PrimitiveTopology::eTriangleList;
                this_info.indicesByteOffset = offset;
                this_info.indicesCount = indices.size();
                offset += indices_byte_size;
            } else {
                size_t indices_byte_size = this_initializeData.IndicesBufferSize();
                memcpy(ptr + offset, this_initializeData.indices.data(), indices_byte_size);

                this_info.drawMode = glTFmodeToPrimitiveTopology_map.find(this_initializeData.drawMode)->second;
                this_info.indicesByteOffset = offset;
                this_info.indicesCount = this_initializeData.indices.size();
                offset += indices_byte_size;
            }
        } else {
            if (this_initializeData.drawMode == glTFmode::triangle_fan) {
                this_info.indicesByteOffset = fan_list_offset;
                this_info.indicesCount = (this_initializeData.position.size() / 4 - 2) * 3;
                this_info.drawMode = vk::PrimitiveTopology::eTriangleList;
            } else if (this_initializeData.drawMode == glTFmode::line_strip) {
                this_info.indicesByteOffset = strip_list_offset;
                this_info.indicesCount = (this_initializeData.position.size() / 4 - 2) * 3;
                this_info.drawMode = vk::PrimitiveTopology::eTriangleList;
            } else {
                this_info.indicesByteOffset = iota_list_offset;
                this_info.drawMode = glTFmodeToPrimitiveTopology_map.find(this_initializeData.drawMode)->second;
                this_info.indicesCount = this_initializeData.position.size() / 4;
            }
        }
    }

}

void PrimitivesOfMeshes::CopyVerticesToBuffer(std::byte *ptr, size_t offset)
{
    for(size_t i = 0; i != primitivesInitializationData.size(); ++i) {
        const PrimitiveInitializationData& this_initializeData = primitivesInitializationData[i];
        PrimitiveInfo& this_info = primitivesInfo[i];

        size_t position_byte_size = this_initializeData.position.size() * sizeof(float);
        assert(position_byte_size % 16 == 0);
        if (position_byte_size) {
            memcpy(ptr + offset, this_initializeData.position.data(), position_byte_size);
            this_info.positionByteOffset = offset;
            offset += position_byte_size;
        }

        size_t normal_byte_size = this_initializeData.normal.size() * sizeof(float);
        assert(normal_byte_size % 16 == 0);
        if (normal_byte_size) {
            memcpy(ptr + offset, this_initializeData.normal.data(), normal_byte_size);
            this_info.normalByteOffset = offset;
            offset += normal_byte_size;
        }

        size_t tangent_byte_size = this_initializeData.tangent.size() * sizeof(float);
        assert(tangent_byte_size % 16 == 0);
        if (tangent_byte_size) {
            memcpy(ptr + offset, this_initializeData.tangent.data(), tangent_byte_size);
            this_info.tangentByteOffset = offset;
            offset += tangent_byte_size;
        }

        size_t texcoords_byte_size = this_initializeData.texcoords.size() * sizeof(float);
        assert(texcoords_byte_size % 16 == 0);
        if (texcoords_byte_size) {
            memcpy(ptr + offset, this_initializeData.texcoords.data(), texcoords_byte_size);
            this_info.texcoordsByteOffset = offset;
            offset += texcoords_byte_size;
        }

        size_t color_byte_size = this_initializeData.color.size() * sizeof(float);
        assert(color_byte_size % 16 == 0);
        if (color_byte_size) {
            memcpy(ptr + offset, this_initializeData.color.data(), color_byte_size);
            this_info.colorByteOffset = offset;
            offset += color_byte_size;
        }

        size_t joints_byte_size = this_initializeData.joints.size() * sizeof(uint16_t);
        assert(joints_byte_size % 16 == 0);
        if (joints_byte_size) {
            memcpy(ptr + offset, this_initializeData.joints.data(), joints_byte_size);
            this_info.jointsByteOffset = offset;
            offset += joints_byte_size;
        }

        size_t weights_byte_size = this_initializeData.weights.size() * sizeof(float);
        assert(weights_byte_size % 16 == 0);
        if (weights_byte_size) {
            memcpy(ptr + offset, this_initializeData.weights.data(), weights_byte_size);
            this_info.weightsByteOffset = offset;
            offset += weights_byte_size;
        }
    }

}

std::vector<uint32_t> PrimitivesOfMeshes::TransformIndicesStripToList(const std::vector<uint32_t> &indices)
{
    std::vector<uint32_t> indices_list;

    size_t triangles_count = indices.size() - 2;
    for (size_t i = 0; i != triangles_count; ++i) {
        indices_list.emplace_back(indices[i]);
        indices_list.emplace_back(indices[i + (1+i%2)]);
        indices_list.emplace_back(indices[i + (2-i%2)]);
    }

    return indices_list;
}

std::vector<uint32_t> PrimitivesOfMeshes::TransformIndicesFansToList(const std::vector<uint32_t> &indices)
{
    std::vector<uint32_t> indices_list;

    size_t triangles_count = indices.size() - 2;
    for (size_t i = 0; i != triangles_count; ++i) {
        indices_list.emplace_back(indices[i]);
        indices_list.emplace_back(indices[i + 1]);
        indices_list.emplace_back(indices[i + 2]);
    }

    return indices_list;
}

bool PrimitivesOfMeshes::IsPrimitiveSkinned(size_t index) const
{
    if (not hasBeenFlashed) {
        const PrimitiveInitializationData& this_data = primitivesInitializationData[index];
        return this_data.jointsCount || this_data.weightsCount;
    } else {
        const PrimitiveInfo& this_info = primitivesInfo[index];
        return this_info.jointsCount || this_info.weightsCount;
    }
}

size_t PrimitivesOfMeshes::PrimitiveMorphTargetsCount(size_t index) const
{
    if (not hasBeenFlashed) {
        const PrimitiveInitializationData& this_data = primitivesInitializationData[index];
        return std::max({this_data.positionMorphTargets, this_data.normalMorphTargets,
                         this_data.tangentMorphTargets, this_data.texcoordsMorphTargets,
                         this_data.colorMorphTargets});
    } else {
        const PrimitiveInfo& this_info = primitivesInfo[index];
        return std::max({this_info.positionMorphTargets, this_info.normalMorphTargets,
                         this_info.tangentMorphTargets, this_info.texcoordsMorphTargets,
                         this_info.colorMorphTargets});
    }
}

std::tuple<bool, vk::AccelerationStructureGeometryKHR, vk::AccelerationStructureBuildRangeInfoKHR> PrimitivesOfMeshes::GetPrimitiveAccelerationStructureTriangle(size_t index)
{
    assert(hasBeenFlashed);

    const PrimitiveInfo& primitive_info = GetPrimitiveInfo(index);
    const MaterialAbout& material_about = materialsOfPrimitives_ptr->GetMaterialAbout(primitive_info.material);
    uint64_t buffer_device_address = device.getBufferAddress({buffer});

    if (primitive_info.drawMode == vk::PrimitiveTopology::eTriangleList) {
        vk::AccelerationStructureGeometryKHR acceleration_struct;
        acceleration_struct.geometryType = vk::GeometryTypeKHR::eTriangles;
        acceleration_struct.flags = (not material_about.masked && not material_about.transparent) ? vk::GeometryFlagBitsKHR::eOpaque : vk::GeometryFlagsKHR(0);
        acceleration_struct.geometry.triangles.sType = vk::StructureType::eAccelerationStructureGeometryTrianglesDataKHR;
        acceleration_struct.geometry.triangles.vertexFormat = vk::Format::eR32G32B32Sfloat;
        acceleration_struct.geometry.triangles.vertexData = buffer_device_address + primitive_info.positionByteOffset;
        acceleration_struct.geometry.triangles.vertexStride = sizeof(glm::vec4) * (primitive_info.positionMorphTargets + 1);
        acceleration_struct.geometry.triangles.maxVertex = primitive_info.verticesCount;
        acceleration_struct.geometry.triangles.indexType = vk::IndexType::eUint32;
        acceleration_struct.geometry.triangles.indexData = buffer_device_address + primitive_info.indicesByteOffset;
        acceleration_struct.geometry.triangles.transformData = nullptr;

        vk::AccelerationStructureBuildRangeInfoKHR acceleration_range;
        acceleration_range.primitiveCount = primitive_info.indicesCount / 3;
        acceleration_range.primitiveOffset = 0;
        acceleration_range.firstVertex = 0;
        acceleration_range.transformOffset = 0;

        return std::make_tuple(true, acceleration_struct, acceleration_range);
    } else {
        return std::make_tuple(false, vk::AccelerationStructureGeometryKHR(), vk::AccelerationStructureBuildRangeInfoKHR());
    }
}

