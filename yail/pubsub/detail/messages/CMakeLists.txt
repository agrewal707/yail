#
# CMake configuration file for pubsub protobuf messages
#

# proto files
file(GLOB ProtoFiles "${CMAKE_CURRENT_SOURCE_DIR}/*.proto")

# compile flags
add_definitions (-fPIC)

# Generate code in build tree
PROTOBUF_GENERATE_CPP(ProtoSources ProtoHeaders ${ProtoFiles})

add_library(${PROJECT_NAME}_messages STATIC ${ProtoSources} ${ProtoHeaders})
target_include_directories(${PROJECT_NAME}_messages PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
