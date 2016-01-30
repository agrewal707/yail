#
# CMake configuration file for test protobuf messages
#

# proto files
file(GLOB ProtoFiles "${CMAKE_CURRENT_SOURCE_DIR}/*.proto")

# Generate code in build tree
PROTOBUF_GENERATE_CPP(ProtoSources ProtoHeaders ${ProtoFiles})

add_library(rpc_unix_domain_messages STATIC ${ProtoSources} ${ProtoHeaders})
target_include_directories(rpc_unix_domain_messages PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
