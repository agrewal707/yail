#ifndef YAIL_RPC_DETAIL_SERVICE_IMPL_H
#define YAIL_RPC_DETAIL_SERVICE_IMPL_H

#include <yail/rpc/service.h>

#include <yail/log.h>
#include <yail/rpc/detail/client.h>
#include <yail/rpc/detail/server.h>
#include <yail/rpc/detail/service_locator.h>

//
// yail::rpc::detail::service_impl
//
namespace yail {
namespace rpc {
namespace detail {

template <typename Transport>
class service_impl
{
public:
	using transport_endpoint = typename Transport::endpoint;

	service_impl (yail::io_service &io_service);

	service_impl (yail::io_service &io_service, Transport &transport);

	~service_impl ();

	void set_service_location (const std::string &service_name, const transport_endpoint &ep)
	{
		m_service_locator.set_service_location (service_name, ep);
	}

	yail::io_service& get_io_service () 
	{
		return m_io_service;
	}

	client<Transport>& get_client ()
	{
		return m_client;
	}

	server<Transport>& get_server () 
	{
	 	return m_server;
	}

private:
	yail::io_service &m_io_service;
	Transport &m_transport;
	bool m_destroy_transport;
	service_locator<Transport> m_service_locator;
	client<Transport> m_client;
	server<Transport> m_server;
};

} // namespace detail
} // namespace rpc
} // namespace yail

namespace yail {
namespace rpc {
namespace detail {

template <typename Transport>
service_impl<Transport>::service_impl (yail::io_service &io_service) :
	m_io_service (io_service),
	m_transport (*(new Transport {io_service})),
	m_destroy_transport (true),
	m_client (m_service_locator, m_transport),
	m_server (m_service_locator, m_transport)
{
	YAIL_LOG_TRACE (this);
}

template <typename Transport>
service_impl<Transport>::service_impl (yail::io_service &io_service, Transport &transport) :
	m_io_service (io_service),
	m_transport (transport),
	m_destroy_transport (false),
	m_client (m_service_locator, m_transport),
	m_server (m_service_locator, m_transport)
{
	YAIL_LOG_TRACE (this);
}

template <typename Transport>
service_impl<Transport>::~service_impl ()
{
	YAIL_LOG_TRACE (this);

	if (m_destroy_transport)
		delete &m_transport;
}

} // namespace detail
} // namespace rpc
} // namespace yail

#endif // YAIL_RPC_DETAIL_SERVICE_IMPL_H
