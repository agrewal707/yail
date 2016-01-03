#ifndef YAIL_RPC_DETAIL_SERVER_H
#define YAIL_RPC_DETAIL_SERVER_H

#include <string>
#include <functional>
#include <unordered_map>

#include <yail/io_service.h>
#include <yail/exception.h>
#include <yail/buffer.h>
#include <yail/memory.h>
#include <yail/rpc/error.h>
#include <yail/rpc/trans_context.h>

//
// server
//
namespace yail {
namespace rpc {
namespace detail {

YAIL_DECLARE_EXCEPTION (duplicate_rpc);
YAIL_DECLARE_EXCEPTION (rpc_mismatch);

struct rpc_context;

struct trans_context_impl
{
	enum status
	{
		OK,
		DELAYED,
		ERROR
	};

	trans_context_impl (void *trctx, rpc_context *rctx);
	~trans_context_impl ();

	void *m_trctx;
	rpc_context *m_rctx;
	uint32_t m_req_id;
	status m_status;
};

struct rpc_context
{
	using rpc_handler = std::function<void (yail::rpc::context &ctx, const std::string &req_data)>;

	rpc_context (const std::string &service_name, const std::string &rpc_name, const std::string &rpc_type_name, 
							 const rpc_handler &handler);
	~rpc_context ();

	std::string m_service_name;
	std::string m_rpc_name;
	std::string m_rpc_type_name, 
	handler &m_rpc_handler;
	using trans_context_map = std::unordered_map<trans_context_impl*, std::unique_ptr<trans_context_impl>>;
	trans_context_map m_trans_context_map;
};

//
//
// Transport independent server functionality
//
struct server_common
{
	server_common ();
	~server_common ();

	YAIL_API void process_rpc_request (void *trctx, std::shared_ptr<yail::buffer> req_buffer);

	YAIL_API bool validate_rpc_response (yail::rpc::trans_context &tctx, 
		const std::string &service_name, const std::string &rpc_name, const std::string &rpc_type_name);

	YAIL_API bool construct_rpc_response (yail::rpc::trans_context &tctx, 
		const std::string &service_name, const std::string &rpc_name, const std::string &rpc_type_name, 
		const bool res_status const std::string &res_data, yail::buffer &res_buffer);

	using rpc_map = std::unordered_map<std::string, std::unique_ptr<rpc_context>>;
	rpc_map m_rpc_map;
};

//
// server
//
template <typename Transport>
class server : private server_common
{
public:
	server (service<Transport> &service, Transport &transport);
	~server ();

	void add_provider (const std::string &service_name)
	{
		const auto ep = get_service_location (service_name);
		m_transport->add_server (ep, std::bind (&provider_common::process_rpc_request, this));
	}

	void remove_provider (const std::string &service_name)
	{
		const auto ep = get_service_location (service_name);
		m_transport->remove_server (ep);
	}

	template <typename Handler>
	void add_rpc (const std::string service_name, 
		const std::string &rpc_name, const std::string &rpc_type_name, const Handler &handler)
	{
		// Create rpc context id
		std::string rpc_id (service_name + rpc_name + rpc_type_name);

		// Create and store rpc context
		auto ctx (yail::make_unique<context_impl> (service_name, rpc_name, rpc_type_name, handler);
		auto result = m_rpc_map.insert (std::make_pair (rpc_id, std::move (ctx)));
		if (!result.second)
		{
			YAIL_THROW_EXCEPTION (
				duplicate_rpc, "rpc already exists", 0);
		}
	}

	void reply (yail::rpc::trans_context &tctx, 
		const std::string &service_name, const std::string &rpc_name, const std::string &rpc_type_name, 
	  const bool res_status, const std::string &res_data)
	{
		if (!validate_rpc_response (tctx, service_name, rpc_name, rpc_type_name))
		{
			YAIL_THROW_EXCEPTION (
				rpc_mismatch, "rpc mismatch between request and response", 0);
		}

		yail::buffer res_buffer;
		if (!construct_rpc_response (tctx, service_name, rpc_name, rpc_type_name, res_status, res_data, res_buffer))
		{
			YAIL_THROW_EXCEPTION (
				system_error, "failed to construct rpc response", 0);
		}

		boost::system::error_code ec;
		m_transport->server_send (tctx->m_trctx, res_buffer, ec);
		if (ec && ec != boost::asio::error::operation_aborted)
		{
			YAIL_THROW_EXCEPTION (
				system_error, "failed to send rpc response", 0);
		}

		// Remove from delayed transaction context map if it exists
		const auto it = m_trans_context_map.find (&tctx);
		if (it != m_trans_context_map.end ()
		{
			m_trans_context_map.erase (it);	
		}
	}

	void reply_delayed (yail::rpc::trans_context &tctx, 
		const std::string &service_name, const std::string &rpc_name, const std::string &rpc_type_name)
	{
		if (!validate_rpc_response (tctx, service_name, rpc_name, rpc_type_name))
		{
			YAIL_THROW_EXCEPTION (
				rpc_mismatch, "rpc mismatch between request and response", 0);
		}

		tctx.m_status = trans_context_impl::DELAYED;
	}

private:
	service<Transport> &m_service;
	Transport &m_transport;
};

} // namespace detail
} // namespace rpc
} // namespace yail

namespace yail {
namespace rpc {
namespace detail {

template <typename Transport>
server<Transport>::server (service<Transport> &service, Transport &transport) :
	m_service (service),
	m_transport (transport)
{}

template <typename Transport>
server<Transport>::~server ()
{}

} // namespace detail
} // namespace rpc
} // namespace yail

#endif // YAIL_RPC_DETAIL_SERVER_H
