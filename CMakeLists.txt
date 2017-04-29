cmake_minimum_required(VERSION 3.5)

project(compiler)

add_compile_options(-std=c++1y)

add_executable(gc main.cc lexer.hh parser.hh)
