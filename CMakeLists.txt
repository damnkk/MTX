cmake_minimum_required(VERSION 3.25.0)
option(ENABLE_MTX_TEST ON)
set(SHADER_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/_Shaders")
set(CMAKE_CXX_STANDARD 20)

project(MTX)
if (MSVC AND NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY)
    set (CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif ()
#add_definitions(-DNRI_STATIC_LIBRARY)
add_definitions(-DBUILD_SHARED_LIBS=OFF -DASSIMP_WARNINGS_AS_ERRORS=OFF )

set(NRIF_SHADERS_PATH ${SHADER_OUTPUT_PATH} CACHE STRING "")
add_subdirectory(External/NRIFramework)

# External/NRD
set (NRD_DXC_CUSTOM_PATH "${DXC_CUSTOM_PATH}/NRD" CACHE STRING "")
set (NRD_SHADERS_PATH "${SHADER_OUTPUT_PATH}" CACHE STRING "")
set (NRD_NORMAL_ENCODING "2" CACHE STRING "")
set (NRD_ROUGHNESS_ENCODING "1" CACHE STRING "")
add_subdirectory(External/NRD)

file (GLOB_RECURSE SHADERS "Shaders/*.hlsl" "Shaders/*.hlsli")
message(${SHADERS})
set_source_files_properties (${SHADERS} PROPERTIES VS_TOOL_OVERRIDE "None")
add_custom_target (${PROJECT_NAME}_Shaders ALL
        ##disable dx shader compiler for now
            # COMMAND ShaderMake --useAPI
            #     --binary --flatten --stripReflection --compiler "${DXC_PATH}"
            #     --sourceDir "Shaders"
            #     -p DXIL
            #    -c Shaders.cfg
            #     -o "${SHADER_OUTPUT_PATH}"
            #     -I "Shaders"
            #     -I "External"
            #     -I "External/NRIFramework/External"
            #     -I "External/NRIFramework/External/NRI/Include"
            #     -I "External/NRIFramework/External/MathLib"
            #     -I "External/NRD/External"
            #     -D COMPILER_DXC
            COMMAND ShaderMake --useAPI
                --binary --flatten --stripReflection --compiler "${DXC_SPIRV_PATH}"
                --sourceDir "Shaders"
                -p SPIRV
               -c Shaders.cfg
                -o "${SHADER_OUTPUT_PATH}"
                -I "Shaders"
                -I "External"
                -I "External/NRIFramework/External"
                -I "External/NRIFramework/External/NRI/Include"
                -I "External/NRIFramework/External/MathLib"
                -I "External/NRD/External"
                -D COMPILER_DXC
                -D VULKAN
  
            DEPENDS ShaderMake
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            VERBATIM
            SOURCES ${SHADERS}
        )
set_property (TARGET ${PROJECT_NAME}_Shaders PROPERTY FOLDER ${PROJECT_NAME})
add_subdirectory(External/assimp)
set_target_properties(NRI  
            PROPERTIES 
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)
file(GLOB RENDERER_SOURCE 
./Src/*.cpp
./Src/*/*.cpp
)
file(GLOB NRD_INTERFACE ./External/NRD/integration/*.h ./External/NRD/integration/*.hpp)
message(${NRD_INTERFACE})
add_executable(${PROJECT_NAME} main.cpp ${RENDERER_SOURCE} ${NRD_INTERFACE})
target_link_libraries(${PROJECT_NAME} PUBLIC NRIFramework NRI NRD assimp)
target_include_directories(${PROJECT_NAME} PUBLIC 
${CMAKE_SOURCE_DIR}/External/NRIFramework/Include
${CMAKE_SOURCE_DIR}/External/NRIFramework/External/
${CMAKE_SOURCE_DIR}/External/NRIFramework/External/nri/include
${CMAKE_SOURCE_DIR}/External/NRD/include
${CMAKE_SOURCE_DIR}/External/NRD/integration
${CMAKE_SOURCE_DIR}/External/stduuid/include
${CMAKE_SOURCE_DIR}/External/Assimp/include
${CMAKE_SOURCE_DIR}/External/spdlog/include
${CMAKE_SOURCE_DIR}/External/glm
${CMAKE_SOURCE_DIR}/External/rttr/src
${CMAKE_SOURCE_DIR}/Include
)


if(ENABLE_MTX_TEST)
    include(CTest)
    enable_testing()
    add_subdirectory(External/googletest)
    add_subdirectory(./MTXTest)
endif()