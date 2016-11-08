#ifndef YAIL_PUBSUB_TRANSPORT_DETAIL_SHMEM_IMPL_H
#define YAIL_PUBSUB_TRANSPORT_DETAIL_SHMEM_IMPL_H

#include <yail/pubsub/transport/shmem.h>

#include <sstream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <utility>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include <yail/log.h>
#include <yail/pubsub/error.h>

//
// yail::detail::shmem_impl
//
namespace yail {
namespace pubsub {
namespace transport {
namespace detail {

using namespace boost::interprocess;	

class shmem_impl
{
public:
	class channel_map
	{
	public:
		// receiver list
		using shm_char_allocator = allocator<char, managed_shared_memory::segment_manager>; 
		using shm_string = basic_string<char, std::char_traits<char>, shm_char_allocator>;

		struct shm_receiver_ctx
		{
			shm_receiver_ctx (shm_char_allocator &allocator);
			~shm_receiver_ctx ();

			shm_string m_uuid;
			pid_t m_pid;
		};

		using KeyType = shm_string;
		using MappedType = shm_receiver_ctx;
		using ValueType = std::pair<const shm_string, shm_receiver_ctx>;
		using shm_receiver_ctx_allocator = allocator<ValueType, managed_shared_memory::segment_manager>;
		using receiver_map = multimap<KeyType, MappedType, std::less<KeyType>, shm_receiver_ctx_allocator>;
		using receivers = std::vector<std::pair<std::string, pid_t>>;

		struct shm_ctx
		{
			shm_ctx (shm_receiver_ctx_allocator &allocator);
			~shm_ctx ();

			boost::interprocess::interprocess_mutex m_mutex;
			receiver_map m_receiver_map;
		};

		channel_map ();
		~channel_map ();

		void add_receiver (const std::string &topic_id, const std::string &uuid);
		void remove_receiver (const std::string &topic_id, const std::string &uuid);
		receivers get_receivers (const std::string &topic_id) const;
		void lock ();
		void unlock ();

	private:
		managed_shared_memory m_segment;
		shm_char_allocator m_char_allocator;
		shm_receiver_ctx_allocator m_receiver_ctx_allocator;
		shm_ctx *m_shm_ctx;
	};

	class sender
	{
	public:
		sender (yail::io_service &io_service, channel_map &channel_map);
		~sender ();

		void send (const std::string &topic_id, const yail::buffer &buffer, boost::system::error_code &ec, const uint32_t timeout)
		{
			auto op = std::make_shared<sync_send_operation> (topic_id, buffer, ec);
			{
				std::lock_guard<std::mutex> lock (m_op_mutex);
				m_op_queue.push (op);
			}
			m_op_available.notify_one ();
			
			// wait for operation to complete
			std::unique_lock<std::mutex> lock (op->m_mutex);
			if (timeout)
			{
				const auto ret = op->m_cond_done.wait_for (lock, std::chrono::seconds(timeout), [op] () { return op->m_done; });
				if (!ret)
				{
					op->m_ec = boost::asio::error::operation_aborted;
					op->m_done = true;
				}
			}
			else
			{
				op->m_cond_done.wait (lock, [op] () { return op->m_done; });
			}
		}
		
		template <typename Handler>
		void async_send (const std::string &topic_id, const yail::buffer &buffer, const Handler &handler)
		{
			auto op = std::make_shared<async_send_operation> (topic_id, buffer, handler);
			{
				std::lock_guard<std::mutex> lock (m_op_mutex);
				m_op_queue.push (op);
			}	
			m_op_available.notify_one ();
		}

	private:
		struct send_operation
		{
			enum type { SYNC, ASYNC };
			YAIL_API send_operation (const std::string &topic_id, const yail::buffer &buffer, type t);
			YAIL_API virtual ~send_operation ();
			
			bool is_async () const { return m_type == ASYNC; }
		
