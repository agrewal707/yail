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
// publisher_common::dw_ctx
//
publisher_common::dw::dw (const std::string &topic_name, const std::string &topic_type_name) :
	m_topic_name (topic_name),
	m_topic_type_name (topic_type_name)
{}

publisher_common::dw::~dw ()
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

void publisher_common::add_data_writer (const void *id, const std::string &topic_name,
	const std::string &topic_type_name, std::string &topic_id)
{
	// set topic id for this data writer
	topic_id = m_domain+topic_name+topic_type_name;

	// create data reader ctx
	auto dwctx (yail::make_unique<dw> (topic_name, topic_type_name));

	auto it = m_topic_map.find (topic_id);
	if (it == m_topic_map.end ())
	{
		// create and add data writer map to topic map
		auto dwmap (yail::make_unique<dw_map> ());
		auto result = m_topic_map.insert (std::make_pair (topic_id, std::move (dwmap)));
		if (!result.second)
		{
			YAIL_THROW_EXCEPTION (
				yail::system_error, "failed to add data writer map for " + topic_id, 0);
		}
		it = result.first;
	}

	// add data writer ctx to data writer map
	auto &dwmap = it->second;
	auto result = dwmap->insert (std::make_pair (id, std::move(dwctx)));
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
		auto &dwmap = it->second;

		// look up data writer ctx
		auto it2 = dwmap->find (id);
		if (it2 != dwmap->end ())
		{
			dwmap->erase (it2);
		}

		if (dwmap->empty ())
		{
			m_topic_map.erase (it);
		}
	}
}

bool publisher_common::construct_pubsub_message (const std::string &topic_name,
	const std::string &topic_type_name, const std::string &topic_data, yail::buffer &buffer)
{
	bool retval = false;

	try
	{
		// header
		auto hdr (yail::make_unique<messages::pubsub_header> ());
		hdr->set_version (1);
		hdr->set_type (messages::pubsub_header::DATA);
		hdr->set_id (m_mid++);

		// data
		auto data (yail::make_unique<messages::pubsub_data> ());
		data->set_domain (m_domain);
		data->set_topic_name (topic_name);
		data->set_topic_type_name (topic_type_name);
		data->set_topic_data (topic_data);

		messages::pubsub msg;
		msg.set_allocated_header (hdr.get ());
		msg.set_allocated_data (data.get ());

		buffer.resize (msg.ByteSize ());
		YAIL_LOG_FUNCTION ("message size = " << buffer.size ());
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

