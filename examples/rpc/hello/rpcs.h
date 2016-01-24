#include <yail/rpc/rpc_traits.h>

#include <examples/rpc/hello/messages/hello.pb.h>

REGISTER_RPC_TRAITS(messages::hello_request, messages::hello_response);
REGISTER_RPC_TRAITS(messages::bye_request, messages::bye_response);
