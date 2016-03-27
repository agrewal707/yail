#ifndef YAIL_PUBSUB_DETAIL_SUBSCRIBER_H
#define YAIL_PUBSUB_DETAIL_SUBSCRIBER_H

#include <string>
#include <queue>
#include <functional>
#include <unordered_map>
#include <mutex>
#include <condition_variable>

#include <yail/io_service.h>
#include <yail/exception.h>
#include <yail/buffer.h>
#include <yail/memory.h>
#include <yail/pubsub/error.h>
#include <yail/pubsub/detail/messages/pubsub.pb.h>

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
		enum type { SYNC, ASYNC };
		YAIL_API receive_operation (std::string &topic_data, type t);
		YAIL_API virtual ~receive_operation ();

		bool is_async () const { return m_type == ASYNC; }

		std::string &m_topic_data;
		type m_type;
	};
	struct sync_receive_operation : public receive_operation
	{
		YAIL_API sync_receive_operation (std::string &topic_data, boost::system::error_code &ec);
		YAIL_API ~sync_receive_operation ();
		
		boost::system::error_code &m_ec;
		std::mutex m_mutex;
		std::condition_variable m_cond_done;
		bool m_done;
	};
	struct async_receive_operation : public receive_operation
	{
		YAIL_API async_receive_operation (std::string &topic_data, const receive_handler &handler);
		YAIL_API ~async_receive_operation ();

		receive_handler m_handler;
	};

	struct dr
	{
		dr (const std::string &topic_name, const std::string &topic_type_name);
		~dr ();

		std::string m_topic_name;
		std::string m_topic_type_name;
		std::queue<std::unique_ptr<receive_operation>> m_op_queue;
		std::mutex m_op_queue_mutex;
		std::queue<std::string> m_data_queue;
		std::mutex m_data_queue_mutex;		
	};
	using dr_map = std::unordered_map<const void*, std::unique_ptr<dr>>;

	yail::io_service &m_io_service;
	std::string m_domain;
	using topic_map = std::unordered_map<std::string, std::unique_ptr<dr_map>>;
	topic_map m_topic_map;
	std::mutex m_topic_map_mutex;
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
		std::lock_guard<std::mutex> lock (m_topic_map_mutex);
		subscriber_common::add_data_reader (id, topic_name, topic_type_name, topic_id);
	}

	/// Remove data reader from the set of data readers that are serviced by this subscriber
	void remove_data_reader (const void *id, const std::string &topic_id)
	{
		std::lock_guard<std::mutex> lock (m_topic_map_mutex);
		subscriber_common::remove_data_reader (id, topic_id);
	}

	/// Receive topic data
	void receive (const void*id, const std::string &topic_id, std::string &topic_data, boost::system::error_code &ec, const uint32_t timeout)
	{
		std::unique_lock<std::mutex> lock (m_topic_map_mutex);
		
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
				lock.unlock ();
				
				std::unique_lock<std::mutex> dq_lock (drctx->m_data_queue_mutex);
				if (!drctx->m_data_queue.empty ())
				{
					auto data = drctx->m_data_queue.front ();
					drctx->m_data_queue.pop ();
					dq_lock.unlock ();

					topic_data = std::move (data);
					ec = yail::pubsub::error::success;
				}
				else
				{
					dq_lock.unlock ();
					
					auto op (make_unique<sync_receive_operation> (topic_data, ec));
					auto *op_raw = op.get ();
					{
						std::lock_guard<std::mutex> oq_lock (drctx->m_op_queue_mutex);
						drctx->m_op_queue.push (std::move (op));
					}
					
					// wait for operation to complete
					std::unique_lock<std::mutex> l (op_raw->m_mutex);
					if (timeout)
					{
						const auto ret = op_raw->m_cond_done.wait_for (l, std::chrono::seconds(timeout), [&op_raw] () { return op_raw->m_done; });
						if (!ret)
						{
							op_raw->m_ec = boost::asio::error::operation_aborted;
							op_raw->m_done = true;
						}
					}
					else
					{
						op_raw->m_cond_done.wait (l, [&op_raw] () { return op_raw->m_done; });
					}
				}
			}
			else
			{
				ec = yail::pubsub::error::unknown_data_reader;
			}
		}
		else
		{
			ec = yail::pubsub::error::unknown_topic;
		}
	}
	
	/// Receive topic data
	template <typename Handler>
	void async_receive (const void*id, const std::string &topic_id, std::string &topic_data, const Handler &handler)
	{
		std::unique_lock<std::mutex> lock(m_topic_map_mutex);
		
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
				lock.unlock ();
				
				std::unique_lock<std::mutex> dq_lock (drctx->m_data_queue_mutex);
				if (!drctx->m_data_queue.empty ())
				{
					auto data = drctx->m_data_queue.front ();
					drctx->m_data_queue.pop ();
					dq_lock.unlock ();

					topic_data = std::move (data);
					m_io_service.post (std::bind (handler, yail::pubsub::error::success));
				}
				else
				{
					dq_lock.unlock ();
				
					auto op (make_unique<async_receive_operation> (topic_data, handler));
					std::lock_guard<std::mutex> oq_lock (drctx->m_op_queue_mutex);
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
