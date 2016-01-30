#ifndef YAIL_RPC_TRANSPORT_IMPL_UNIX_DOMAIN_H
#define YAIL_RPC_TRANSPORT_IMPL_UNIX_DOMAIN_H

#include <yail/rpc/transport/detail/unix_domain_impl.h>
#include <yail/rpc/transport/traits.h>

namespace yail {
namespace rpc {
namespace transport {

template <>
struct traits<unix_domain>
{
	std::pair<bool, unix_domain::endpoint> 
	static service_name_to_ep (const std::string &service_name)
	{
		return std::make_pair (true, unix_domain::endpoint ("/var/run/"+service_name));
	}
};

template <typename Handler>
void unix_domain::async_client_send_n_receive (const endpoint &ep,
	const yail::buffer &req_buffer, yail::buffer &res_buffer, const Handler &handler)
{
	m_impl->async_client_send_n_receive (ep, req_buffer, res_buffer, handler);
}

template <typename Handler>
void unix_domain::server_set_receive_handler (const Handler &handler)
{
	m_impl->server_set_receive_handler (handler);
}

} // namespace transport
} // namespace rpc
} // namespace yail

#endif // YAIL_RPC_TRANSPORT_IMPL_UNIX_DOMAIN_H
