cmake_minimum_required(VERSION 3.10)
project(png2bin)
set(SOURCE_FILES
	src/png2bin.c)
add_executable (png2bin ${SOURCE_FILES})

## Link libraries
find_package(PNG REQUIRED)
include_directories(${PNG_INCLUDE_DIR} /usr/local/include)
target_link_libraries(png2bin ${PNG_LIBRARY})
