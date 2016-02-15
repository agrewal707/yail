#ifndef YAIL_RPC_DETAIL_CLIENT_H
#define YAIL_RPC_DETAIL_CLIENT_H

#include <string>
#include <yail/buffer.h>
#include <yail/rpc/error.h>
#include <yail/rpc/detail/service_locator.h>

namespace yail {
namespace rpc {
namespace detail {

//
// Transport independent client functionality
//
struct client_common
{
	client_common ();
	~client_common ();

	/// construct rpc request message
	YAIL_API bool construct_rpc_request (const std::string &service_name, 
		const std::string &rpc_name, const std::string &rpc_type_name, const std::string &req_data, 
		uint32_t &req_id, yail::buffer &req_buffer);

	// process rpc response message
	YAIL_API bool process_rpc_response (const std::string &service_name, 
		const std::string &rpc_name, const std::string &rpc_type_name, const uint32_t req_id, const yail::buffer &res_buffer, 
		bool &res_status, std::string &res_data);

	template <typename Handler>
	struct call_operation
	{
		YAIL_API call_operation (const std::string &service_name, 
			const std::string &rpc_name, const std::string &rpc_type_name, std::string &res_data, const Handler &handler);
		YAIL_API ~call_operation ();

		std::string m_service_name;
		std::string m_rpc_name;
		std::string m_rpc_type_name;
		std::string &m_res_data;
		Handler m_handler;
		uint32_t m_req_id;
		yail::buffer m_req_buffer;
		yail::buffer m_res_buffer;
	};

	uint32_t m_id;
};

//
// client
//
template <typename Transport>
class client : private client_common
{
public:
	client (service_locator<Transport> &service_locator, Transport &transport);
	~client ();

	/// Call rpc synchronously
	void call (const std::string &service_name, const std::string &rpc_name, const std::string &rpc_type_name, 
	           const std::string &req_data, std::string &res_data, boost::system::error_code &ec, const uint32_t timeout)
	{
		uint32_t req_id; yail::buffer req_buffer;
		if (construct_rpc_request (service_name, rpc_name, rpc_type_name, 
		                           req_data, req_id, req_buffer))
		{
			yail::buffer res_buffer;
			const auto ep = m_service_locator.get_service_location (service_name);
			m_transport.client_send_n_receive (ep, req_buffer, res_buffer, ec, timeout);
			if (!ec)
			{
				bool res_status;
				if (process_rpc_response (service_name, rpc_name, rpc_type_name, 
				                          req_id, res_buffer, res_status, res_data))
				{
					if (res_status)
						ec = yail::rpc::error::success;
					else
						ec = yail::rpc::error::failure_response;
				}
				else
				{
					ec = yail::rpc::error::invalid_response;
				}
			}
		}
	}

	/// Call rpc asynchronously
	template <typename Handler>
	void async_call (const std::string &service_name, const std::string &rpc_name, const std::string &rpc_type_name, 
	                 const std::string &req_data, std::string &res_data, const Handler &handler)
	{
		auto op (std::make_shared<call_operation<Handler>> (service_name, rpc_name, rpc_type_name, res_data, handler));

		if (construct_rpc_request (op->m_service_name, op->m_rpc_name, op->m_rpc_type_name, 
		                           req_data, op->m_req_id, op->m_req_buffer))
		{
			const auto ep = m_service_locator.get_service_location (service_name);
			m_transport.async_client_send_n_receive (ep, op->m_req_buffer, op->m_res_buffer,
				[this, op] (const boost::system::error_code &ec)
				{
					if (!ec)
					{
						bool res_status;
						if (process_rpc_response (op->m_service_name, op->m_rpc_name, op->m_rpc_type_name, 
						                          op->m_req_id, op->m_res_buffer, res_status, op->m_res_data))
						{
							if (res_status)
								op->m_handler (yail::rpc::error::success);
							else
								op->m_handler (yail::rpc::error::failure_response);
						}
						else
						{
							op->m_handler (yail::rpc::error::invalid_response);
						}
					}
				});
		}
	}

private:
	service_locator<Transport> &m_service_locator;
	Transport &m_transport;
};

} // namespace detail
} // namespace rpc
} // namespace yail

namespace yail {
namespace rpc {
namespace detail {

//
// client::call_operation
//
template <typename Handler>
client_common::call_operation<Handler>::call_operation (const std::string &service_name, 
		const std::string &rpc_name, const std::string &rpc_type_name, std::string &res_data, const Handler &handler):
	m_service_name (service_name),
	m_rpc_name (rpc_name),
	m_rpc_type_name (rpc_type_name),
	m_res_data (res_data),
	m_handler (handler),
	m_req_id (0),
	m_req_buffer ()
{
	YAIL_LOG_FUNCTION (this << m_service_name << m_rpc_name << m_rpc_type_name << m_req_id);
}

template <typename Handler>
client_common::call_operation<Handler>::~call_operation ()
{
	YAIL_LOG_FUNCTION (this << m_service_name << m_rpc_name << m_rpc_type_name << m_req_id);
}

//
// client
//
template <typename Transport>
client<Transport>::client (service_locator<Transport> &service_locator, Transport &transport) :
	m_service_locator (service_locator),
	m_transport (transport)
{
	YAIL_LOG_FUNCTION (this);
}

template <typename Transport>
client<Transport>::~client ()
{
	YAIL_LOG_FUNCTION (this);
}

} // namespace detail
} // namespace rpc
} // namespace yail

#endif // YAIL_RPC_DETAIL_CLIENT_H
