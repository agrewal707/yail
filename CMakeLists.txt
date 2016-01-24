#
# CMake configuration file for YAIL
#
cmake_minimum_required (VERSION 2.8.12)
project (yail)

# project version
set (${PROJECT_NAME}_VERSION 1.0.0)
set (${PROJECT_NAME}_SOVERSION ${${PROJECT_NAME}_VERSION})

# options
option (YAIL_USES_BOOST_ASIO
	"Use boost::asio as yail::io_sevice" ON)
option (YAIL_BUILD_BOOST_ASIO_LIBRARY
	"Build boost asio as standlone shared library" ON)

option (YAIL_PUBSUB_ENABLE
	"Enable pubsub support in the library" ON)
option (YAIL_PUBSUB_ENABLE_SHMEM_TRANSPORT
	"Enable shmem transport for pubsub service" ON)
option (YAIL_PUBSUB_ENABLE_UDP_TRANSPORT
	"Enable udp transport for pubsub service" OFF)

option (YAIL_RPC_ENABLE
	"Enable rpc support in the library" ON)
option (YAIL_RPC_ENABLE_UNIX_DOMAIN_TRANSPORT
	"Enable udp transport for pubsub service" ON)

Option (BUILD_SHARED_LIBS
	"Set to OFF to build static libraries" ON)

# constants 
set (YAIL_PUBSUB_MAX_MSG_SIZE 2048)
set (YAIL_PUBSUB_SHMEM_SEGMENT_SIZE 65535)
set (YAIL_PUBSUB_SHMEM_RECEIVER_QUEUE_DEPTH 15)
set (YAIL_RPC_MAX_MSG_SIZE 2048)

# external dependencies
if (YAIL_BUILD_BOOST_ASIO_LIBRARY)
	add_subdirectory (contrib/boost_asio)
endif (YAIL_BUILD_BOOST_ASIO_LIBRARY)

# yail library
add_subdirectory (yail)

# yail examples
add_subdirectory (examples)

# yail documentation
add_subdirectory (docs)

# yail tests
add_subdirectory (tests)

enable_testing ()

macro (do_test test_name test_exec arg result)
	string(REPLACE " " ";" arg_list ${arg})
	add_test (NAME ${test_name} COMMAND ${test_exec} ${arg_list})
  set_tests_properties (${test_name}
    PROPERTIES PASS_REGULAR_EXPRESSION ${result})
endmacro (do_test)

# yail pubsub tests on udp transport
if (YAIL_PUBSUB_ENABLE_UDP_TRANSPORT)
do_test (
pubsub_udp_1
test_pubsub_udp
"--num-readers 1 --num-msgs 1 --data-size 1"
"pubsub_udp1, writer0, sent:1
pubsub_udp1, reader0, rcvd:1, dropped:0, valid:1

pubsub_udp2, reader0, rcvd:1, dropped:0, valid:1"
)

do_test (
pubsub_udp_2
test_pubsub_udp
"--num-readers 5 --num-msgs 1000 --data-size 1024"
"pubsub_udp1, writer0, sent:1000
pubsub_udp1, reader0, rcvd:1000, dropped:0, valid:1000
pubsub_udp1, reader1, rcvd:1000, dropped:0, valid:1000
pubsub_udp1, reader2, rcvd:1000, dropped:0, valid:1000
pubsub_udp1, reader3, rcvd:1000, dropped:0, valid:1000
pubsub_udp1, reader4, rcvd:1000, dropped:0, valid:1000

pubsub_udp2, reader0, rcvd:1000, dropped:0, valid:1000
pubsub_udp2, reader1, rcvd:1000, dropped:0, valid:1000
pubsub_udp2, reader2, rcvd:1000, dropped:0, valid:1000
pubsub_udp2, reader3, rcvd:1000, dropped:0, valid:1000
pubsub_udp2, reader4, rcvd:1000, dropped:0, valid:1000"
)
endif(YAIL_PUBSUB_ENABLE_UDP_TRANSPORT)

# yail pubsub tests on shared memory transport
if (YAIL_PUBSUB_ENABLE_SHMEM_TRANSPORT)
do_test (
pubsub_shmem_1
test_pubsub_shmem
"--num-readers 1 --num-msgs 1 --data-size 1"
"pubsub_shmem1, writer0, sent:1
pubsub_shmem1, reader0, rcvd:1, dropped:0, valid:1

pubsub_shmem2, reader0, rcvd:1, dropped:0, valid:1"
)

do_test (
pubsub_shmem_2
test_pubsub_shmem
"--num-readers 5 --num-msgs 1000 --data-size 1024"
"pubsub_shmem1, writer0, sent:1000
pubsub_shmem1, reader0, rcvd:1000, dropped:0, valid:1000
pubsub_shmem1, reader1, rcvd:1000, dropped:0, valid:1000
pubsub_shmem1, reader2, rcvd:1000, dropped:0, valid:1000
pubsub_shmem1, reader3, rcvd:1000, dropped:0, valid:1000
pubsub_shmem1, reader4, rcvd:1000, dropped:0, valid:1000

pubsub_shmem2, reader0, rcvd:1000, dropped:0, valid:1000
pubsub_shmem2, reader1, rcvd:1000, dropped:0, valid:1000
pubsub_shmem2, reader2, rcvd:1000, dropped:0, valid:1000
pubsub_shmem2, reader3, rcvd:1000, dropped:0, valid:1000
pubsub_shmem2, reader4, rcvd:1000, dropped:0, valid:1000"
)
endif(YAIL_PUBSUB_ENABLE_SHMEM_TRANSPORT)
