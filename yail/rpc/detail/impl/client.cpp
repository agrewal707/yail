#include <yail/rpc/detail/client.h>

#include <yail/log.h>
#include <yail/memory.h>
#include <yail/rpc/detail/messages/yail.pb.h>

namespace yail {
namespace rpc {
namespace detail {

//
// client_common
//
client_common::client_common () :
	m_id (0)
{}

client_common::~client_common ()
{}

bool client_common::construct_rpc_request (const std::string &service_name,
	const std::string &rpc_name, const std::string &rpc_type_name, const std::string &req_data, 
	uint32_t &req_id, yail::buffer &req_buffer)
{
	bool retval = false;

	try
	{
		req_id = m_id++;

		// common
		auto com (yail::make_unique<messages::rpc_common> ());
		com->set_version (1);
		com->set_id (req_id);
		com->set_service_name (service_name);
		com->set_rpc_name (rpc_name);
		com->set_rpc_type_name (rpc_type_name);
	
		// request	
		messages::rpc_request msg;
		msg.set_allocated_common (com.get ());
		msg.set_data (req_data);

		req_buffer.resize (msg.ByteSize ());
		YAIL_LOG_TRACE ("message size = " << req_buffer.size ());
		msg.SerializeToArray (req_buffer.data (), req_buffer.size ());

		msg.release_common ();

		retval = true;
	}
	catch (const std::exception &ex)
	{
		YAIL_LOG_WARNING (ex.what ());
	}

	return retval;
}

bool client_common::process_rpc_response (const std::string &service_name,
	const std::string &rpc_name, const std::string &rpc_type_name, const uint32_t req_id, const yail::buffer &res_buffer, 
	bool &res_status, std::string &res_data)
{
	bool retval = false;
	messages::rpc_response msg;
	if (msg.ParseFromArray (res_buffer.data (), res_buffer.size ()))
	{
		if (msg.common ().version () != 1)
		{
			YAIL_LOG_WARNING ("invalid rpc message version");
			goto exit;
		}
	
		if (msg.common ().id () != req_id)
		{
			YAIL_LOG_WARNING ("unknown rpc message id");
			goto exit;
		}

		if (msg.common ().service_name () != service_name)
		{
			YAIL_LOG_WARNING ("invalid service name");
			goto exit;
		}

		if (msg.common ().rpc_name () != rpc_name)
		{
			YAIL_LOG_WARNING ("invalid rpc name");
			goto exit;
		}

		if (msg.common ().rpc_type_name () != rpc_type_name)
		{
			YAIL_LOG_WARNING ("invalid rpc type name");
			goto exit;
		}

		res_status = msg.status ();
		res_data = std::move (msg.data ());
		retval = true;
	}
	else
	{
		YAIL_LOG_WARNING ("unable to parse rpc message");
	}

exit:
	return retval;
}

} // namespace detail
} // namespace rpc
} // namespace yail

