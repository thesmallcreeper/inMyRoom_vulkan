cmake_minimum_required(VERSION 3.0.2)
project (inMyRoom_vulkan)

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    include(CheckCXXCompilerFlag)
    
    CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
    
    if(COMPILER_SUPPORTS_CXX11)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    else()
        message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
    endif()
endif()

# Add AMD Anvil
if (NOT ANVIL_LINK_EXAMPLES)
	add_subdirectory   (../Anvil "${CMAKE_CURRENT_BINARY_DIR}/anvil")
endif()

target_include_directories(Anvil PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/anvil/include")


include_directories(../Anvil/include
					../Configuru                                                       #Include configuru
                    ../glm                                                             #Include glm
					${inMyRoom_vulkan_SOURCE_DIR}/include)
					
SET (SRC      "${inMyRoom_vulkan_SOURCE_DIR}/include/Engine.h"
              "${inMyRoom_vulkan_SOURCE_DIR}/include/Graphics.h"
			  "${inMyRoom_vulkan_SOURCE_DIR}/include/InputManager.h"
			  "${inMyRoom_vulkan_SOURCE_DIR}/include/MovementBaseClass.h"
			  "${inMyRoom_vulkan_SOURCE_DIR}/include/NaiveCamera.h"
			  "${inMyRoom_vulkan_SOURCE_DIR}/include/teapot_data.h"
			  "${inMyRoom_vulkan_SOURCE_DIR}/include/WindowWithAsyncInput.h"
			  "${inMyRoom_vulkan_SOURCE_DIR}/src/Engine.cpp"
			  "${inMyRoom_vulkan_SOURCE_DIR}/src/Graphics.cpp"
			  "${inMyRoom_vulkan_SOURCE_DIR}/src/InputManager.cpp"
			  "${inMyRoom_vulkan_SOURCE_DIR}/src/MovementBaseClass.cpp"
			  "${inMyRoom_vulkan_SOURCE_DIR}/src/NaiveCamera.cpp"
			  "${inMyRoom_vulkan_SOURCE_DIR}/src/teapot_data.cpp"
			  "${inMyRoom_vulkan_SOURCE_DIR}/src/WindowWithAsyncInput.cpp"
			  "${inMyRoom_vulkan_SOURCE_DIR}/src/main.cpp")
			  
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${inMyRoom_vulkan_SOURCE_DIR})
add_executable (inMyRoom_vulkan ${SRC})

add_dependencies     (inMyRoom_vulkan Anvil)


if (WIN32)
    target_link_libraries(inMyRoom_vulkan Anvil)
else()
    target_link_libraries(inMyRoom_vulkan Anvil dl)
endif()