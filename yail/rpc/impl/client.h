#ifndef YAIL_RPC_IMPL_CLIENT_H
#define YAIL_RPC_IMPL_CLIENT_H

#include <yail/rpc/detail/client_impl.h>

namespace yail {
namespace rpc {

template <typename Transport>
client<Transport>::client (service<Transport> &service) :
	m_impl (make_unique<impl_type> (service.get_impl ()))
{
	YAIL_LOG_FUNCTION (this);
}

template <typename Transport>
client<Transport>::~client ()
{
	YAIL_LOG_FUNCTION (this);
}

template <typename Transport> 
template <typename Request, typename Response>
void client<Transport>::call (const std::string& service_name, const rpc<Request, Response> &service_rpc, 
                              const Request &req, Response &res, boost::system::error_code &ec, const uint32_t timeout)
{
	m_impl->call (service_name, service_rpc.get_impl (), req, res, ec, timeout);
}

template <typename Transport> 
template <typename Request, typename Response, typename Handler>
void client<Transport>::async_call (const std::string& service_name, const rpc<Request, Response> &service_rpc, 
	                                  const Request &req, Response &res, const Handler &handler)
{
	m_impl->async_call (service_name, service_rpc.get_impl (), req, res, handler);
}

} // namespace rpc
} // namespace yail

#endif // YAIL_RPC_IMPL_CLIENT_H
