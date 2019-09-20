#ifndef YAIL_PUBSUB_DETAIL_PUBLISHER_H
#define YAIL_PUBSUB_DETAIL_PUBLISHER_H

#include <string>
#include <queue>
#include <functional>
#include <unordered_map>
#include <mutex>

#include <boost/circular_buffer.hpp>

#include <yail/io_service.h>
#include <yail/exception.h>
#include <yail/buffer.h>
#include <yail/memory.h>
#include <yail/pubsub/error.h>
#include <yail/pubsub/detail/topic_info.h>
#include <yail/pubsub/detail/messages/pubsub.pb.h>

namespace yail {
namespace pubsub {
namespace detail {

//
// Transport independent publisher functionality
//
struct publisher_common
{
	using send_handler = std::function<void (const boost::system::error_code &ec)>;

	publisher_common (yail::io_service &io_service, const std::string &domain);
	~publisher_common ();

	/// Add data writer to the set of data writers that are serviced by this publisher
	YAIL_API void add_data_writer (const void *id, const topic_info &topic_info, std::string &topic_id);

	/// Remove data writer from the set of data writers that are serviced by this publisher
	YAIL_API void remove_data_writer (const void *id, const std::string &topic_id);

	struct send_operation
	{
		YAIL_API send_operation (const send_handler &handler);
		YAIL_API ~send_operation ();

		send_handler m_handler;
		yail::buffer m_buffer;
	};

	struct dw
	{
		std::queue<std::unique_ptr<send_operation>> m_op_queue;
		std::mutex m_op_queue_mutex;
	};
	struct topic
	{
		topic (const topic_info &topic_info);
		~topic();

		topic_info m_topic_info;
		using dw_map = std::unordered_map<const void*, std::shared_ptr<dw>>;
		dw_map m_dw_map;
		boost::circular_buffer<messages::pubsub_data> m_data_ring;
	};

	YAIL_API std::weak_ptr<dw> build_data_message (
		const void *id,
		const std::string &topic_id,
		const std::string &topic_data,
		yail::buffer &buffer,
		boost::system::error_code &ec);

	/// construct pubsub data
	YAIL_API bool construct_pubsub_data (
		const topic_info &topic_info, const std::string &topic_data, messages::pubsub_data &data);

	/// construct pubsub data message
	YAIL_API bool construct_pubsub_message (const messages::pubsub_data &data, yail::buffer &buffer);

	yail::io_service &m_io_service;
	std::string m_domain;
	using topic_map = std::unordered_map<std::string, std::unique_ptr<topic>>;
	topic_map m_topic_map;
	std::mutex m_topic_map_mutex;
	int32_t m_mid;
};

//
// publisher
//
template <typename Transport>
class publisher : private publisher_common
{
public:
	publisher (yail::io_service &io_service, Transport &transport, const std::string &domain);
	~publisher ();

	/// Add data writer to the set of data writers that are serviced by this publisher
	void add_data_writer (const void *id, const topic_info &topic_info, std::string &topic_id)
	{
		std::lock_guard<std::mutex> lock (m_topic_map_mutex);
		publisher_common::add_data_writer (id, topic_info, topic_id);
	}

	/// Remove data writer from the set of data writers that are serviced by this publisher
	void remove_data_writer (const void *id, const std::string &topic_id)
	{
		std::lock_guard<std::mutex> lock (m_topic_map_mutex);
		publisher_common::remove_data_writer (id, topic_id);
	}

	/// Notify publisher to re-send previously published data (if any)
	void notify (const messages::subscription &sub)
	{
		std::string topic_id (sub.domain () + sub.topic_name () + sub.topic_type_name ());

		YAIL_LOG_TRACE ("topic id: " << topic_id);

		std::lock_guard<std::mutex> lock (m_topic_map_mutex);
		auto it = m_topic_map.find (topic_id);
		if (it != m_topic_map.end ())
		{
			auto &tctx = it->second;
			YAIL_LOG_TRACE ("data ring size : " << tctx->m_data_ring.size());

			for (auto it2 = tctx->m_data_ring.begin (); it2 != tctx->m_data_ring.end (); ++it2)
			{
				const auto &data = *it2;
				yail::buffer buffer;
				if (construct_pubsub_message (data, buffer))
				{
					boost::system::error_code ec;
					m_transport.send (topic_id, buffer, ec, 5);
					if (ec)
					{
						YAIL_LOG_ERROR ("failed to publish history");
					}
				}
			}
		}
	}

	/// Send topic data
	void send (
		const void *id,
		const std::string &topic_id,
		const std::string &topic_data,
		boost::system::error_code &ec,
		const uint32_t timeout)
	{
		yail::buffer buffer;
		build_data_message (id, topic_id, topic_data, buffer, ec);
		if (!ec)
		{
			m_transport.send (topic_id, buffer, ec, timeout);
		}
	}

	/// Send topic data
	template <typename Handler>
	void async_send (
		const void *id,
		const std::string &topic_id,
		const std::string &topic_data,
		const Handler &handler)
	{
		auto op (yail::make_unique<send_operation> (handler));

		boost::system::error_code ec;
		auto wp = build_data_message (id, topic_id, topic_data, op->m_buffer, ec);
		if (!ec)
		{
			m_transport.async_send (topic_id, op->m_buffer,
				[wp] (const boost::system::error_code &ec2)
					{
						if (auto dwctx = wp.lock())
						{
							// Transport Send API is assumed to complete send operations
							// in the sequence that they are invoked. Otherwise, following code
							// needs to be modified to check which operation in queue was completed,
							// instead of picking one at the head of the queue.
							std::unique_lock<std::mutex> oq_lock (dwctx->m_op_queue_mutex);
							auto op = std::move (dwctx->m_op_queue.front ());
							dwctx->m_op_queue.pop ();
							oq_lock.unlock ();

							op->m_handler (ec2);
						}
					});

			if (auto dwctx = wp.lock())
			{
				std::lock_guard<std::mutex> oq_lock (dwctx->m_op_queue_mutex);
				dwctx->m_op_queue.push (std::move(op));
			}
		}
		else
		{
			m_io_service.post (std::bind (handler, ec));
		}
	}

private:
	Transport &m_transport;
};

} // namespace detail
} // namespace pubsub
} // namespace yail

namespace yail {
namespace pubsub {
namespace detail {

template <typename Transport>
publisher<Transport>::publisher (yail::io_service &io_service, Transport &transport, const std::string &domain) :
	publisher_common (io_service, domain),
	m_transport (transport)
{}

template <typename Transport>
publisher<Transport>::~publisher ()
{}

} // namespace detail
} // namespace pubsub
} // namespace yail

#endif // YAIL_PUBSUB_DETAIL_PUBLISHER_H
