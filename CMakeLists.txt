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
set (YAIL_PUBSUB_SHMEM_RECEIVER_QUEUE_DEPTH 25)
set (YAIL_PUBSUB_SHMEM_RECEIVER_BUFFER_QUEUE_DEPTH 1000)
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

# yail wrappers
add_subdirectory (wrappers)

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
pubsub_udp_async_singlethreaded
test_pubsub_udp
"--num-writers 1 --num-readers 5 --num-msgs 100 --data-size 1024"
"pubsub_udp1, writer0, sent:100
pubsub_udp1, reader0, rcvd:100, dropped:0, valid:100
pubsub_udp1, reader1, rcvd:100, dropped:0, valid:100
pubsub_udp1, reader2, rcvd:100, dropped:0, valid:100
pubsub_udp1, reader3, rcvd:100, dropped:0, valid:100
pubsub_udp1, reader4, rcvd:100, dropped:0, valid:100

pubsub_udp2, reader0, rcvd:100, dropped:0, valid:100
pubsub_udp2, reader1, rcvd:100, dropped:0, valid:100
pubsub_udp2, reader2, rcvd:100, dropped:0, valid:100
pubsub_udp2, reader3, rcvd:100, dropped:0, valid:100
pubsub_udp2, reader4, rcvd:100, dropped:0, valid:100"
)

do_test (
pubsub_udp_sync_multithreaded
test_pubsub_udp
"--num-writers 1 --num-readers 5 --num-msgs 100 --data-size 1024 --multithreaded"
"pubsub_udp1, writer0, sent:100
pubsub_udp1, reader0, rcvd:100, dropped:0, valid:100
pubsub_udp1, reader1, rcvd:100, dropped:0, valid:100
pubsub_udp1, reader2, rcvd:100, dropped:0, valid:100
pubsub_udp1, reader3, rcvd:100, dropped:0, valid:100
pubsub_udp1, reader4, rcvd:100, dropped:0, valid:100

pubsub_udp2, reader0, rcvd:100, dropped:0, valid:100
pubsub_udp2, reader1, rcvd:100, dropped:0, valid:100
pubsub_udp2, reader2, rcvd:100, dropped:0, valid:100
pubsub_udp2, reader3, rcvd:100, dropped:0, valid:100
pubsub_udp2, reader4, rcvd:100, dropped:0, valid:100"
)
endif(YAIL_PUBSUB_ENABLE_UDP_TRANSPORT)

