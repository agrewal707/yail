#ifndef YAIL_PUBSUB_DETAIL_PUBLISHER_H
#define YAIL_PUBSUB_DETAIL_PUBLISHER_H

#include <string>
#include <queue>
#include <functional>
#include <unordered_map>
#include <mutex>

#include <yail/io_service.h>
#include <yail/exception.h>
#include <yail/buffer.h>
#include <yail/memory.h>
#include <yail/pubsub/error.h>
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
	YAIL_API void add_data_writer (const void *id, const std::string &topic_name, 
		const std::string &topic_type_name, std::string &topic_id);

	/// Remove data writer from the set of data writers that are serviced by this publisher
	YAIL_API void remove_data_writer (const void *id, const std::string &topic_id);

	/// construct pubsub data message
	YAIL_API bool construct_pubsub_message (const std::string &topic_name,
		const std::string &topic_type_name, const std::string &topic_data, yail::buffer &buffer);

	struct send_operation
	{
		YAIL_API send_operation (const send_handler &handler);
		YAIL_API ~send_operation ();

		send_handler m_handler;
		yail::buffer m_buffer;
	};

	struct dw
	{
		dw (const std::string &topic_name, const std::string &topic_type_name);
		~dw ();

		std::string m_topic_name;
		std::string m_topic_type_name;
		std::queue<std::unique_ptr<send_operation>> m_op_queue;
		std::mutex m_op_queue_mutex;
	};
	using dw_map = std::unordered_map<const void*, std::unique_ptr<dw>>;

	yail::io_service &m_io_service;
	std::string m_domain;
	using topic_map = std::unordered_map<std::string, std::unique_ptr<dw_map>>;
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
	void add_data_writer (const void *id, const std::string &topic_name, const std::string &topic_type_name, std::string &topic_id)
	{
		std::lock_guard<std::mutex> lock (m_topic_map_mutex);
		publisher_common::add_data_writer (id, topic_name, topic_type_name, topic_id);		
	}

	/// Remove data writer from the set of data writers that are serviced by this publisher
	void remove_data_writer (const void *id, const std::string &topic_id)
	{
		std::lock_guard<std::mutex> lock (m_topic_map_mutex);
		publisher_common::remove_data_writer (id, topic_id);
	}

	/// Send topic data
	void send (const void *id, const std::string &topic_id, const std::string &topic_data, boost::system::error_code &ec, const uint32_t timeout)
	{
		// A performance optimization is possible by maintaining separate mutex for topic_map and dw_map, 
		// but we keep things simple by maintaining a single mutex.
		std::unique_lock<std::mutex> lock (m_topic_map_mutex);
		
		// lookup data writer map
		auto it = m_topic_map.find (topic_id);
		if (it != m_topic_map.end ())
		{
			auto &dwmap = it->second;

			// look up data writer ctx
			auto it2 = dwmap->find (id);
			if (it2 != dwmap->end ())
			{
				auto &dwctx = it2->second;
				lock.unlock ();
				
				yail::buffer buffer;
				if (construct_pubsub_message (dwctx->m_topic_name, dwctx->m_topic_type_name, topic_data, buffer))
				{
					m_transport.send (buffer, ec, timeout);
				}
				else
				{
					ec = yail::pubsub::error::system_error;
				}
			}
			else
			{
				ec = yail::pubsub::error::unknown_data_writer;
			}
		}
		else
		{
			ec = yail::pubsub::error::unknown_topic;
		}
	}

	/// Send topic data
	template <typename Handler>
	void async_send (const void *id, const std::string &topic_id, const std::string &topic_data, const Handler &handler)
	{
		std::unique_lock<std::mutex> lock (m_topic_map_mutex);
		
		// lookup data writer map
		auto it = m_topic_map.find (topic_id);
		if (it != m_topic_map.end ())
		{
			auto &dwmap = it->second;

			// look up data writer ctx
			auto it2 = dwmap->find (id);
			if (it2 != dwmap->end ())
			{
				auto &dwctx = it2->second;
				lock.unlock ();
				
				auto op (yail::make_unique<send_operation> (handler));
				if (construct_pubsub_message (dwctx->m_topic_name, dwctx->m_topic_type_name, topic_data, op->m_buffer))
				{
					m_transport.async_send (op->m_buffer,
						[&dwctx] (const boost::system::error_code &ec)
							{
								// Transport Send API is assumed to complete send operations 
								// in the sequence that they are invoked. Otherwise, following code
								// needs to be modified to check which operation in queue was completed,
								// instead of picking one at the head of the queue.
								std::unique_lock<std::mutex> oq_lock (dwctx->m_op_queue_mutex);
								auto op = std::move (dwctx->m_op_queue.front ());
								dwctx->m_op_queue.pop ();
								oq_lock.unlock ();

								op->m_handler (ec);
							});

					std::lock_guard<std::mutex> oq_lock (dwctx->m_op_queue_mutex);
					dwctx->m_op_queue.push (std::move(op));
				}
				else
				{
					m_io_service.post (std::bind (op->m_handler, yail::pubsub::error::system_error));
				}
			}
			else
			{
				m_io_service.post (std::bind (handler, yail::pubsub::error::unknown_data_writer));
			}
		}
		else
		{
			m_io_service.post (std::bind (handler, yail::pubsub::error::unknown_topic));
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
