cmake_minimum_required(VERSION 3.0.2)
project(inMyRoom_vulkan)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${inMyRoom_vulkan_SOURCE_DIR})

set(MATH_AVX ON)                                                           #Enable AVX at MathGeoLib. FM3 and FM4 can be also enabled

add_subdirectory(../Anvil "${CMAKE_CURRENT_BINARY_DIR}/anvil")
add_subdirectory(../Compressonator/Compressonator/Make "${CMAKE_CURRENT_BINARY_DIR}/compressonator")
add_subdirectory(../MathGeoLib "${CMAKE_CURRENT_BINARY_DIR}/mathgeolib")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
set(THREADS_PREFER_PTHREAD_FLAG ON)
add_compile_options(-m64)
if (MSVC)
    add_definitions(/arch:AVX)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    set(relFlags "/Ox /Ob2 /Oi /Ot /GT /GF /GS- /fp:fast /fp:except- /MP")
    set(relLinkFlags "/OPT:ICF /INCREMENTAL:NO")
	set(relFlags "${relFlags} /GL")
	set(relLinkFlags "${relLinkFlags} /LTCG")
	set(CMAKE_C_FLAGS_RELEASE     "${CMAKE_C_FLAGS_RELEASE} /Oy ${relFlags}")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Oy ${relFlags}")
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} ${relLinkFlags}")
	set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} ${relLinkFlags}")
	set(CMAKE_MODULE_LINKER_FLAGS_RELEASE "${CMAKE_MODULE_LINKER_FLAGS_RELEASE} ${relLinkFlags}")
else ()
    add_definitions(-mavx -march=corei7-avx -mtune=corei7-avx)
    set(OPT_FLAGS "-O3 -ffast-math")
    set(CMAKE_C_FLAGS_RELEASE   "${CMAKE_C_FLAGS_RELEASE} ${OPT_FLAGS}")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${OPT_FLAGS}")
endif()

find_package(Threads REQUIRED)

target_include_directories(Anvil PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/anvil/include")

include_directories(
        ../Anvil/include                                                   #Include Anvil
        ../Compressonator/Compressonator/Header                            #Include Compressonator
        ../MathGeoLib/src                                                  #Include MathGeoLib
        ../tinygltf                                                        #Include tinyglTF
        ../Configuru                                                       #Include configuru
        ../glm                                                             #Include glm
        ${inMyRoom_vulkan_SOURCE_DIR}/include)

SET(SRC "${inMyRoom_vulkan_SOURCE_DIR}/include/Engine.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/glTFenum.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/Graphics.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/hash_combine.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/InputManager.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/TexturesImagesUsage.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/MaterialsTextures.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/MeshesPrimitives.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/CameraBaseClass.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/NaiveCamera.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/NodesMeshes.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/PrimitivesMaterials.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/PrimitivesPipelines.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/PrimitivesShaders.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/SceneNodes.h"
        "${inMyRoom_vulkan_SOURCE_DIR}/include/WindowWithAsyncInput.h"
 
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Engine.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/Graphics.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/InputManager.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/TexturesImagesUsage.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/MaterialsTextures.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/MeshesPrimitives.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/CameraBaseClass.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/NaiveCamera.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/NodesMeshes.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/PrimitivesMaterials.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/PrimitivesPipelines.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/PrimitivesShaders.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/SceneNodes.cpp"
        "${inMyRoom_vulkan_SOURCE_DIR}/src/WindowWithAsyncInput.cpp"
 
        "${inMyRoom_vulkan_SOURCE_DIR}/src/main.cpp")

add_executable(inMyRoom_vulkan ${SRC})

add_dependencies(inMyRoom_vulkan Anvil)
add_dependencies(inMyRoom_vulkan Compressonator)
add_dependencies(inMyRoom_vulkan MathGeoLib)

if (WIN32)
    target_link_libraries(inMyRoom_vulkan Anvil)
    target_link_libraries(inMyRoom_vulkan Compressonator)
    target_link_libraries(inMyRoom_vulkan MathGeoLib)
else ()
    target_link_libraries(inMyRoom_vulkan Anvil dl)
    target_link_libraries(inMyRoom_vulkan Compressonator dl)
    target_link_libraries(inMyRoom_vulkan MathGeoLib dl)
endif ()

if (UNIX)
    target_link_libraries(inMyRoom_vulkan stdc++fs)
endif ()
