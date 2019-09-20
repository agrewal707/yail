#include <iostream>
#include <fstream>

#include <yail/pubsub/transport/shmem.h>
#include <yail/pubsub/transport/detail/shmem_impl.h>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/exception/diagnostic_information.hpp>

#include <yail/pubsub/error.h>

namespace yail {
namespace pubsub {
namespace transport {
namespace detail {

using namespace boost::interprocess;
using namespace boost::posix_time;

// shmem_impl::uuid_str
shmem_impl::uuid_str::uuid_str ()
{
	std::string s;
	std::getline( std::ifstream ("/proc/sys/kernel/random/uuid", std::ios::in|std::ios::binary), s );
	this->assign(s);
};

//
// shmem_impl::channel_map::shm_receiver_ctx
//
shmem_impl::channel_map::shm_receiver_ctx::shm_receiver_ctx (shm_char_allocator &allocator):
	m_uuid (allocator)
{}

shmem_impl::channel_map::shm_receiver_ctx::~shm_receiver_ctx ()
{}

//
// shmem_impl::channel_map::shm_receiver_ctx
//
shmem_impl::channel_map::shm_ctx::shm_ctx (shm_receiver_ctx_allocator &allocator):
	m_receiver_map (allocator)
{}

shmem_impl::channel_map::shm_ctx::~shm_ctx ()
{}

//
// shmem_impl::channel_map
//
shmem_impl::channel_map::channel_map () :
	m_segment (open_or_create, "yail_shmem_transport", YAIL_PUBSUB_SHMEM_SEGMENT_SIZE),
	m_char_allocator (m_segment.get_segment_manager()),
	m_receiver_ctx_allocator (m_segment.get_segment_manager()),
	m_shm_ctx (nullptr)
{
	YAIL_LOG_FUNCTION (this);

	try
	{
		m_shm_ctx = m_segment.find_or_construct<shm_ctx>(unique_instance)(m_receiver_ctx_allocator);
#ifndef NDEBUG
		scoped_lock<interprocess_mutex> lock(m_shm_ctx->m_mutex);
		for (const auto &val : m_shm_ctx->m_receiver_map)
		{
			YAIL_LOG_DEBUG ("receiver: " << val.first << "," << val.second.m_uuid << "," << val.second.m_pid);
		}
#endif
	}
	catch (const std::exception &ex)
	{
		YAIL_LOG_DEBUG (ex.what ());
	}
}

shmem_impl::channel_map::~channel_map ()
{
	YAIL_LOG_FUNCTION (this);
}

void shmem_impl::channel_map::add_receiver (const std::string &topic_id, const std::string &uuid)
{
	YAIL_LOG_FUNCTION (uuid);

	// Create receiver ctx
	shm_receiver_ctx ctx (m_char_allocator);
	ctx.m_uuid = shm_string (uuid.begin (), uuid.end (), m_char_allocator);
	ctx.m_pid = getpid ();
	YAIL_LOG_DEBUG ("add: " << ctx.m_uuid << "," << ctx.m_pid);

	// insert into receiver map
	KeyType key (topic_id.begin (), topic_id.end (), m_char_allocator);
	ValueType val (key, ctx);

	scoped_lock<interprocess_mutex> lock(m_shm_ctx->m_mutex);
	m_shm_ctx->m_receiver_map.insert(val);

	// remove receivers that no longer exist
	for (auto it = m_shm_ctx->m_receiver_map.begin (); it != m_shm_ctx->m_receiver_map.end ();)
	{
		if (-1 == kill (it->second.m_pid, 0))
		{
			// this process doesnot exist, so remove this receiver
			YAIL_LOG_DEBUG ("removed: " << it->second.m_uuid << "," << it->second.m_pid);
			it = m_shm_ctx->m_receiver_map.erase (it);
		}
		else
		{
			++it;
		}
	}
}

void shmem_impl::channel_map::remove_receiver (const std::string &topic_id, const std::string &uuid)
{
	YAIL_LOG_FUNCTION (uuid);

	scoped_lock<interprocess_mutex> lock(m_shm_ctx->m_mutex);
	for (auto it = m_shm_ctx->m_receiver_map.begin (); it != m_shm_ctx->m_receiver_map.end ();)
	{
		if (!((!topic_id.empty () && strcmp (topic_id.c_str (), it->first.c_str ())) || strcmp (uuid.c_str (), it->second.m_uuid.c_str ())))
		{
			YAIL_LOG_DEBUG ("removed: " << it->second.m_uuid << "," << it->second.m_pid);
			m_shm_ctx->m_receiver_map.erase (it);
			break;
		 }
		else
		{
			++it;
		}
	}
}


shmem_impl::channel_map::receivers shmem_impl::channel_map::get_receivers (const std::string &topic_id) const
{
	YAIL_LOG_FUNCTION (this);

	receivers retval;

	KeyType key (topic_id.begin (), topic_id.end (), m_char_allocator);
	const auto range = m_shm_ctx->m_receiver_map.equal_range (key);
	if (range.first != m_shm_ctx->m_receiver_map.end ())
	{
		// return receivers that are hosting this topic_id
		std::for_each (range.first, range.second,
			[ &retval ] (const receiver_map::value_type& val)
			{
				auto tmp = std::make_pair (val.second.m_uuid.c_str (), val.second.m_pid);
				retval.push_back (tmp);
			}
		);
	}

	return retval;
}

void shmem_impl::channel_map::lock ()
{
	YAIL_LOG_FUNCTION (this);

	m_shm_ctx->m_mutex.lock ();
}

void shmem_impl::channel_map::unlock ()
{
	YAIL_LOG_FUNCTION (this);

	m_shm_ctx->m_mutex.unlock ();
}


//
// shmem_impl::sender::send_operation
//
shmem_impl::sender::send_operation::send_operation (const std::string &topic_id, const yail::buffer &buffer, type t) :
	m_topic_id (topic_id),
	m_buffer (buffer),
	m_type (t)
{
	YAIL_LOG_FUNCTION (this);
}

shmem_impl::sender::send_operation::~send_operation ()
{
	YAIL_LOG_FUNCTION (this);
}

//
// shmem_impl::sender::sync_send_operation
//
shmem_impl::sender::sync_send_operation::sync_send_operation (const std::string &topic_id, const yail::buffer &buffer, boost::system::error_code &ec) :
	send_operation (topic_id, buffer, SYNC),
	m_ec (ec),
	m_mutex (),
	m_cond_done (),
	m_done (false)
{
	YAIL_LOG_FUNCTION (this);
}

shmem_impl::sender::sync_send_operation::~sync_send_operation ()
{
	YAIL_LOG_FUNCTION (this);
}

//
// shmem_impl::sender::async_send_operation
//
shmem_impl::sender::async_send_operation::async_send_operation (const std::string &topic_id, const yail::buffer &buffer, const send_handler &handler) :
	send_operation(topic_id, buffer, ASYNC),
	m_handler (handler)
{
	YAIL_LOG_FUNCTION (this);
}

shmem_impl::sender::async_send_operation::~async_send_operation ()
{
	YAIL_LOG_FUNCTION (this);
}

//
// shmem_impl::sender
//
shmem_impl::sender::sender (yail::io_service &io_service, shmem_impl::channel_map &chmap) :
	m_io_service (io_service),
	m_channel_map (chmap),
	m_op_mutex (),
	m_op_available (),
	m_op_queue (),
	m_thread (&shmem_impl::sender::do_work, this),
	m_stop_work (false)
{
	YAIL_LOG_FUNCTION (this);
}

shmem_impl::sender::~sender ()
{
	YAIL_LOG_FUNCTION (this);

	try
	{
		{
			std::lock_guard<std::mutex> lock (m_op_mutex);
			m_stop_work = true;
		}
		m_op_available.notify_one ();
		m_thread.join ();
	} catch (...) {}
}

void shmem_impl::sender::do_work ()
{
	YAIL_LOG_FUNCTION (this);

	bool stop = false;
	while (!stop)
	{
		std::shared_ptr<send_operation> op;
		{
			// wait on send operation from client
			std::unique_lock<std::mutex> lock (m_op_mutex);
			m_op_available.wait (lock, [this] () { return !m_op_queue.empty () || m_stop_work; });

			if (!m_stop_work)
			{
				// dequeue an operation
				op = m_op_queue.front ();
				m_op_queue.pop ();
			}
			else
			{
				stop = true;
			}
		}

		if (!stop)
		{
			try
			{
				std::vector<std::string> dead_receivers;

				{ // lock channel and send message
					struct channel_lock
					{
						channel_lock (channel_map &cm) : m_cm (cm) {  m_cm.lock (); }
						~channel_lock () {  m_cm.unlock (); }
						channel_map &m_cm;
					} lock (m_channel_map);

					const auto receivers = m_channel_map.get_receivers (op->m_topic_id);
					for (const auto &rcv: receivers)
					{
						const auto &uuid = rcv.first;
						const auto pid = rcv.second;

						YAIL_LOG_TRACE ("sending to: " << uuid<< "," << pid);

						try
						{
							// send data to receiver's mq
							boost::interprocess::message_queue mq (open_only, uuid.c_str ());
							ptime abs_time (second_clock::universal_time() + seconds(1));
							if (!mq.timed_send(op->m_buffer.data (), op->m_buffer.size (), 0, abs_time))
							{
								YAIL_LOG_WARNING ("receiver: " << uuid << "," << pid << " queue is full");
								if (-1 == kill (pid, 0))
								{
									// this process doesnot exist, so remove this receiver
									YAIL_LOG_WARNING ("removed: " << uuid << "," << pid);
									dead_receivers.push_back (uuid);
								}
							}
						}
						catch (const interprocess_exception &ex)
						{
							YAIL_LOG_ERROR ("receiver: " << uuid << "," << pid << " error: " << ex.what ());
							if (-1 == kill (pid, 0))
							{
								// this process doesnot exist, so remove this receiver
								YAIL_LOG_WARNING ("removed: " << uuid << "," << pid);
								dead_receivers.push_back (uuid);
							}

							// Keep going until we loop through all receivers
						}
					}
				}

				// remove all dead receivers from the channel
				for (const auto &uuid : dead_receivers)
				{
					m_channel_map.remove_receiver (std::string (), uuid);
				}

				// complete operation with success. this is best effort transport
				// we don't error if unable to send to one or more receivers.
				if (op->is_async ())
				{
					auto async_op = static_cast<async_send_operation*> (op.get ());
					m_io_service.post (std::bind (async_op->m_handler, yail::pubsub::error::success));
				}
				else
				{
					auto sync_op = static_cast<sync_send_operation*> (op.get ());
					std::lock_guard<std::mutex> l (sync_op->m_mutex);
					if (!sync_op->m_done)
					{
						sync_op->m_ec = yail::pubsub::error::success;
						sync_op->m_done = true;
						sync_op->m_cond_done.notify_one ();
					}
				}
			}
			catch (const std::exception &ex)
			{
				YAIL_LOG_ERROR ("sender error: " << boost::diagnostic_information(ex));

				// complete pending operations with error but continue operation
				complete_ops_with_error (yail::pubsub::error::system_error);
			}
		}
	}
}

void shmem_impl::sender::complete_ops_with_error (const boost::system::error_code &ec)
{
	YAIL_LOG_FUNCTION (this);

	std::lock_guard<std::mutex> lock (m_op_mutex);
	while (!m_op_queue.empty ())
	{
		auto op = m_op_queue.front ();
		m_op_queue.pop ();
		if (op->is_async ())
		{
			auto async_op = static_cast<async_send_operation*> (op.get ());
			m_io_service.post (std::bind (async_op->m_handler, ec));
		}
		else
		{
			auto sync_op = static_cast<sync_send_operation*> (op.get ());
			sync_op->m_ec = ec;
			sync_op->m_done = true;
			sync_op->m_cond_done.notify_one ();
		}
	}
}

//
// receiver::receive_operation
//
shmem_impl::receiver::receive_operation::receive_operation (yail::buffer &buffer, const receive_handler &handler) :
	m_buffer (buffer),
	m_handler (handler)
{
	YAIL_LOG_FUNCTION (this);
}

shmem_impl::receiver::receive_operation::~receive_operation ()
{
	YAIL_LOG_FUNCTION (this);
}

//
// shmem_impl::receiver
//
shmem_impl::receiver::receiver (yail::io_service &io_service, shmem_impl::channel_map &chmap) :
	m_io_service (io_service),
	m_channel_map (chmap),
	m_uuid (),
	m_mq (create_only, m_uuid.c_str(), YAIL_PUBSUB_SHMEM_RECEIVER_QUEUE_DEPTH, YAIL_PUBSUB_MAX_MSG_SIZE),
	m_op_queue (),
	m_thread (&shmem_impl::receiver::do_work, this),
	m_stop_work (false)
{
	YAIL_LOG_FUNCTION (this);
}

shmem_impl::receiver::~receiver ()
{
	YAIL_LOG_FUNCTION (this);

	try
	{
		// A hack to break out of blocking mq receive
		uint8_t dummy;
		m_mq.send (&dummy, 0, 0);

		m_stop_work = true;
		m_thread.join ();

		m_channel_map.remove_receiver (std::string (), m_uuid);

		message_queue::remove(m_uuid.c_str ());
	}
	catch (...) {};
}

void shmem_impl::receiver::do_work ()
{
	YAIL_LOG_FUNCTION (this);

	bool stop = false;
	while (!stop)
	{
		try
		{
			yail::buffer buf (YAIL_PUBSUB_MAX_MSG_SIZE);
			message_queue::size_type recvd_size; unsigned int priority;
			m_mq.receive(buf.data (), buf.size (), recvd_size, priority);
			if (recvd_size)
			{
				// resize buffer based on data received
				buf.resize (recvd_size);

				std::unique_lock<std::mutex> oq_lock (m_op_queue_mutex);
				if (!m_op_queue.empty())
				{
					auto op = std::move (m_op_queue.front ());
					m_op_queue.pop ();
					oq_lock.unlock ();

					op->m_buffer = std::move (buf);
					m_io_service.post (std::bind (op->m_handler, yail::pubsub::error::success));
				}
				else
				{
					oq_lock.unlock ();

					std::lock_guard<std::mutex> bq_lock (m_buffer_queue_mutex);
					if (m_buffer_queue.size () <= YAIL_PUBSUB_SHMEM_RECEIVER_BUFFER_QUEUE_DEPTH)
					{
						m_buffer_queue.push (std::move(buf));
					}
					else
					{
						YAIL_LOG_WARNING ("buffer queue is full");
					}
				}
			}
			else
			{
				stop = true;
			}
		}
		catch (const std::bad_alloc &ex)
		{
			YAIL_LOG_ERROR ("receive buffer allocation error: " << boost::diagnostic_information(ex));

			// assume temporary resource unavailability..just delay resuming operation
			sleep (1);
		}
		catch (const std::exception &ex)
		{
			YAIL_LOG_ERROR ("receiver error: " << boost::diagnostic_information(ex));

			// complete pending operations with error but continue operation
			complete_ops_with_error (yail::pubsub::error::system_error);
		}
	}
}

void shmem_impl::receiver::complete_ops_with_error (const boost::system::error_code &ec)
{
	YAIL_LOG_FUNCTION (this);

	std::lock_guard<std::mutex> oq_lock (m_op_queue_mutex);
	while (!m_op_queue.empty ())
	{
		auto op = std::move (m_op_queue.front ());
		m_op_queue.pop ();
		m_io_service.post (std::bind (op->m_handler, ec));
	}
}

//
// shmem_impl
//
shmem_impl::shmem_impl (yail::io_service &io_service) :
	m_work (io_service),
	m_channel_map (),
	m_sender (io_service, m_channel_map),
	m_receiver (io_service, m_channel_map)
{
	YAIL_LOG_FUNCTION (this);
}

shmem_impl::~shmem_impl ()
{
	YAIL_LOG_FUNCTION (this);
}

void shmem_impl::add_topic (const std::string &topic_id)
{
	m_channel_map.add_receiver (topic_id, m_receiver.get_uuid ());
}

void shmem_impl::remove_topic (const std::string &topic_id)
{
	m_channel_map.remove_receiver (topic_id, m_receiver.get_uuid ());
}

} // namespace detail
} // namespace transport
} // namespace pubsub
} // namespace yail
