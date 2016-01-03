#ifndef YAIL_RPC_DETAIL_SERVICE_IMPL_H
#define YAIL_RPC_DETAIL_SERVICE_IMPL_H

#include <yail/rpc/service.h>

#include <yail/log.h>
#include <yail/exception.h>
#include <yail/rpc/detail/client.h>
#include <yail/rpc/detail/server.h>

//
// yail::rpc::detail::service_impl
//
namespace yail {
namespace rpc {
namespace detail {

YAIL_DECLARE_EXCEPTION(duplicate_service);
YAIL_DECLARE_EXCEPTION(unknown_service);

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
		const auto it = m_service_map.find (service_name);
		if (it != m_service_map.end ())
		{
			YAIL_THROW_EXCEPTION (
				duplicate_service, "service already exists", 0);
		}
		
		m_service_map[service_name] = ep;
	}

	transport_endpoint get_service_location (const std::string &service_name) const
	{
		const auto it = m_service_map.find (service_name);
		if (it == m_service_map.end ())
		{
			// Check if transport suports implicit derivation of ep from service_name
			const auto result = transport_traits<Transport>::derive_ep_from_service_name (service_name);
			if (result.first)
			{
				return result.second;
			}
			else
			{
				YAIL_THROW_EXCEPTION (
					unknown_service, "service does not exist", 0);
			}
		}
		return m_service_map[service_name];
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
	client<Transport> m_client;
	server<Transport> m_server;
	using service_map = std::unordered_map<std::string, transport_endpoint>;
	service_map m_service_map;
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
	m_client (io_service, this, m_transport),
	m_server (io_service, this, m_transport)
{
	YAIL_LOG_TRACE (this);
}

template <typename Transport>
service_impl<Transport>::service_impl (yail::io_service &io_service, Transport &transport) :
	m_io_service (io_service),
	m_transport (transport),
	m_destroy_transport (false),
	m_client (io_service, this, m_transport),
	m_server (io_service, this, m_transport)
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
