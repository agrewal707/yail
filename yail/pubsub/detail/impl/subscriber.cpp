#include <yail/pubsub/detail/subscriber.h>

#include <yail/log.h>

namespace yail {
namespace pubsub {
namespace detail {

//
// subscriber_common::receive_operation
//
subscriber_common::receive_operation::receive_operation (std::string &topic_data, type t) :
	m_topic_data (topic_data),
	m_type (t)
{}

subscriber_common::receive_operation::~receive_operation ()
{}


//
// subscriber_common::sync_receive_operation
//
subscriber_common::sync_receive_operation::sync_receive_operation (std::string &topic_data, boost::system::error_code &ec) :
	receive_operation(topic_data, SYNC),
	m_ec (ec),
	m_mutex (),
	m_cond_done (),
	m_done (false)
{}

subscriber_common::sync_receive_operation::~sync_receive_operation ()
{}


//
// subscriber_common::async_receive_operation
//
subscriber_common::async_receive_operation::async_receive_operation (std::string &topic_data, const receive_handler &handler) :
	receive_operation(topic_data, ASYNC),
	m_handler (handler)	
{}

subscriber_common::async_receive_operation::~async_receive_operation ()
{}


//
// subscriber_common::dr_ctx
//
subscriber_common::dr::dr (const std::string &topic_name, const std::string &topic_type_name) :
	m_topic_name (topic_name),
	m_topic_type_name (topic_type_name)
{}

subscriber_common::dr::~dr ()
{}

//
// subscriber_common
//
subscriber_common::subscriber_common (yail::io_service &io_service, const std::string &domain) :
	m_io_service (io_service),
	m_domain (domain)
{}

subscriber_common::~subscriber_common ()
{
	m_topic_map.clear ();
}

bool subscriber_common::add_data_reader (const void *id, const std::string &topic_name,
	const std::string &topic_type_name, std::string &topic_id)
{
	bool retval = false;

	// set topic id for this data reader
	topic_id = m_domain+topic_name+topic_type_name;

	// create data reader ctx
	auto drctx (yail::make_unique<dr> (topic_name, topic_type_name));

	auto it = m_topic_map.find (topic_id);
	if (it == m_topic_map.end ())
	{
		// create and add data reader map to topic map
		auto drmap (yail::make_unique<dr_map> ());
		auto result = m_topic_map.insert (std::make_pair (topic_id, std::move (drmap)));
		if (!result.second)
		{
			YAIL_THROW_EXCEPTION (
				yail::system_error, "failed to add data reader map for " + topic_id, 0);
		}
		it = result.first;
		retval = true;
	}

	// add data reader ctx to data reader map
	auto &drmap = it->second;
	auto result = drmap->insert (std::make_pair (id, std::move(drctx)));
	if (!result.second)
	{
		YAIL_THROW_EXCEPTION (
			yail::system_error, "failed to add data reader for " + topic_id, 0);
	}

	return retval;
}

bool subscriber_common::remove_data_reader (const void *id, const std::string &topic_id)
{
	bool retval = false;

	// lookup data reader map
	auto it = m_topic_map.find (topic_id);
	if (it != m_topic_map.end ())
	{
		auto &drmap = it->second;

		// look up data reader ctx
		auto it2 = drmap->find (id);
		if (it2 != drmap->end ())
		{
			drmap->erase (it2);
		}

		if (drmap->empty ())
		{
			m_topic_map.erase (it);
			retval = true;
		}
	}

	return retval;
}

void subscriber_common::cancel (const void *id, const std::string &topic_id)
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
			
			std::lock_guard<std::mutex> oq_lock (drctx->m_op_queue_mutex);
			while (!drctx->m_op_queue.empty ())
			{
				auto op = drctx->m_op_queue.front ();
				drctx->m_op_queue.pop ();
				
				if (op->is_async ())
				{
					auto async_op = static_cast<async_receive_operation*> (op.get ());
					m_io_service.post (std::bind (async_op->m_handler, boost::asio::error::operation_aborted));
				}
			}
		}
	}
}

void subscriber_common::process_pubsub_message (const yail::buffer &buffer)
{
	messages::pubsub msg;
	if (msg.ParseFromArray (buffer.data (), buffer.size ()))
	{
		if (msg.header ().version () != 1)
		{
			YAIL_LOG_WARNING ("invalid pubsub message version");
			return;
		}

		if (msg.header ().type () != messages::pubsub_header::DATA)
		{
			YAIL_LOG_WARNING ("unknown pubsub message received");
			return;
		}

		process_pubsub_data (msg.data ());
	}
	else
	{
		YAIL_LOG_WARNING ("unable to parse pubsub message");
	}
}

void subscriber_common::process_pubsub_data (const messages::pubsub_data &data)
{
	std::string topic_id (m_domain + data.topic_name () + data.topic_type_name ());

	std::unique_lock<std::mutex> lock(m_topic_map_mutex);

	// lookup data reader map
	auto it = m_topic_map.find (topic_id);
	if (it != m_topic_map.end ())
	{
		auto &drmap = it->second;
		
		for (auto &val : *drmap)
		{
			auto &drctx = val.second;
			
			std::unique_lock<std::mutex> oq_lock (drctx->m_op_queue_mutex);
			if (!drctx->m_op_queue.empty ())
			{
				auto op = drctx->m_op_queue.front ();
				drctx->m_op_queue.pop ();
				oq_lock.unlock ();
				
				op->m_topic_data = data.topic_data ();
				
				if (op->is_async ())
				{
					auto async_op = static_cast<async_receive_operation*> (op.get ());
					m_io_service.post (std::bind (async_op->m_handler, yail::pubsub::error::success));
				}
				else
				{
					auto sync_op = static_cast<sync_receive_operation*> (op.get ());
					std::lock_guard<std::mutex> l (sync_op->m_mutex);
					if (!sync_op->m_done)
					{
						sync_op->m_ec = yail::pubsub::error::success;
						sync_op->m_done = true;
						sync_op->m_cond_done.notify_one ();
					}
				}
			}
			else
			{
				oq_lock.unlock ();
				
				std::lock_guard<std::mutex> dq_lock (drctx->m_data_queue_mutex);
				drctx->m_data_queue.push (data.topic_data ());
			}
		}
	}
}

void subscriber_common::complete_ops_with_error (const boost::system::error_code &ec)
{
	YAIL_LOG_FUNCTION (this);

	std::unique_lock<std::mutex> lock(m_topic_map_mutex);

	for (auto &val : m_topic_map)
	{
		auto &drmap = val.second;
		for (auto &val2 : *drmap)
		{
			auto &drctx = val2.second;
			
			std::lock_guard<std::mutex> oq_lock (drctx->m_op_queue_mutex);
			while (!drctx->m_op_queue.empty ())
			{
				auto op = std::move (drctx->m_op_queue.front ());
				drctx->m_op_queue.pop ();
				
				if (op->is_async ())
				{
					auto async_op = static_cast<async_receive_operation*> (op.get ());
					m_io_service.post (std::bind (async_op->m_handler, ec));
				}
				else
				{
					auto sync_op = static_cast<sync_receive_operation*> (op.get ());
					if (!sync_op->m_done)
					{
						sync_op->m_ec = ec;
						sync_op->m_done = true;
						sync_op->m_cond_done.notify_one ();
					}
				}
			}
		}
	}
}

} // namespace detail
} // namespace pusub
} // namespace yail

