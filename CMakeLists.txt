
cmake_minimum_required(VERSION 3.25)

project(quickproto)

find_package(entt CONFIG REQUIRED)

add_executable(quickproto main.cpp)
target_compile_features(quickproto PRIVATE cxx_std_23)
target_include_directories(quickproto PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(quickproto PRIVATE EnTT::EnTT)
