cmake_minimum_required (VERSION 2.8.12)

if (YAIL_PUBSUB_ENABLE_SHMEM_TRANSPORT)
add_subdirectory (pubsub/hello)
endif(YAIL_PUBSUB_ENABLE_SHMEM_TRANSPORT)

if (YAIL_RPC_ENABLE_UNIX_DOMAIN_TRANSPORT)
add_subdirectory (rpc/hello)
endif(YAIL_RPC_ENABLE_UNIX_DOMAIN_TRANSPORT)
