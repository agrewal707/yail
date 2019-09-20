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
// subscriber_common::topic
//
subscriber_common::topic::topic (const topic_info &topic_info) :
	m_topic_info (topic_info)
{}

subscriber_common::topic::~topic ()
{}

//
// subscriber_common
//
subscriber_common::subscriber_common (
	yail::io_service &io_service,
	const std::string &domain,
	const notify_handler &handler) :
	m_io_service (io_service),
	m_domain (domain),
	m_notify_handler (handler)
{}

subscriber_common::~subscriber_common ()
{
	m_topic_map.clear ();
}

bool subscriber_common::add_data_reader (const void *id, const topic_info &topic_info, std::string &topic_id)
{
	bool retval = false;

	// set topic id for this data reader
	topic_id = m_domain+topic_info.m_name+topic_info.m_type_name;

	auto it = m_topic_map.find (topic_id);
	if (it == m_topic_map.end ())
	{
		// create and add topic context to topic map
		auto tctx (yail::make_unique<topic> (topic_info));
		auto result = m_topic_map.emplace (topic_id, std::move (tctx));
		if (!result.second)
		{
			YAIL_THROW_EXCEPTION (
				yail::system_error, "failed to add topic context for " + topic_id, 0);
		}
		it = result.first;
		retval = true;
	}

	// create data reader ctx
	auto drctx (yail::make_unique<dr> ());

	// add data reader ctx to data reader map in topic context
	auto &tctx = it->second;
	auto result = tctx->m_dr_map.emplace (id, std::move(drctx));
	if (!result.second)
	{
		YAIL_THROW_EXCEPTION (
			yail::system_error, "failed to add data reader for " + topic_id, 0);
	}

	if (!topic_info.is_builtin () && topic_info.is_durable ())
	{
		messages::subscription sub;
		if (construct_subscription(topic_info, sub))
		{
			m_io_service.post(std::bind (m_notify_handler, sub));
		}
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
		auto &tctx = it->second;

		// look up data reader ctx
		auto it2 = tctx->m_dr_map.find (id);
		if (it2 != tctx->m_dr_map.end ())
		{
			tctx->m_dr_map.erase (it2);
		}

		if (tctx->m_dr_map.empty ())
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
		auto &tctx = it->second;

		// look up data reader ctx
		auto it2 = tctx->m_dr_map.find (id);
		if (it2 != tctx->m_dr_map.end ())
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

bool subscriber_common::construct_subscription (
	const topic_info &topic_info,
	messages::subscription &sub)
{
	bool retval = false;

	try
	{
		sub.set_domain (m_domain);
		sub.set_topic_name (topic_info.m_name);
		sub.set_topic_type_name (topic_info.m_type_name);
		retval = true;
	}
	catch (const std::exception &ex)
	{
		YAIL_LOG_WARNING (ex.what ());
	}

	return retval;
}

void subscriber_common::process_pubsub_message (const yail::buffer &buffer)
{
	messages::pubsub msg;
	if (msg.ParseFromArray (buffer.data (), buffer.size ()))
	{
		if (msg.header ().version () != messages::pubsub_header::VERSION_1)
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
	std::string topic_id (data.domain () + data.topic_name () + data.topic_type_name ());

	std::unique_lock<std::mutex> lock(m_topic_map_mutex);

	// lookup data reader map
	auto it = m_topic_map.find (topic_id);
	if (it != m_topic_map.end ())
	{
		auto &tctx = it->second;

		// copy received data to all data readers for this topic
		for (auto &val : tctx->m_dr_map)
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
		auto &tctx = val.second;
		for (auto &val2 : tctx->m_dr_map)
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
