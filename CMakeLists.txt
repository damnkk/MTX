cmake_minimum_required(VERSION 3.25.0)
set(CMAKE_CXX_STANDARD 20)
option(ENABLE_MTX_TEST OFF)

project(MTX)
if (MSVC AND NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY)
    set (CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif ()

add_subdirectory(External/NRIFramework)
file(GLOB RENDERER_SOURCE ./Src/*/*.cpp
)

add_executable(${PROJECT_NAME} main.cpp ${RENDERER_SOURCE})
target_link_libraries(${PROJECT_NAME} PUBLIC NRIFramework)
target_include_directories(${PROJECT_NAME} PUBLIC 
${CMAKE_SOURCE_DIR}/External/NRIFramework/Include
${CMAKE_SOURCE_DIR}/External/NRIFramework/External/
${CMAKE_SOURCE_DIR}/External/NRIFramework/External/nri/include
${CMAKE_SOURCE_DIR}/External/stduuid/include
${CMAKE_SOURCE_DIR}/External/spdlog/include
${CMAKE_SOURCE_DIR}/External/rttr/src
${CMAKE_SOURCE_DIR}/Include
)

if(ENABLE_MTX_TEST)
    include(CTest)
    enable_testing()
    add_subdirectory(External/googletest)
    add_subdirectory(./MTXTest)
endif()