# yail pubsub tests on shared memory transport
if (YAIL_PUBSUB_ENABLE_SHMEM_TRANSPORT)
do_test (
pubsub_shmem_async_singlethreaded
test_pubsub_shmem
"--num-writers 5 --num-readers 5 --num-msgs 200 --data-size 1024"
"pubsub_shmem1, writer0, sent:200
pubsub_shmem1, writer1, sent:200
pubsub_shmem1, writer2, sent:200
pubsub_shmem1, writer3, sent:200
pubsub_shmem1, writer4, sent:200
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

do_test (
pubsub_shmem_sync_multithreaded
test_pubsub_shmem
"--num-writers 5 --num-readers 5 --num-msgs 200 --data-size 1024 --multithreaded"
"pubsub_shmem1, writer0, sent:200
pubsub_shmem1, writer1, sent:200
pubsub_shmem1, writer2, sent:200
pubsub_shmem1, writer3, sent:200
pubsub_shmem1, writer4, sent:200
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


# yail rpc tests on unix domain transport
if (YAIL_RPC_ENABLE_UNIX_DOMAIN_TRANSPORT)
do_test (
rpc_unix_domain_sync_call_reply_ok
test_rpc_unix_domain
"--num-clients 5 --num-calls 5 --call-type 0 --reply-type 0"
"rpc_unix_domain1, provider0, call:25, reply-ok:25, reply-delayed:0, reply-error:0, reply-none:0, valid:25

rpc_unix_domain2, client0, call:5, reply-ok:5, reply-error:0, reply-timeout:0, valid:5
rpc_unix_domain2, client1, call:5, reply-ok:5, reply-error:0, reply-timeout:0, valid:5
rpc_unix_domain2, client2, call:5, reply-ok:5, reply-error:0, reply-timeout:0, valid:5
rpc_unix_domain2, client3, call:5, reply-ok:5, reply-error:0, reply-timeout:0, valid:5
rpc_unix_domain2, client4, call:5, reply-ok:5, reply-error:0, reply-timeout:0, valid:5"
)
do_test (
rpc_unix_domain_async_call_reply_ok
test_rpc_unix_domain
"--num-clients 5 --num-calls 5 --call-type 1 --reply-type 0"
"rpc_unix_domain1, provider0, call:25, reply-ok:25, reply-delayed:0, reply-error:0, reply-none:0, valid:25

rpc_unix_domain2, client0, call:5, reply-ok:5, reply-error:0, reply-timeout:0, valid:5
rpc_unix_domain2, client1, call:5, reply-ok:5, reply-error:0, reply-timeout:0, valid:5
rpc_unix_domain2, client2, call:5, reply-ok:5, reply-error:0, reply-timeout:0, valid:5
rpc_unix_domain2, client3, call:5, reply-ok:5, reply-error:0, reply-timeout:0, valid:5
rpc_unix_domain2, client4, call:5, reply-ok:5, reply-error:0, reply-timeout:0, valid:5"
)
do_test (
rpc_unix_domain_sync_call_reply_delayed
test_rpc_unix_domain
"--num-clients 2 --num-calls 2 --call-type 0 --reply-type 1"
"rpc_unix_domain1, provider0, call:4, reply-ok:4, reply-delayed:4, reply-error:0, reply-none:0, valid:4

rpc_unix_domain2, client0, call:2, reply-ok:2, reply-error:0, reply-timeout:0, valid:2
rpc_unix_domain2, client1, call:2, reply-ok:2, reply-error:0, reply-timeout:0, valid:2"
)
do_test (
rpc_unix_domain_async_call_reply_delayed
test_rpc_unix_domain
"--num-clients 2 --num-calls 2 --call-type 1 --reply-type 1"
"rpc_unix_domain1, provider0, call:4, reply-ok:4, reply-delayed:4, reply-error:0, reply-none:0, valid:4

rpc_unix_domain2, client0, call:2, reply-ok:2, reply-error:0, reply-timeout:0, valid:2
rpc_unix_domain2, client1, call:2, reply-ok:2, reply-error:0, reply-timeout:0, valid:2"
)
do_test (
rpc_unix_domain_sync_call_reply_error
test_rpc_unix_domain
"--num-clients 5 --num-calls 5 --call-type 0 --reply-type 2"
"rpc_unix_domain1, provider0, call:25, reply-ok:0, reply-delayed:0, reply-error:25, reply-none:0, valid:25

rpc_unix_domain2, client0, call:5, reply-ok:0, reply-error:5, reply-timeout:0, valid:0
rpc_unix_domain2, client1, call:5, reply-ok:0, reply-error:5, reply-timeout:0, valid:0
rpc_unix_domain2, client2, call:5, reply-ok:0, reply-error:5, reply-timeout:0, valid:0
rpc_unix_domain2, client3, call:5, reply-ok:0, reply-error:5, reply-timeout:0, valid:0
rpc_unix_domain2, client4, call:5, reply-ok:0, reply-error:5, reply-timeout:0, valid:0"
)
do_test (
rpc_unix_domain_async_call_reply_error
test_rpc_unix_domain
"--num-clients 5 --num-calls 5 --call-type 1 --reply-type 2"
"rpc_unix_domain1, provider0, call:25, reply-ok:0, reply-delayed:0, reply-error:25, reply-none:0, valid:25

rpc_unix_domain2, client0, call:5, reply-ok:0, reply-error:5, reply-timeout:0, valid:0
rpc_unix_domain2, client1, call:5, reply-ok:0, reply-error:5, reply-timeout:0, valid:0
rpc_unix_domain2, client2, call:5, reply-ok:0, reply-error:5, reply-timeout:0, valid:0
rpc_unix_domain2, client3, call:5, reply-ok:0, reply-error:5, reply-timeout:0, valid:0
rpc_unix_domain2, client4, call:5, reply-ok:0, reply-error:5, reply-timeout:0, valid:0"
)
do_test (
rpc_unix_domain_sync_call_timeout_reply_ok
test_rpc_unix_domain
"--num-clients 3 --num-calls 1 --call-type 0 --reply-type 0 --timeout 1"
"rpc_unix_domain1, provider0, call:3, reply-ok:3, reply-delayed:0, reply-error:0, reply-none:0, valid:3

rpc_unix_domain2, client0, call:1, reply-ok:1, reply-error:0, reply-timeout:0, valid:1
rpc_unix_domain2, client1, call:1, reply-ok:1, reply-error:0, reply-timeout:0, valid:1
rpc_unix_domain2, client2, call:1, reply-ok:1, reply-error:0, reply-timeout:0, valid:1"
)
do_test (
rpc_unix_domain_sync_call_timeout_reply_none
test_rpc_unix_domain
"--num-clients 3 --num-calls 1 --call-type 0 --reply-type 3 --timeout 1"
"rpc_unix_domain1, provider0, call:3, reply-ok:0, reply-delayed:0, reply-error:0, reply-none:3, valid:3

rpc_unix_domain2, client0, call:1, reply-ok:0, reply-error:0, reply-timeout:1, valid:0
rpc_unix_domain2, client1, call:1, reply-ok:0, reply-error:0, reply-timeout:1, valid:0
rpc_unix_domain2, client2, call:1, reply-ok:0, reply-error:0, reply-timeout:1, valid:0"
)
endif(YAIL_RPC_ENABLE_UNIX_DOMAIN_TRANSPORT)

