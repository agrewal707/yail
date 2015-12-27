cmake_minimum_required (VERSION 2.8.12)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/contrib/boost_asio/cmake)

# dependencies
find_package (Boost REQUIRED program_options system)
find_package (Protobuf REQUIRED)

if(YAIL_BUILD_BOOST_ASIO_LIBRARY)
	set (boost_asio_LIBRARIES boost_asio)
else(YAIL_BUILD_BOOST_ASIO_LIBRARY)
	find_package (boost_asio REQUIRED)
endif(YAIL_BUILD_BOOST_ASIO_LIBRARY)

# build flags
add_definitions (-std=c++11 -Werror -DYAIL_DLL -DBOOST_ASIO_DYN_LINK)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fvisibility-inlines-hidden")
set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -fvisibility-inlines-hidden")

# protobufs
add_subdirectory (messages)

# header paths
include_directories(
  ${PROJECT_SOURCE_DIR}
	${PROJECT_BINARY_DIR}
	${BOOST_INCLUDE_DIRS}
)

# library paths
link_directories(
  ${Boost_LIBRARY_DIRS}
)

# build
set (PUB_SOURCES
 pub.cpp
)

set (SUB_SOURCES
 sub.cpp
)

set (PUB_EXE hello_pub)
set (SUB_EXE hello_sub)

add_executable (${PUB_EXE} ${PUB_SOURCES})
target_link_libraries (${PUB_EXE} hello_messages ${PROJECT_NAME} ${boost_asio_LIBRARIES} ${Boost_LIBRARIES} ${PROTOBUF_LIBRARY} -lpthread)

add_executable (${SUB_EXE} ${SUB_SOURCES})
target_link_libraries (${SUB_EXE} hello_messages ${PROJECT_NAME} ${boost_asio_LIBRARIES} ${Boost_LIBRARIES} ${PROTOBUF_LIBRARY} -lpthread)

# install
install (TARGETS ${PUB_EXE} DESTINATION bin)
install (TARGETS ${SUB_EXE} DESTINATION bin)
