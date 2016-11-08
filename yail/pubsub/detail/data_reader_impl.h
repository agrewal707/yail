#ifndef YAIL_PUBSUB_DETAIL_DATA_READER_IMPL_H
#define YAIL_PUBSUB_DETAIL_DATA_READER_IMPL_H

#include <yail/pubsub/data_reader.h>

#include <yail/pubsub/error.h>
#include <yail/pubsub/detail/subscriber.h>
#include <yail/pubsub/detail/service_impl.h>
#include <yail/pubsub/detail/topic_impl.h>

namespace yail {
namespace pubsub {
namespace detail {

template <typename T, typename Transport>
class data_reader_impl
{
public:
	data_reader_impl (service_impl<Transport> &service, topic_impl<T> &topic);
	~data_reader_impl ();

	void read (T &t, boost::system::error_code &ec, const uint32_t timeout);

	template <typename Handler>
	void async_read (T &t, const Handler &handler);

	void cancel ();

private:
	template <typename Handler>
	struct read_operation
	{
		read_operation (T &t, const Handler &handler);
		~read_operation ();

		T &m_t;
		Handler m_handler;
		std::string m_topic_data;
	};

	service_impl<Transport> &m_service;
	topic_impl<T> &m_topic;
	std::string m_topic_id;
};

} // namespace detail
} // namespace pubsub
} // namespace yail

//
// yail::detail::data_reader_impl template implementation
//
//
namespace yail {
namespace pubsub {
namespace detail {

//
// data_reader_impl::read_operation
//
template <typename T, typename Transport>
template <typename Handler>
data_reader_impl<T, Transport>::read_operation<Handler>::read_operation (T &t, const Handler &handler) :
	m_t (t),
	m_handler (handler),
	m_topic_data ()
{}

template <typename T, typename Transport>
template <typename Handler>
data_reader_impl<T, Transport>::read_operation<Handler>::~read_operation ()
{}

//
// data_reader_impl
//
template <typename T, typename Transport>
data_reader_impl<T, Transport>::data_reader_impl (service_impl<Transport> &service, topic_impl<T> &topic) :
	m_service (service),
	m_topic (topic),
	m_topic_id ()
{
	m_service.get_subscriber ().add_data_reader (this, m_topic.get_name (), m_topic.get_type_name (), m_topic_id);
}

template <typename T, typename Transport>
data_reader_impl<T, Transport>::~data_reader_impl ()
{
	m_service.get_subscriber ().remove_data_reader (this, m_topic_id);
}

template <typename T, typename Transport>
void data_reader_impl<T, Transport>::read (T &t, boost::system::error_code &ec, const uint32_t timeout)
{
	std::string topic_data;
	m_service.get_subscriber ().receive (this, m_topic_id, topic_data, ec, timeout);
	if (!ec)
	{
		if (!m_topic.deserialize (t, topic_data))
		{
			ec = yail::pubsub::error::deserialization_failed;
		}
	}
}

template <typename T, typename Transport>
template <typename Handler>
void data_reader_impl<T, Transport>::async_read (T &t, const Handler &handler)
{
	auto op = std::make_shared<read_operation<Handler>> (t, handler);

	m_service.get_subscriber ().async_receive (this, m_topic_id, op->m_topic_data,
		[this, op] (const boost::system::error_code &ec)
			{
				if (!ec)
				{
					if (m_topic.deserialize (op->m_t, op->m_topic_data))
					{
						op->m_handler (ec);
					}
					else
					{
						op->m_handler (yail::pubsub::error::deserialization_failed);
					}
				}
				else
				{
					op->m_handler (ec);
				}
			});
}

template <typename T, typename Transport>
void data_reader_impl<T, Transport>::cancel ()
{
	m_service.get_subscriber ().cancel (this, m_topic_id);
}

} // namespace detail
} // namespace pubsub
} // namespace yail

#endif // YAIL_PUBSUB_DETAIL_DATA_READER_IMPL_H
