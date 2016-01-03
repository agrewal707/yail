#ifndef YAIL_RPC_TRANSPORT_IMPL_UNIX_DOMAIN_H
#define YAIL_RPC_TRANSPORT_IMPL_UNIX_DOMAIN_H

#include <yail/rpc/transport/detail/unix_domain_impl.h>
#include <yail/rpc/transport_traits.h>

namespace yail {
namespace rpc {
namespace transport {

template <>
struct transport_traits<unix_domain>
{
	std::pair<bool, Transport::endpoint> 
	derive_ep_from_service_name (const std::string &service_name)
	{
		return std::make_pair (true, Transport::endpoint ("/var/run"+service_name));
	}
}

template <typename Handler>
void unix_domain::async_client_send_n_receive (const endpoint &ep,
	const yail::buffer &req_buffer, yail::buffer &res_buffer, const Handler &handler)
{
	m_impl->async_client_send_n_receive (ep, req_buffer, res_buffer, handler);
}

} // namespace transport
} // namespace rpc
} // namespace yail

#endif // YAIL_RPC_TRANSPORT_IMPL_UNIX_DOMAIN_H
