cmake_minimum_required (VERSION 3.16)

project(rcopy)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
#set(CMAKE_VERBOSE_MAKEFILE ON)
if (MSVC)
	set(CMAKE_CXX_STANDARD 23)
else (MSVC)
	set(CMAKE_CXX_STANDARD 20)
endif (MSVC)
	
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})

add_executable(rcopy main.cpp crc32.h db.h)
target_include_directories(rcopy PRIVATE cxx/include)
