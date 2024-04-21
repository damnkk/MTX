FILE(GLOB TestCode  ./*.cpp)
message(${TestCode})
add_executable(MTXTest ${TestCode})
target_link_libraries(MTXTest gtest)
target_include_directories(MTXTest PUBLIC 
${CMAKE_SOURCE_DIR}/External/NRIFramework/Include
${CMAKE_SOURCE_DIR}/External/NRIFramework/External/
${CMAKE_SOURCE_DIR}/External/NRIFramework/External/nri/include
${CMAKE_SOURCE_DIR}/External/stduuid/include
${CMAKE_SOURCE_DIR}/External/spdlog/include
${CMAKE_SOURCE_DIR}/External/googleTest/googletest/include
${CMAKE_SOURCE_DIR}/Include
${CMAKE_SOURCE_DIR}/Src/Renderer
)
add_test(Name MTXTest COMMAND MTXTest)