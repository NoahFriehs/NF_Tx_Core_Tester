cmake_minimum_required(VERSION 3.26)
project(NF_Tx_Core_Tester)

set(CMAKE_CXX_STANDARD 20)

add_executable(NF_Tx_Core_Tester main.cpp
        LibLoader.h)