			std::string m_topic_id;
			const yail::buffer &m_buffer;
			type m_type;
		};
		struct sync_send_operation : public send_operation
		{
			YAIL_API sync_send_operation (const std::string &topic_id, const yail::buffer &buffer, boost::system::error_code &ec);
			YAIL_API ~sync_send_operation ();

			boost::system::error_code &m_ec;
			std::mutex m_mutex;
			std::condition_variable m_cond_done;
			bool m_done;
		};
		using send_handler = std::function<void (const boost::system::error_code &ec)>;
		struct async_send_operation : public send_operation
		{
			YAIL_API async_send_operation (const std::string &topic_id, const yail::buffer &buffer, const send_handler &handler);
			YAIL_API ~async_send_operation ();

			send_handler m_handler;	
		};

		void do_work ();
		void complete_ops_with_error (const boost::system::error_code &ec);

		yail::io_service &m_io_service;
		channel_map &m_channel_map;
		std::mutex m_op_mutex;
		std::condition_variable m_op_available;
		std::queue<std::shared_ptr<send_operation>> m_op_queue;
		std::thread m_thread;
		bool m_stop_work;
	};

	class receiver
	{
	public:
		receiver (yail::io_service &io_service, channel_map &channel_map);
		~receiver ();

		boost::uuids::uuid get_uuid () const 
		{ 
			return m_uuid;
		}

		template <typename Handler>
		void async_receive (yail::buffer &buffer, const Handler &handler)
		{
			std::unique_lock<std::mutex> bq_lock (m_buffer_queue_mutex);
			if (!m_buffer_queue.empty())
			{
				auto buf = std::move (m_buffer_queue.front ());
				m_buffer_queue.pop ();
				bq_lock.unlock ();

				buffer = std::move (buf);
				m_io_service.post (std::bind (handler, yail::pubsub::error::success));
			}
			else
			{
				bq_lock.unlock ();
				
				auto op = yail::make_unique<receive_operation> (buffer, handler);
				std::lock_guard<std::mutex> op_lock (m_op_queue_mutex);
				m_op_queue.push (std::move(op));
			}
		}

	private:
		using receive_handler = std::function<void (const boost::system::error_code &ec)>;
		struct receive_operation
		{
			YAIL_API receive_operation (yail::buffer &buffer, const receive_handler &handler);
			YAIL_API ~receive_operation ();

			yail::buffer &m_buffer;
			receive_handler m_handler;
		};

		void do_work ();
		void complete_ops_with_error (const boost::system::error_code &ec);

		yail::io_service &m_io_service;
		channel_map &m_channel_map;
		boost::uuids::uuid m_uuid;
		boost::interprocess::message_queue m_mq;
		std::queue<std::unique_ptr<receive_operation>> m_op_queue;
		std::mutex m_op_queue_mutex;
		std::queue<yail::buffer> m_buffer_queue;
		std::mutex m_buffer_queue_mutex;
		std::thread m_thread;
		bool m_stop_work;
	};

	shmem_impl (yail::io_service &io_service);
	~shmem_impl ();

	YAIL_API void add_topic (const std::string &topic_id);

	YAIL_API void remove_topic (const std::string &topic_id);

	void send (const std::string &topic_id, const yail::buffer &buffer, boost::system::error_code &ec, const uint32_t timeout)
	{
		m_sender.send (topic_id, buffer, ec, timeout);
	}

	template <typename Handler>
	void async_send (const std::string &topic_id, const yail::buffer &buffer, const Handler &handler)
	{
		m_sender.async_send (topic_id, buffer, handler);
	}

	template <typename Handler>
	void async_receive (yail::buffer &buffer, const Handler &handler)
	{
		m_receiver.async_receive (buffer, handler);
	}

private:
	boost::asio::io_service::work m_work;
	channel_map m_channel_map;
	sender m_sender;
	receiver m_receiver;
};

} // namespace detail
} // namespace transport
} // namespace pubsub
} // namespace yail

#endif // YAIL_PUBSUB_TRANSPORT_DETAIL_SHMEM_IMPL_H
