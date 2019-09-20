#include <yail/pubsub/detail/publisher.h>

#include <yail/log.h>

namespace yail {
namespace pubsub {
namespace detail {

//
// publisher_common::send_operation
//
publisher_common::send_operation::send_operation (const send_handler &handler) :
	m_handler (handler),
	m_buffer (YAIL_PUBSUB_MAX_MSG_SIZE)
{}

publisher_common::send_operation::~send_operation ()
{}

//
// publisher_common::topic
//
publisher_common::topic::topic (const topic_info &topic_info) :
	m_topic_info (topic_info)
{

	if (topic_info.m_qos.m_durability.m_type == topic_qos::durability::TRANSIENT_LOCAL)
	{
		m_data_ring.set_capacity (topic_info.m_qos.m_durability.m_depth);
	}
}

publisher_common::topic::~topic ()
{}

//
// publisher_common
//
publisher_common::publisher_common (yail::io_service &io_service, const std::string &domain) :
	m_io_service (io_service),
	m_domain (domain),
	m_mid (0)
{}

publisher_common::~publisher_common ()
{
	m_topic_map.clear ();
}

void publisher_common::add_data_writer (const void *id, const topic_info &topic_info, std::string &topic_id)
{
	// set topic id for this data writer
	topic_id = m_domain+topic_info.m_name+topic_info.m_type_name;

	auto it = m_topic_map.find (topic_id);
	if (it == m_topic_map.end ())
	{
		// create and add data writer map to topic map
		auto tctx (yail::make_unique<topic> (topic_info));
		auto result = m_topic_map.emplace (topic_id, std::move (tctx));
		if (!result.second)
		{
			YAIL_THROW_EXCEPTION (
				yail::system_error, "failed to add topic context for " + topic_id, 0);
		}
		it = result.first;
	}

	// create dw ctx
	auto dwctx (std::make_shared<dw> ());

	// add data writer ctx to data writer map
	auto &tctx = it->second;
	auto result = tctx->m_dw_map.emplace (id, dwctx);
	if (!result.second)
	{
		YAIL_THROW_EXCEPTION (
			yail::system_error, "failed to add data writer for " + topic_id, 0);
	}
}

void publisher_common::remove_data_writer (const void *id, const std::string &topic_id)
{
	// lookup data writer map
	auto it = m_topic_map.find (topic_id);
	if (it != m_topic_map.end ())
	{
		auto &tctx = it->second;

		// look up data writer ctx
		auto it2 = tctx->m_dw_map.find (id);
		if (it2 != tctx->m_dw_map.end ())
		{
			tctx->m_dw_map.erase (it2);
		}

		if (tctx->m_dw_map.empty ())
		{
			m_topic_map.erase (it);
		}
	}
}

std::weak_ptr<publisher_common::dw>
publisher_common::build_data_message (
	const void *id,
	const std::string &topic_id,
	const std::string &topic_data,
	yail::buffer &buffer,
	boost::system::error_code &ec)
{
	std::weak_ptr<dw> retval;

	// A performance optimization is possible by maintaining
	// separate mutex for topic_map and dw_map, but we keep things
	// simple by maintaining a single mutex.
	std::lock_guard<std::mutex> lock (m_topic_map_mutex);

	// lookup topic context
	auto it = m_topic_map.find (topic_id);
	if (it != m_topic_map.end ())
	{
		auto &tctx = it->second;

		// look up data writer ctx
		auto it2 = tctx->m_dw_map.find (id);
		if (it2 != tctx->m_dw_map.end ())
		{
			messages::pubsub_data data;
			if (construct_pubsub_data (tctx->m_topic_info, topic_data, data))
			{
				if (construct_pubsub_message (data, buffer))
				{
					tctx->m_data_ring.push_back (data);
					retval = it2->second;
					ec = boost::system::error_code ();
				}
				else
				{
					ec = yail::pubsub::error::system_error;
				}
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

	return retval;
}

bool publisher_common::construct_pubsub_data (
	const topic_info &topic_info,
	const std::string &topic_data,
	messages::pubsub_data &data)
{
	bool retval = false;

	try
	{
		data.set_domain (m_domain);
		data.set_topic_name (topic_info.m_name);
		data.set_topic_type_name (topic_info.m_type_name);
		data.set_topic_data (topic_data);
		retval = true;
	}
	catch (const std::exception &ex)
	{
		YAIL_LOG_WARNING (ex.what ());
	}

	return retval;
}

bool publisher_common::construct_pubsub_message (
	const messages::pubsub_data &d,
	yail::buffer &buffer)
{
	bool retval = false;

	try
	{
		// header
		auto hdr (yail::make_unique<messages::pubsub_header> ());
		hdr->set_version (messages::pubsub_header::VERSION_1);
		hdr->set_type (messages::pubsub_header::DATA);
		hdr->set_id (m_mid++);

		// data
		auto data (yail::make_unique<messages::pubsub_data> (d));

		messages::pubsub msg;
		msg.set_allocated_header (hdr.get ());
		msg.set_allocated_data (data.get ());

		buffer.resize (msg.ByteSize ());
		YAIL_LOG_TRACE ("message size = " << buffer.size ());
		msg.SerializeToArray (buffer.data (), buffer.size ());

		msg.release_header ();
		msg.release_data ();

		retval = true;
	}
	catch (const std::exception &ex)
	{
		YAIL_LOG_WARNING (ex.what ());
	}

	return retval;
}

} // namespace detail
} // namespace pubsub
} // namespace yail
