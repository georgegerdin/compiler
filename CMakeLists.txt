cmake_minimum_required(VERSION 3.5)

project(compiler)

find_package(Boost 1.63 REQUIRED) 

add_compile_options(-std=c++1z)
if(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
endif(MSVC)


add_executable(gc main.cc lexer.hh parser.hh symboltable.hh sema.hh)

target_include_directories(gc PRIVATE ${Boost_INCLUDE_DIR})