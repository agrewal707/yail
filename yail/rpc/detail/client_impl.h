#ifndef YAIL_RPC_DETAIL_CLIENT_IMPL_H
#define YAIL_RPC_DETAIL_CLIENT_IMPL_H

#include <yail/rpc/client.h>

#include <yail/rpc/error.h>
#include <yail/rpc/detail/service_impl.h>
#include <yail/rpc/detail/rpc_impl.h>
#include <yail/rpc/detail/client.h>

namespace yail {
namespace rpc {
namespace detail {

template <typename Transport>
class client_impl
{
public:
	client_impl (service_impl<Transport> &service);
	~client_impl ();

	template <typename Request, typename Response>
	void call (const std::string &service_name, const rpc_impl<Request, Response> &service_rpc, 
	           const Request &req, Response &res, boost::system::error_code &ec);

	template <typename Request, typename Response, typename Handler>
	void async_call (const std::string &service_name, const rpc_impl<Request, Response> &service_rpc, 
	                 const Request &req, Response &res, const Handler &handler);

private:
	template <typename Request, typename Response, typename Handler>
	struct call_operation
	{
		call_operation (const rpc_impl<Request, Response> &service_rpc, Response &res, const Handler &handler);
		~call_operation ();

		const rpc_impl<Request, Response> &m_service_rpc;
		Response &m_res;
		Handler m_handler;
		std::string m_res_data;
	};

	service_impl<Transport> &m_service;
};

} // namespace detail
} // namespace rpc
} // namespace yail

//
// yail::detail::client_impl template implementation
//
namespace yail {
namespace rpc {
namespace detail {

//
// client_impl::call_operation
//
template <typename Transport>
template <typename Request, typename Response, typename Handler>
client_impl<Transport>::call_operation<Request, Response, Handler>::call_operation (
		const rpc_impl<Request, Response> &service_rpc, Response &res, const Handler &handler) :
	m_service_rpc (service_rpc),
	m_res (res),
	m_handler (handler)
{
	YAIL_LOG_FUNCTION (this);
}

template <typename Transport>
template <typename Request, typename Response, typename Handler>
client_impl<Transport>::call_operation<Request, Response, Handler>::~call_operation ()
{
	YAIL_LOG_FUNCTION (this);
}

//
// client_impl
//
template <typename Transport>
client_impl<Transport>::client_impl (service_impl<Transport> &service) :
	m_service (service)
{
	YAIL_LOG_FUNCTION (this);
}

template <typename Transport>
client_impl<Transport>::~client_impl ()
{
	YAIL_LOG_FUNCTION (this);
}

template <typename Transport>
template <typename Request, typename Response>
void client_impl<Transport>::call (const std::string &service_name, const rpc_impl<Request, Response> &service_rpc, 
                                   const Request &req, Response &res, boost::system::error_code &ec)
{
	std::string req_data;
	if (service_rpc.serialize (req, &req_data))
	{
		std::string res_data;
		m_service.get_client ().call (service_name, service_rpc.get_name (), service_rpc.get_type_name (), req_data, res_data, ec);
		if (!ec)
		{
			if (!service_rpc.deserialize (res, res_data))
			{
				ec = yail::rpc::error::deserialization_failed;
			}
		}
	}
	else
	{
		ec = yail::rpc::error::serialization_failed;
	}
}

template <typename Transport>
template <typename Request, typename Response, typename Handler>
void client_impl<Transport>::async_call (const std::string &service_name, const rpc_impl<Request, Response> &service_rpc, 
                                         const Request &req, Response &res, const Handler &handler)
{
	std::string req_data;
	if (service_rpc.serialize (req, &req_data))
	{
		auto op (std::make_shared<call_operation<Request, Response, Handler>> (service_rpc, res, handler));

		m_service.get_client ().async_call (service_name, service_rpc.get_name (), service_rpc.get_type_name (), req_data, op->m_res_data,
			[op] (const boost::system::error_code &ec)
			{
				if (!ec)
				{
					if (op->m_service_rpc.deserialize (op->m_res, op->m_res_data))
					{
						op->m_handler (yail::rpc::error::success);
					}
					else
					{
						op->m_handler (yail::rpc::error::deserialization_failed);
					}
				}
				else
				{
					op->m_handler (ec);
				}
			});
	}
	else
	{
		m_service.get_io_service ().post (
			std::bind (handler, yail::rpc::error::serialization_failed));
	}
}

} // namespace detail
} // namespace rpc
} // namespace yail

#endif // YAIL_RPC_DETAIL_CLIENT_IMPL_H
