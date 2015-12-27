#ifndef YAIL_PUBSUB_DATA_WRITER_IMPL_H
#define YAIL_PUBSUB_DATA_WRITER_IMPL_H

#include <yail/pubsub/data_writer.h>

#include <yail/pubsub/error.h>
#include <yail/pubsub/detail/publisher.h>
#include <yail/pubsub/detail/service_impl.h>
#include <yail/pubsub/detail/topic_impl.h>

namespace yail {
namespace pubsub {
namespace detail {

template <typename T, typename Transport>
class data_writer_impl
{
public:
	data_writer_impl (service_impl<Transport> &service, topic_impl<T> &topic);
	~data_writer_impl ();

	template <typename Handler>
	void async_write (const T &t, const Handler &h);

private:
	template <typename Handler>
	struct write_operation
	{
		write_operation (const Handler &handler);
		~write_operation ();

		Handler m_handler;
	};

	service_impl<Transport> &m_service;
	topic_impl<T> &m_topic;
	std::string m_topic_id;
};

} // namespace detail
} // namespace pubsub
} // namespace yail

namespace yail {
namespace pubsub {
namespace detail {

//
// data_writer_impl::write_operation
//
template <typename T, typename Transport>
template <typename Handler>
data_writer_impl<T, Transport>::write_operation<Handler>::write_operation (const Handler &handler) :
	m_handler (handler)
{}

template <typename T, typename Transport>
template <typename Handler>
data_writer_impl<T, Transport>::write_operation<Handler>::~write_operation ()
{}

//
// data_writer_impl
//
template <typename T, typename Transport>
data_writer_impl<T, Transport>::data_writer_impl (
	service_impl<Transport> &service, topic_impl<T> &topic) :
	m_service (service),
	m_topic (topic),
	m_topic_id ()
{
	m_service.get_publisher ().add_data_writer (this, m_topic.get_name (), m_topic.get_type_name (), m_topic_id);
}

template <typename T, typename Transport>
data_writer_impl<T, Transport>::~data_writer_impl ()
{
	m_service.get_publisher ().remove_data_writer (this, m_topic_id);
}

template <typename T, typename Transport>
template <typename Handler>
void data_writer_impl<T, Transport>::async_write (const T &t, const Handler &handler)
{
	std::string topic_data;
	if (!m_topic.serialize (t, &topic_data))
	{
		m_service.get_io_service ().post (
			std::bind (handler, yail::pubsub::error::serialization_failed));
	}
	else
	{
		auto op = std::make_shared<write_operation<Handler>> (handler);

		m_service.get_publisher ().async_send (this, m_topic_id, topic_data,
			[this, op] (const boost::system::error_code &ec)
				{
					op->m_handler (ec);
				});
	}
}

} // namespace detail
} // namespace pubsub
} // namespace yail

#endif // YAIL_PUBSUB_DATA_WRITER_IMPL_H
