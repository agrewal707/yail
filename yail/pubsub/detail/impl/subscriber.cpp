#include <yail/pubsub/detail/subscriber.h>

#include <yail/log.h>

namespace yail {
namespace pubsub {
namespace detail {

//
// subscriber_common::receive_operation
//
subscriber_common::receive_operation::receive_operation (std::string &topic_data, const receive_handler &handler) :
	m_topic_data (topic_data),
	m_handler (handler)
{}

subscriber_common::receive_operation::~receive_operation ()
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

void subscriber_common::add_data_reader (const void *id, const std::string &topic_name,
	const std::string &topic_type_name, std::string &topic_id)
{
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
	}

	// add data reader ctx to data reader map
	auto &drmap = it->second;
	auto result = drmap->insert (std::make_pair (id, std::move(drctx)));
	if (!result.second)
	{
		YAIL_THROW_EXCEPTION (
			yail::system_error, "failed to add data reader for " + topic_id, 0);
	}
}

void subscriber_common::remove_data_reader (const void *id, const std::string &topic_id)
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
			drmap->erase (it2);
		}

		if (drmap->empty ())
		{
			m_topic_map.erase (it);
		}
	}
}

void subscriber_common::process_pubsub_message (const yail::buffer &buffer)
{
	messages::pubsub msg;
	if (msg.ParseFromArray (buffer.data (), buffer.size ()))
	{
		if (msg.header ().type () == messages::pubsub_header::DATA)
		{
			process_pubsub_data (msg.data ());
		}
		else
		{
			YAIL_LOG_WARNING ("unknown pubsub message received");
		}
	}
	else
	{
		YAIL_LOG_WARNING ("unable to parse pubsub message");
	}
}

void subscriber_common::process_pubsub_data (const messages::pubsub_data &data)
{
	std::string topic_id (m_domain + data.topic_name () + data.topic_type_name ());

	// lookup data reader map
	auto it = m_topic_map.find (topic_id);
	if (it != m_topic_map.end ())
	{
		auto &drmap = it->second;

		for (auto &val : *drmap)
		{
			auto &drctx = val.second;
			if (!drctx->m_op_queue.empty ())
			{
				auto op = std::move (drctx->m_op_queue.front ());
				drctx->m_op_queue.pop ();

				op->m_topic_data = data.topic_data ();
				op->m_handler (yail::pubsub::error::success);
			}
			else
			{
				drctx->m_data_queue.push (data.topic_data ());
			}
		}
	}
}

} // namespace detail
} // namespace pusub
} // namespace yail

