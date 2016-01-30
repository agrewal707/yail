#ifndef YAIL_PUBSUB_DETAIL_SERVICE_IMPL_H
#define YAIL_PUBSUB_DETAIL_SERVICE_IMPL_H

#include <yail/pubsub/service.h>

#include <yail/log.h>
#include <yail/pubsub/detail/publisher.h>
#include <yail/pubsub/detail/subscriber.h>

//
// yail::pubsub::detail::service_impl
//
namespace yail {
namespace pubsub {
namespace detail {

template <typename Transport>
class service_impl
{
public:
	service_impl (yail::io_service &io_service, const std::string &domain);

	service_impl (yail::io_service &io_service, Transport &transport, const std::string &domain);

	~service_impl ();

	yail::io_service& get_io_service () 
	{
		return m_io_service;
	}

	publisher<Transport>& get_publisher ()
	{
		return m_publisher;
	}

	subscriber<Transport>& get_subscriber () 
	{
	 	return m_subscriber;
	}

private:
	yail::io_service &m_io_service;
	Transport &m_transport;
	bool m_destroy_transport;
	publisher<Transport> m_publisher;
	subscriber<Transport> m_subscriber;
};

} // namespace detail
} // namespace pubsub
} // namespace yail

namespace yail {
namespace pubsub {
namespace detail {

template <typename Transport>
service_impl<Transport>::service_impl (yail::io_service &io_service, const std::string &domain) :
	m_io_service (io_service),
	m_transport (*(new Transport {io_service})),
	m_destroy_transport (true),
	m_publisher (io_service, m_transport, domain),
	m_subscriber (io_service, m_transport, domain)
{
	YAIL_LOG_FUNCTION (this);
}

template <typename Transport>
service_impl<Transport>::service_impl (yail::io_service &io_service, Transport &transport, const std::string &domain) :
	m_io_service (io_service),
	m_transport (transport),
	m_destroy_transport (false),
	m_publisher (io_service, m_transport, domain),
	m_subscriber (io_service, m_transport, domain)
{
	YAIL_LOG_FUNCTION (this);
}

template <typename Transport>
service_impl<Transport>::~service_impl ()
{
	YAIL_LOG_FUNCTION (this);

	if (m_destroy_transport)
		delete &m_transport;
}

} // namespace detail
} // namespace pusub
} // namespace yail

#endif // YAIL_PUBSUB_DETAIL_SERVICE_IMPL_H
