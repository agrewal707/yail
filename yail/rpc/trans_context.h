#ifndef YAIL_RPC_TRANS_CONTEXT_H
#define YAIL_RPC_TRANS_CONTEXT_H

namespace yail {
namespace rpc {
namespace detail {

class trans_context_impl;

} // namespace detail
} // namespace rpc
} // namespace yail

//
// yail::rpc:context
//
namespace yail {
namespace rpc {

using trans_context = detail::trans_context_impl;

} // namespace rpc
} // namespace yail

#endif // YAIL_RPC_TRANS_CONTEXT_H
