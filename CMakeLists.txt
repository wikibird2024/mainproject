cmake_minimum_required(VERSION 3.16)

# Khai báo thư mục chứa các component tự tạo
set(EXTRA_COMPONENT_DIRS components)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(mainproject)
