#include <yail/rpc/detail/server.h>

#include <yail/log.h>
#include <yail/rpc/detail/messages/yail.pb.h>

namespace yail {
namespace rpc {
namespace detail {

YAIL_DEFINE_EXCEPTION (duplicate_rpc);
YAIL_DEFINE_EXCEPTION (rpc_mismatch);

//
// trans_context_impl
//
trans_context_impl::trans_context_impl (const void *trctx, const rpc_context *rctx, const int req_id):
	m_trctx (trctx),
	m_rctx (rctx),
	m_req_id (req_id)
{}

trans_context_impl::~trans_context_impl ()
{}

//
// rpc_context
//
rpc_context::rpc_context (const std::string &service_name, const std::string &rpc_name, const std::string &rpc_type_name, 
							 const rpc_handler &handler):
	m_service_name (service_name),
	m_rpc_name (rpc_name),
	m_rpc_type_name (rpc_type_name),
	m_rpc_handler (handler),
	m_trans_context_map ()
{}

rpc_context::~rpc_context ()
{}

//
// server_common
//
server_common::server_common () :
	m_rpc_map ()
{}

server_common::~server_common ()
{}

bool server_common::validate_rpc_response (yail::rpc::trans_context &tctx,
	const std::string &service_name, const std::string &rpc_name, const std::string &rpc_type_name)
{
	return (tctx->m_rctx->m_service_name == service_name) && 
	       (tctx->m_rctx->m_rpc_name == rpc_name) && 
				 (tctx->m_rctx->m_rpc_type_name == rpc_type_name);
}

bool server_common::construct_rpc_response (yail::rpc::trans_context &tctx, 
	const std::string &service_name, const std::string &rpc_name, const std::string &rpc_type_name, 
	const bool res_status const std::string &res_data, yail::buffer &res_buffer)
{
	bool retval = false;

	try
	{
		// common
		auto com (yail::make_unique<messages::rpc_common> ());
		com->set_version (1);
		com->set_id (tctx.m_req_id);
		com->set_service_name (service_name);
		com->set_rpc_name (rpc_name);
		com->set_rpc_type_name (rpc_type_name);
	
		// response
		messages::rpc_response msg;
		msg.set_allocated_common (com.get ());
		msg.set_status (res_status);
		msg.set_data (res_data);

		res_buffer.resize (msg.ByteSize ());
		YAIL_LOG_TRACE ("message size = " << res_buffer.size ());
		msg.SerializeToArray (res_buffer.data (), res_buffer.size ());

		msg.release_common ();

		retval = true;
	}
	catch (const std::exception &ex)
	{
		YAIL_LOG_WARNING (ex.what ());
	}

	return retval;
}

/// processs rpc message received on the transport channel
void server_common::process_rpc_request (void *trctx, std::shared_ptr<yail::buffer> req_buffer)
{
	messages::rpc_request msg;
	if (msg.ParseFromArray (req_buffer->data (), req_buffer->size ()))
	{
		if (msg.common ().version () != 1)
		{
			YAIL_LOG_WARNING ("invalid rpc message version");
			goto exit;
		}

		std::string rpc_id (msg.common ().service_name () + msg.common ().rpc_name () +  msg.common ().rpc_type_name ());
		const auto it = m_rpc_map.find (rpc_id);
		if (it != m_rpc_map.end ())
		{
			auto &rctx = it->second;

			// allocate transaction context
			auto tctx (std::make_unique<trans_context> (trctx, rctx.get (), msg.common ().id ()));
	
			// call provider handler
			rctx->m_rpc_handler (*tctx, msg.data ());

			// add to trans context map if signaled as delayed by the provider
			if (trans_context_impl::DELAYED == tctx->m_status)
			{
				const auto result = rctx->m_trans_context_map.insert (std::make_pair (tctx.get (), std::move (tctx)));
				if (!result.second)
				{
					YAIL_LOG_WARNING ("failed to add transaction context");
				}
			}
		}
		else
		{
			YAIL_LOG_WARNING ("unknown rpc");
		}
	}
	else
	{
		YAIL_LOG_WARNING ("unable to parse rpc message");
	}
}

} // namespace detail
} // namespace pusub
} // namespace yail

