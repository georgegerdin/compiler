cmake_minimum_required(VERSION 3.5)

project(compiler)

find_package(Boost 1.63 REQUIRED) 

add_compile_options(-std=c++1z)

add_executable(gc main.cc lexer.hh parser.hh symboltable.hh sema.hh)

