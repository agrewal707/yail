#include <yail/rpc/rpc_traits.h>

#include <tests/rpc/unix_domain/messages/hello.pb.h>

REGISTER_RPC_TRAITS(messages::hello_request, messages::hello_response);
