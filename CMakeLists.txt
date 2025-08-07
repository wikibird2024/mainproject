cmake_minimum_required(VERSION 3.16)

# Declare manual created
# This is a better way to register your custom component directory
set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/components")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(mainproject)
