#ifndef YAIL_RPC_IMPL_SERVICE_H
#define YAIL_RPC_IMPL_SERVICE_H

#include <yail/rpc/detail/service_impl.h>

#include <yail/log.h>

namespace yail {
namespace rpc {

template <typename Transport>
service<Transport>::service (yail::io_service &io_service) :
	m_impl (yail::make_unique<impl_type> (io_service))
{
	YAIL_LOG_TRACE (this);
}

template <typename Transport>
service<Transport>::service (yail::io_service &io_service, Transport &transport) :
	m_impl (make_unique<impl_type> (io_service, transport))
{
	YAIL_LOG_TRACE (this);
}

template <typename Transport>
service<Transport>::~service ()
{
	YAIL_LOG_TRACE (this);
}

void service<Transport>::set_service_location (const std::string &service_name, const transport_endpoint &ep)
{
	m_impl->set_service_location (service_name, ep);
}

} // namespace rpc
} // namespace yail

#endif // YAIL_RPC_IMPL_SERVICE_H
