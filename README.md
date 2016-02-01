YAIL: Yet Another IPC Library
----

YAIL is a C++ library that provides publish/subscribe (PUBSUB) and remote procedure call (RPC) functionality with a strongly typed templated API.

The PUBSUB functionality is based on concepts from Data Distribution 
Service (DDS) standard from Object Management Group (OMG). YAIL separates the IPC 
messaging layer from the underlying transport enabling user to instantiate 
YAIL pubsub or rpc service with specific transport at compile time. 

PUBSUB
------
YAIL library currently provides following transports for PUBSUB service:
- UDP multicast
- Shared Memory

![Atl text](/docs/yail_pubsub_arch.jpg?raw=true "Optional Title")

RPC
---
YAIL library currently provides following transports for RPC service:
- UNIX Domain

![Atl text](/docs/yail_rpc_arch.jpg?raw=true "Optional Title")

Dependencies
------------

YAIL depends on following libraries:
- Standard C++
- Boost ASIO 
- Google protobuf
- Boost Interprocess (only if shared memory transport is enabled)
- POSIX threads (only if shared memory transport is enabled)

Build and Installation
-------------------

```
$ cd ~/projects
$ git clone git://github.com/agrewal707/yail.git
$ mkdir -p build/yail
$ cd build/yail
$ cmake -DYAIL_PUBSUB_ENABLE_UDP_TRANSPORT=on -DCMAKE_INSTALL_PREFIX=local -DCMAKE_BUILD_TYPE=MinSizeRel ../../yail
$ make install/strip
```

Tests
-----
```
$ sudo LD_LIBRARY_PATH=local/lib make test
Running tests...
Test project /home/agrewal/projects/build/yail
      Start  1: pubsub_udp_1
 1/10 Test  #1: pubsub_udp_1 ...............................   Passed    1.04 sec
      Start  2: pubsub_udp_2
 2/10 Test  #2: pubsub_udp_2 ...............................   Passed    1.15 sec
      Start  3: pubsub_shmem_1
 3/10 Test  #3: pubsub_shmem_1 .............................   Passed    1.04 sec
      Start  4: pubsub_shmem_2
 4/10 Test  #4: pubsub_shmem_2 .............................   Passed    1.15 sec
      Start  5: rpc_unix_domain_sync_call_reply_ok
 5/10 Test  #5: rpc_unix_domain_sync_call_reply_ok .........   Passed    1.04 sec
      Start  6: rpc_unix_domain_async_call_reply_ok
 6/10 Test  #6: rpc_unix_domain_async_call_reply_ok ........   Passed    1.04 sec
      Start  7: rpc_unix_domain_sync_call_reply_delayed
 7/10 Test  #7: rpc_unix_domain_sync_call_reply_delayed ....   Passed    5.04 sec
      Start  8: rpc_unix_domain_async_call_reply_delayed
 8/10 Test  #8: rpc_unix_domain_async_call_reply_delayed ...   Passed    2.04 sec
      Start  9: rpc_unix_domain_sync_call_reply_error
 9/10 Test  #9: rpc_unix_domain_sync_call_reply_error ......   Passed    1.04 sec
      Start 10: rpc_unix_domain_async_call_reply_error
10/10 Test #10: rpc_unix_domain_async_call_reply_error .....   Passed    1.04 sec

100% tests passed, 0 tests failed out of 10

Total Test time (real) =  15.64 sec
```

Documentation
-------------
```
$ make doc
```
