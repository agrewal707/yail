#include <yail/pubsub/transport/shmem.h>
#include <yail/pubsub/transport/detail/shmem_impl.h>

#include <boost/uuid/uuid_io.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <yail/pubsub/error.h>

namespace yail {
namespace pubsub {
namespace transport {
namespace detail {

using namespace boost::interprocess;
using namespace boost::posix_time; 

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
	m_receivers (allocator)
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
	YAIL_LOG_TRACE (this);

	try
	{
		m_shm_ctx = m_segment.find_or_construct<shm_ctx>(unique_instance)(m_receiver_ctx_allocator);
#ifndef NDEBUG
		scoped_lock<interprocess_mutex> lock(m_shm_ctx->m_mutex);
		for (auto &receiver : m_shm_ctx->m_receivers)
		{
			YAIL_LOG_DEBUG ("receiver: " << receiver.m_uuid << "," << receiver.m_pid);
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
	YAIL_LOG_TRACE (this);
}

void shmem_impl::channel_map::add_receiver (const std::string &uuid)
{
	YAIL_LOG_TRACE (uuid);

	shm_receiver_ctx ctx (m_char_allocator);
	ctx.m_uuid = shm_string (uuid.begin (), uuid.end (), m_char_allocator);
	ctx.m_pid = getpid ();
	YAIL_LOG_DEBUG ("add: " << ctx.m_uuid << "," << ctx.m_pid);

	scoped_lock<interprocess_mutex> lock(m_shm_ctx->m_mutex);
	m_shm_ctx->m_receivers.push_back (std::move(ctx));

	// remove receivers that no longer exist
	for (auto it = m_shm_ctx->m_receivers.begin (); it != m_shm_ctx->m_receivers.end ();)
	{
		if (-1 == kill (it->m_pid, 0))
		{
			// this process doesnot exist, so remove this receiver
			YAIL_LOG_DEBUG ("removed: " << it->m_uuid << "," << it->m_pid);
			it = m_shm_ctx->m_receivers.erase (it);
		}
		else
		{
			++it;
		}
	}
}

void shmem_impl::channel_map::remove_receiver (const std::string &uuid)
{
	YAIL_LOG_TRACE (uuid);

	scoped_lock<interprocess_mutex> lock(m_shm_ctx->m_mutex);
	for (auto it = m_shm_ctx->m_receivers.begin (); it != m_shm_ctx->m_receivers.end ();)
	{
		if (!strcmp (uuid.c_str (), it->m_uuid.c_str ()))
		{
			YAIL_LOG_DEBUG ("removed: " << it->m_uuid << "," << it->m_pid);
			m_shm_ctx->m_receivers.erase (it);
			break;
		}
		else
		{
			++it;
		}
	}
}


shmem_impl::channel_map::receiver_uuids shmem_impl::channel_map::get_receivers () const
{
	YAIL_LOG_TRACE (this);

	receiver_uuids uuids;
	
	scoped_lock<interprocess_mutex> lock(m_shm_ctx->m_mutex);
	for (auto &receiver : m_shm_ctx->m_receivers)
	{
		uuids.push_back (receiver.m_uuid.c_str ());
	}

	return uuids;
}

//
// shmem_impl::sender::send_operation
//
shmem_impl::sender::send_operation::send_operation (const yail::buffer &buffer, const send_handler &handler) :
	m_buffer (buffer),
	m_handler (handler)
{	
	YAIL_LOG_TRACE (this);
}

shmem_impl::sender::send_operation::~send_operation ()
{
	YAIL_LOG_TRACE (this);
}

//
// shmem_impl::sender
//
shmem_impl::sender::sender (yail::io_service &io_service, const shmem_impl::channel_map &chmap) :
	m_io_service (io_service),
	m_channel_map (chmap),
	m_op_mutex (),
	m_op_available (),
	m_op_queue (),
	m_thread (&shmem_impl::sender::do_work, this),
	m_stop_work (false)
{
	YAIL_LOG_TRACE (this);
}

shmem_impl::sender::~sender ()
{
	YAIL_LOG_TRACE (this);

	try
	{
		m_stop_work = true;
		m_op_available.notify_one ();
		m_thread.join ();
	} catch (...) {}
}

void shmem_impl::sender::do_work ()
{
	YAIL_LOG_TRACE (this);

	bool stop = false;
	while (!stop)
	{
		std::unique_ptr<send_operation> op;
		{
			// wait on send operation from client
			std::unique_lock<std::mutex> lock (m_op_mutex);
			m_op_available.wait (lock, [this] () { return !m_op_queue.empty () ||  m_stop_work; });

			if (!m_stop_work)
			{
				// dequeue an operation
				op = std::move(m_op_queue.front ());
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
				auto receivers = m_channel_map.get_receivers ();
				for (auto &uuid : receivers)
				{
					YAIL_LOG_TRACE ("sending to = " << uuid);

					// send data to receiver's mq
					boost::interprocess::message_queue mq (open_only, uuid.c_str ());
					ptime abs_time (second_clock::universal_time() + seconds(10));
					if (!mq.timed_send(op->m_buffer.data (), op->m_buffer.size (), 0, abs_time))
					{
						YAIL_LOG_WARNING ("receiver: " << uuid << " queue is full");
					}
				}

				// complete operation with success. this is best effort transport
				// we don't error if unable to send to one or more receivers.
				m_io_service.post (std::bind (op->m_handler, yail::pubsub::error::success));
			}
			catch (const interprocess_exception &ex)
			{
				YAIL_LOG_ERROR ("interprocess error: " << ex.what ());

				complete_ops_with_error (yail::pubsub::error::system_error);

				// we can't recover from this here. cease operation.
				stop = true;
			}
		}
	}
}

void shmem_impl::sender::complete_ops_with_error (const boost::system::error_code &ec)
{
	YAIL_LOG_TRACE (this);

	while (!m_op_queue.empty ())
	{
		m_io_service.post ([this, ec] () {
			auto op = std::move (m_op_queue.front ());
			m_op_queue.pop ();
			op->m_handler (ec);
		});
	}
}

//
// receiver::receive_operation
//
shmem_impl::receiver::receive_operation::receive_operation (yail::buffer &buffer, const receive_handler &handler) :
	m_buffer (buffer),
	m_handler (handler)
{
	YAIL_LOG_TRACE (this);
}

shmem_impl::receiver::receive_operation::~receive_operation ()
{
	YAIL_LOG_TRACE (this);
}

//
// shmem_impl::receiver
//
shmem_impl::receiver::receiver (yail::io_service &io_service, shmem_impl::channel_map &chmap) :
	m_io_service (io_service),
	m_channel_map (chmap),
	m_uuid (boost::uuids::random_generator()()),
	m_mq (create_only, boost::uuids::to_string (m_uuid).c_str (), YAIL_PUBSUB_SHMEM_RECEIVER_QUEUE_DEPTH, YAIL_PUBSUB_MAX_MSG_SIZE),
	m_op_queue (),
	m_thread (&shmem_impl::receiver::do_work, this),
	m_stop_work (false)
{
	YAIL_LOG_TRACE (this);

	m_channel_map.add_receiver (boost::uuids::to_string (m_uuid));
}

shmem_impl::receiver::~receiver ()
{
	YAIL_LOG_TRACE (this);

	try 
	{
		// A hack to break out of blocking mq receive
		uint8_t dummy;
		m_mq.send (&dummy, 0, 0);

		m_stop_work = true;
		m_thread.join ();

		m_channel_map.remove_receiver (boost::uuids::to_string (m_uuid));

		message_queue::remove(boost::uuids::to_string (m_uuid).c_str ());
	} 
	catch (...) {};
}

void shmem_impl::receiver::do_work ()
{
	YAIL_LOG_TRACE (this);

	bool stop = false;
	while (!stop)
	{
		try
		{
			auto pbuf (std::make_shared<yail::buffer> (YAIL_PUBSUB_MAX_MSG_SIZE));
			message_queue::size_type recvd_size; unsigned int priority;
			m_mq.receive(pbuf->data (), pbuf->size (), recvd_size, priority);
			if (recvd_size)
			{
				// resize buffer based on data received
				pbuf->resize (recvd_size);
				// completion operation if one is pending, otherwise enqueue received message.
				m_io_service.post ([this, pbuf] () {
					if (!m_op_queue.empty())
					{
						auto op = std::move (m_op_queue.front ());
						m_op_queue.pop ();

						op->m_buffer = std::move (*pbuf);
						op->m_handler (yail::pubsub::error::success);
					}
					else
					{
						m_buffer_queue.push (std::move(*pbuf));
					}
				});
			}
			else
			{
				stop = true;
			}
		}
		catch (const interprocess_exception &ex)
		{
			YAIL_LOG_ERROR ("interprocess error: " << ex.what ());

			complete_ops_with_error (yail::pubsub::error::system_error);

			// we can't recover from this here. cease operation.
			stop = true;
		}
		catch (const std::bad_alloc &ex)
		{
			YAIL_LOG_ERROR ("receive buffer allocation error: " << ex.what ());

			// assume temporary resource unavailability..just delay resuming operation
			sleep (1);
		}
	}
}

void shmem_impl::receiver::complete_ops_with_error (const boost::system::error_code &ec)
{
	YAIL_LOG_TRACE (this);

	while (!m_op_queue.empty ())
	{
		m_io_service.post ([this, ec] () {
			auto op = std::move (m_op_queue.front ());
			m_op_queue.pop ();
			op->m_handler (ec);
		});
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
	YAIL_LOG_TRACE (this);
}

shmem_impl::~shmem_impl ()
{
	YAIL_LOG_TRACE (this);
}

} // namespace detail
} // namespace transport
} // namespace pubsub
} // namespace yail

