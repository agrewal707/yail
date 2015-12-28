YAIL: Yet Another IPC Library
----

YAIL is a C++ library that provides publish/subscribe (PUBSUB) and remote procedure call (RPC) functionality.

The PUBSUB functionality is loosely based on concepts from Data Distribution 
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
TODO

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
$ git clone git@github.com:agrewal707/yail.git
$ mkdir -p build/yail
$ cd build/yail
$ cmake -DYAIL_PUBSUB_ENABLE_UDP_TRANSPORT=on -DCMAKE_INSTALL_PREFIX=local -DCMAKE_BUILD_TYPE=MinSizeRel ../../yail
$ make install/strip
```

Tests
-----
```
$ LD_LIBRARY_PATH=local/lib make test
Running tests...
Test project /home/agrewal/projects/build/yail
    Start 1: pubsub_udp_1
1/4 Test #1: pubsub_udp_1 .....................   Passed    1.04 sec
    Start 2: pubsub_udp_2
2/4 Test #2: pubsub_udp_2 .....................   Passed    1.16 sec
    Start 3: pubsub_shmem_1
3/4 Test #3: pubsub_shmem_1 ...................   Passed    1.04 sec
    Start 4: pubsub_shmem_2
4/4 Test #4: pubsub_shmem_2 ...................   Passed    1.17 sec

100% tests passed, 0 tests failed out of 4

Total Test time (real) =   4.41 sec
```

Documentation
-------------
```
$ make doc
```

TODO
----
1. Implement RPC functionality.
