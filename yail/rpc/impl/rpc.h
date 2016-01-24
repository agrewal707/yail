#ifndef YAIL_RPC_IMPL_RPC_H
#define YAIL_RPC_IMPL_RPC_H

#include <yail/rpc/detail/rpc_impl.h>

namespace yail {
namespace rpc {

template <typename Request, typename Response>
rpc<Request, Response>::rpc (const std::string& name) :
	m_impl (yail::make_unique<impl_type> (name))
{}

template <typename Request, typename Response>
rpc<Request, Response>::~rpc ()
{}

} // namespace rpc
} // namespace yail

#endif // YAIL_RPC_IMPL_RPC_H
