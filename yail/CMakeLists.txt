#
# CMake configuration file for YAIL library
#

# dependencies
find_package (Boost REQUIRED system)
find_package (Protobuf REQUIRED)

# options header file
configure_file (
  "${PROJECT_SOURCE_DIR}/yail/config.h.in"
  "${PROJECT_BINARY_DIR}/yail/config.h"
)

# compiler flags
add_definitions (-std=c++11 -Werror -DYAIL_DLL -DYAIL_DLL_EXPORTS -DBOOST_ASIO_DYN_LINK -DBOOST_NO_AUTO_PTR)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DYAIL_DEBUG -DYAIL_TRACE")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fvisibility=hidden -fvisibility-inlines-hidden")
set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -fvisibility=hidden -fvisibility-inlines-hidden")

MESSAGE(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
MESSAGE(STATUS "None: ${CMAKE_CXX_FLAGS}")
MESSAGE(STATUS "Debug: ${CMAKE_CXX_FLAGS_DEBUG}")
MESSAGE(STATUS "Release: ${CMAKE_CXX_FLAGS_RELEASE}")
MESSAGE(STATUS "RelWithDebInfo: ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
MESSAGE(STATUS "MinSizeRel: ${CMAKE_CXX_FLAGS_MINSIZEREL}")

MESSAGE(STATUS "boost include: ${Boost_INCLUDE_DIRS}")
MESSAGE(STATUS "boost libs: ${Boost_LIBRARIES}")

# internal sub libraries
if(YAIL_PUBSUB_ENABLE)
add_subdirectory (pubsub/detail/messages)
endif(YAIL_PUBSUB_ENABLE)

if(YAIL_RPC_ENABLE)
add_subdirectory (rpc/detail/messages)
endif(YAIL_RPC_ENABLE)

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
file (GLOB YAIL_SOURCES impl/*.cpp)

file (GLOB YAIL_PUBSUB_SOURCES pubsub/impl/*.cpp  pubsub/detail/impl/*.cpp)
file (GLOB YAIL_PUBSUB_UDP_TRANSPORT_SOURCES pubsub/transport/impl/udp*.cpp  pubsub/transport/detail/impl/udp*.cpp)
file (GLOB YAIL_PUBSUB_SHMEM_TRANSPORT_SOURCES pubsub/transport/impl/shmem*.cpp  pubsub/transport/detail/impl/shmem*.cpp)

file (GLOB YAIL_RPC_SOURCES rpc/impl/*.cpp  rpc/detail/impl/*.cpp)
file (GLOB YAIL_RPC_UNIX_DOMAIN_TRANSPORT_SOURCES rpc/transport/impl/unix_domain*.cpp  rpc/transport/detail/impl/unix_domain*.cpp)

if(YAIL_PUBSUB_ENABLE)
	list(APPEND YAIL_SOURCES ${YAIL_PUBSUB_SOURCES})
	list(APPEND YAIL_MESSAGES_LIBS ${PROJECT_NAME}_pubsub_messages)
endif(YAIL_PUBSUB_ENABLE)

if(YAIL_PUBSUB_ENABLE_UDP_TRANSPORT)
	list(APPEND YAIL_SOURCES ${YAIL_PUBSUB_UDP_TRANSPORT_SOURCES})
endif(YAIL_PUBSUB_ENABLE_UDP_TRANSPORT)

if(YAIL_PUBSUB_ENABLE_SHMEM_TRANSPORT)
	list(APPEND YAIL_SOURCES ${YAIL_PUBSUB_SHMEM_TRANSPORT_SOURCES})
endif(YAIL_PUBSUB_ENABLE_SHMEM_TRANSPORT)

if(YAIL_RPC_ENABLE)
	list(APPEND YAIL_SOURCES ${YAIL_RPC_SOURCES})
	list(APPEND YAIL_MESSAGES_LIBS ${PROJECT_NAME}_rpc_messages)
endif(YAIL_RPC_ENABLE)

if(YAIL_RPC_ENABLE_UNIX_DOMAIN_TRANSPORT)
	list(APPEND YAIL_SOURCES ${YAIL_RPC_UNIX_DOMAIN_TRANSPORT_SOURCES})
endif(YAIL_RPC_ENABLE_UNIX_DOMAIN_TRANSPORT)

MESSAGE(STATUS "messages libs: ${YAIL_MESSAGES_LIBS}")

add_library(${PROJECT_NAME} ${YAIL_SOURCES})
target_link_libraries (${PROJECT_NAME} ${YAIL_MESSAGES_LIBS} ${Boost_LIBRARIES} -lrt)

# install lib
set_target_properties(${PROJECT_NAME} PROPERTIES
	VERSION ${${PROJECT_NAME}_VERSION}
	SOVERSION ${${PROJECT_NAME}_SOVERSION}
)
install (TARGETS ${PROJECT_NAME} DESTINATION lib)

# install headers
set (YAIL_HEADER_DIRS ${PROJECT_SOURCE_DIR}/yail ${PROJECT_BINARY_DIR}/yail)

install (DIRECTORY ${YAIL_HEADER_DIRS}
	DESTINATION include FILES_MATCHING PATTERN "*.h"
)
