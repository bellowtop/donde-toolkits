cmake_minimum_required(VERSION 3.0)

project(cppchannel)

add_library(${PROJECT_NAME} STATIC channel.h channel.cpp)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
target_link_libraries(${PROJECT_NAME} -lpthread)


install(TARGETS ${PROJECT_NAME} EXPORT ${CMAKE_PROJECT_NAME}Targets)
