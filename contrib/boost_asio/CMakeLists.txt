#
# CMake configuration file for boost_asio
#
cmake_minimum_required (VERSION 2.8.12)
project (boost_asio)

# project version
set (${PROJECT_NAME}_VERSION 0.0.1)
set (${PROJECT_NAME}_SOVERSION ${${PROJECT_NAME}_VERSION})

# dependencies
find_package (Boost REQUIRED system)

add_definitions (-std=c++11 -Werror -DBOOST_ASIO_DYN_LINK -DBOOST_NO_AUTO_PTR)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fvisibility-inlines-hidden")
set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -fvisibility-inlines-hidden")

# header paths
include_directories(
	${PROJECT_BINARY_DIR}
  ${PROJECT_SOURCE_DIR}
	${Boost_INCLUDE_DIRS}
)

# library paths
link_directories(
  ${Boost_LIBRARY_DIRS}
)

# build
file (GLOB ASIO_SOURCES boost_asio/*.cpp)

add_library(${PROJECT_NAME} ${ASIO_SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES
	VERSION ${${PROJECT_NAME}_VERSION}
	SOVERSION ${${PROJECT_NAME}_SOVERSION}
)

install (TARGETS ${PROJECT_NAME} DESTINATION lib)
