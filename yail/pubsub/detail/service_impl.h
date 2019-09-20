#ifndef YAIL_PUBSUB_DETAIL_SERVICE_IMPL_H
#define YAIL_PUBSUB_DETAIL_SERVICE_IMPL_H

#include <yail/pubsub/service.h>

#include <yail/log.h>
#include <yail/pubsub/detail/publisher.h>
#include <yail/pubsub/detail/subscriber.h>
#include <yail/pubsub/detail/data_writer_impl.h>
#include <yail/pubsub/detail/data_reader_impl.h>
#include <yail/pubsub/topic_traits.h>
#include <yail/pubsub/detail/messages/pubsub.pb.h>

REGISTER_BUILTIN_TOPIC_TRAITS(yail::pubsub::detail::messages::subscription);

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
	void read_sub ();
	void write_sub (const messages::subscription &sub);

	yail::io_service &m_io_service;
	Transport &m_transport;
	bool m_destroy_transport;
	publisher<Transport> m_publisher;
	subscriber<Transport> m_subscriber;
	topic_impl<messages::subscription> m_sub_topic;
	data_writer_impl<messages::subscription, Transport> m_sub_dw;
	data_reader_impl<messages::subscription, Transport> m_sub_dr;
	messages::subscription m_sub;
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
	m_subscriber (io_service, m_transport, domain, std::bind (&service_impl<Transport>::write_sub, this, std::placeholders::_1)),
	m_sub_topic ("__YAIL_INTERNAL_SUBSCRIPTION__"),
	m_sub_dw (*this, m_sub_topic),
	m_sub_dr (*this, m_sub_topic)
{
	YAIL_LOG_FUNCTION (this);

	read_sub ();
}

template <typename Transport>
service_impl<Transport>::service_impl (yail::io_service &io_service, Transport &transport, const std::string &domain) :
	m_io_service (io_service),
	m_transport (transport),
	m_destroy_transport (false),
	m_publisher (io_service, m_transport, domain),
	m_subscriber (io_service, m_transport, domain, std::bind (&service_impl<Transport>::write_sub, this, std::placeholders::_1)),
	m_sub_topic ("__YAIL_INTERNAL_SUBSCRIPTION__"),
	m_sub_dw (*this, m_sub_topic),
	m_sub_dr (*this, m_sub_topic)
{
	YAIL_LOG_FUNCTION (this);

	read_sub ();
}

template <typename Transport>
service_impl<Transport>::~service_impl ()
{
	YAIL_LOG_FUNCTION (this);

	m_sub_dr.shutdown ();
	m_sub_dw.shutdown ();

	if (m_destroy_transport)
		delete &m_transport;
}

template <typename Transport>
void service_impl<Transport>::read_sub ()
{
	m_sub_dr.async_read (m_sub,
		[this] (const boost::system::error_code &ec)
		{
			if (!ec)
			{
				YAIL_LOG_TRACE (
					"domain: " << m_sub.domain () << ", " <<
					"name: " << m_sub.topic_name () << ", " <<
					"type: " << m_sub.topic_type_name () << ", ");

				m_publisher.notify (m_sub);
			}
			else if (ec != io_service_error::operation_aborted)
			{
				YAIL_LOG_ERROR ("failed to read subscription :" << ec);
			}

			m_sub.Clear ();

			read_sub ();
		});
}

template <typename Transport>
void service_impl<Transport>::write_sub (const messages::subscription &sub)
{
	m_sub_dw.async_write (sub,
		[this] (const boost::system::error_code &ec)
		{
			if (ec && ec != io_service_error::operation_aborted)
			{
				YAIL_LOG_WARNING("failed to notify publisher");
			}
		});
}

} // namespace detail
} // namespace pusub
} // namespace yail

#endif // YAIL_PUBSUB_DETAIL_SERVICE_IMPL_H
