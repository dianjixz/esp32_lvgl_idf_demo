# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)
FILE(GLOB_RECURSE app_sources1 ${CMAKE_SOURCE_DIR}/lib/ST_7789_drv/*.*)
FILE(GLOB_RECURSE app_sources2 ${CMAKE_SOURCE_DIR}/lib/lvgl/src/*.*)
idf_component_register(SRCS ${app_sources} ${app_sources1} ${app_sources2} INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/lib/ST_7789_drv ${CMAKE_SOURCE_DIR}/lib/lvgl)
