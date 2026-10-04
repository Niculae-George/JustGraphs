cmake_minimum_required(VERSION 3.21)

project(JustGraphs LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.9 REQUIRED COMPONENTS Core Gui Widgets)
qt_standard_project_setup()
set(CMAKE_AUTORCC ON)

include(CTest)
option(JUSTGRAPHS_SANITIZE "Enable address/undefined sanitizers for app and tests" OFF)
if(JUSTGRAPHS_SANITIZE AND MSVC)
    message(FATAL_ERROR "JUSTGRAPHS_SANITIZE requires GCC or Clang with address/undefined sanitizers")
endif()
add_subdirectory(JustGraphs)
