cmake_minimum_required(VERSION 3.25.0)
set(CMAKE_CXX_STANDARD 20)

project(MTX)
if (MSVC AND NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY)
    set (CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif ()

add_subdirectory(External/NRIFramework)

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} PUBLIC NRIFramework)
target_include_directories(${PROJECT_NAME} PUBLIC 
${CMAKE_SOURCE_DIR}/External/NRIFramework/Include
${CMAKE_SOURCE_DIR}/External/NRIFramework/External/
${CMAKE_SOURCE_DIR}/External/NRIFramework/External/nri/include
)