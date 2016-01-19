#ifndef YAIL_PUBSUB_DETAIL_SUBSCRIBER_H
#define YAIL_PUBSUB_DETAIL_SUBSCRIBER_H

#include <string>
#include <queue>
#include <functional>
#include <unordered_map>

#include <yail/io_service.h>
#include <yail/exception.h>
#include <yail/buffer.h>
#include <yail/memory.h>
#include <yail/pubsub/error.h>
#include <yail/pubsub/detail/messages/yail.pb.h>

//
// subscriber
//

namespace yail {
namespace pubsub {
namespace detail {

//
//
// Transport independent subscriber functionality
//
struct subscriber_common
{
	using receive_handler = std::function<void (const boost::system::error_code &ec)>;

	subscriber_common (yail::io_service &io_service, const std::string &domain);
	~subscriber_common ();

	/// Add data reader to the set of data readers that are serviced by this subscriber
	YAIL_API void add_data_reader (const void *id, const std::string &topic_name, 
		const std::string &topic_type_name, std::string &topic_id);

	/// Remove data reader from the set of data readers that are serviced by this subscriber
	YAIL_API void remove_data_reader (const void *id, const std::string &topic_id);

	/// processs pubsub message received on the transport channel
	void process_pubsub_message (const yail::buffer &buffer);

	/// processs pubsub data
	void process_pubsub_data (const messages::pubsub_data &data);

	/// complete all pending ops with an error
	void complete_ops_with_error (const boost::system::error_code &ec);

	struct receive_operation
	{
		YAIL_API receive_operation (std::string &topic_data, const receive_handler &handler);
		YAIL_API ~receive_operation ();

		std::string &m_topic_data;
		receive_handler m_handler;
	};

	struct dr
	{
		dr (const std::string &topic_name, const std::string &topic_type_name);
		~dr ();

		std::string m_topic_name;
		std::string m_topic_type_name;
		std::queue<std::unique_ptr<receive_operation>> m_op_queue;
		std::queue<std::string> m_data_queue;
	};
	using dr_map = std::unordered_map<const void*, std::unique_ptr<dr>>;

	yail::io_service &m_io_service;
	std::string m_domain;
	using topic_map = std::unordered_map<std::string, std::unique_ptr<dr_map>>;
	topic_map m_topic_map; 
};

//
// subscriber
//
template <typename Transport>
class subscriber : private subscriber_common
{
public:
	subscriber (yail::io_service &io_service, Transport &transport, const std::string &domain);
	~subscriber ();

	/// Add data reader to the set of data readers that are serviced by this subscriber
	void add_data_reader (const void *id, const std::string &topic_name, const std::string &topic_type_name, std::string &topic_id)
	{
		subscriber_common::add_data_reader (id, topic_name, topic_type_name, topic_id);
	}

	/// Remove data reader from the set of data readers that are serviced by this subscriber
	void remove_data_reader (const void *id, const std::string &topic_id)
	{
		subscriber_common::remove_data_reader (id, topic_id);
	}

	/// Receive topic data
	template <typename Handler>
	void async_receive (const void*id, const std::string &topic_id, std::string &topic_data, const Handler &handler)
	{
		// lookup data reader map
		auto it = m_topic_map.find (topic_id);
		if (it != m_topic_map.end ())
		{
			auto &drmap = it->second;

			// look up data reader ctx
			auto it2 = drmap->find (id);
			if (it2 != drmap->end ())
			{
				auto &drctx = it2->second;

				if (!drctx->m_data_queue.empty ())
				{
					auto data = drctx->m_data_queue.front ();
					drctx->m_data_queue.pop ();

					topic_data = std::move (data);
					m_io_service.post (std::bind (handler, yail::pubsub::error::success));
				}
				else
				{
					auto op (make_unique<receive_operation> (topic_data, handler));
					drctx->m_op_queue.push (std::move (op));
				}
			}
			else
			{
				m_io_service.post (std::bind (handler, yail::pubsub::error::unknown_data_reader));
			}
		}
		else
		{
			m_io_service.post (std::bind (handler, yail::pubsub::error::unknown_topic));
		}
	}

private:
	void do_receive ();

	Transport &m_transport;
	yail::buffer m_buffer;
};

} // namespace detail
} // namespace pubsub
} // namespace yail

namespace yail {
namespace pubsub {
namespace detail {

template <typename Transport>
subscriber<Transport>::subscriber (yail::io_service &io_service, Transport &transport, const std::string &domain) :
	subscriber_common (io_service, domain),
	m_transport (transport),
	m_buffer ()
{
	do_receive ();
}

template <typename Transport>
subscriber<Transport>::~subscriber ()
{}

template <typename Transport> 
void subscriber<Transport>::do_receive ()
{
	m_transport.async_receive (m_buffer,
		[this] (const boost::system::error_code &ec)
			{
				if (!ec)
				{
					process_pubsub_message (m_buffer);
					do_receive ();
				}
				else
				{
					complete_ops_with_error (ec);
				}
			});
}

} // namespace detail
} // namespace pubsub
} // namespace yail

#endif // YAIL_PUBSUB_DETAIL_SUBSCRIBER_H
