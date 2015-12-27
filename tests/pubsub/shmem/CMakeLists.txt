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
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DYAIL_DEBUG -DYAIL_TRACE")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fvisibility-inlines-hidden")
set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -fvisibility-inlines-hidden")

# protobufs
add_subdirectory (messages)

# header paths
include_directories(
  ${PROJECT_SOURCE_DIR}
	${PROJECT_BINARY_DIR}
	${Boost_INCLUDE_DIRS}
)

# library paths
link_directories(
  ${Boost_LIBRARY_DIRS}
)

# build
set (PUBSUB_SOURCES
 pubsub.cpp
)

set (TEST_SOURCES
 test.cpp
)

set (PUBSUB_EXE pubsub_shmem)
set (TEST_EXE test_pubsub_shmem)

add_executable (${PUBSUB_EXE} ${PUBSUB_SOURCES})
target_link_libraries (${PUBSUB_EXE} shmem_messages ${PROJECT_NAME} ${boost_asio_LIBRARIES} ${Boost_LIBRARIES} ${PROTOBUF_LIBRARY} -lpthread)

add_executable (${TEST_EXE} ${TEST_SOURCES})
target_link_libraries (${TEST_EXE} ${Boost_LIBRARIES})

# install
install (TARGETS ${PUBSUB_EXE} DESTINATION bin)
install (TARGETS ${TEST_EXE} DESTINATION bin)
