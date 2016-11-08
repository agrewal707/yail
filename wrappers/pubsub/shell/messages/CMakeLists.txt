#
# CMake configuration file for general protobuf message
#
set (LIB_NAME ${PROJECT_NAME}_shell_protobuf)
set (LIB_VERSION 0.0.1)
set (LIB_SOVERSION ${LIB_VERSION})

# proto files
file(GLOB ProtoFiles "${CMAKE_CURRENT_SOURCE_DIR}/*.proto")

# Generate code in build tree
PROTOBUF_GENERATE_CPP(ProtoSources ProtoHeaders ${ProtoFiles})

add_library(${LIB_NAME} SHARED ${ProtoSources} ${ProtoHeaders})

set_target_properties(${LIB_NAME} PROPERTIES
	VERSION ${LIB_VERSION}
	SOVERSION ${LIB_SOVERSION}
)
install (TARGETS ${LIB_NAME} DESTINATION lib)
install (FILES ${ProtoHeaders} DESTINATION include/yail/wrappers/pubsub/shell)
