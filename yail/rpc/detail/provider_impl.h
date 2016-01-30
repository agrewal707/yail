#ifndef YAIL_RPC_DETAIL_PROVIDER_IMPL_H
#define YAIL_RPC_DETAIL_PROVIDER_IMPL_H

#include <yail/rpc/provider.h>

#include <yail/rpc/error.h>
#include <yail/rpc/detail/service_impl.h>
#include <yail/rpc/detail/rpc_impl.h>
#include <yail/rpc/detail/server.h>

namespace yail {
namespace rpc {
namespace detail {

template <typename Transport>
class provider_impl
{
public:
	provider_impl (service_impl<Transport> &service, const std::string &service_name);
	~provider_impl ();

	template <typename Request, typename Response, typename Handler>
	void add_rpc (const rpc_impl<Request, Response> &service_rpc, const Handler &handler);

	template <typename Request, typename Response>
	void reply_ok (yail::rpc::trans_context &tctx, const rpc_impl<Request, Response> &service_rpc, const Response &res);

	template <typename Request, typename Response>
	void reply_error (yail::rpc::trans_context &tctx, const rpc_impl<Request, Response> &service_rpc, const std::string &errmsg);

	template <typename Request, typename Response>
	void reply_delayed (yail::rpc::trans_context &tctx, const rpc_impl<Request, Response> &service_rpc);

private:
	template <typename Request, typename Response, typename Handler>
	struct context
	{
		context (const rpc_impl<Request, Response> &service_rpc, const Handler &handler);
		~context ();

		const rpc_impl<Request, Response> &m_service_rpc;
		Handler m_handler;
	};

	service_impl<Transport> &m_service;
	std::string m_service_name;
};

} // namespace detail
} // namespace rpc
} // namespace yail

//
// yail::detail::provider_impl template implementation
//
namespace yail {
namespace rpc {
namespace detail {

//
// provider::context
//
template <typename Transport>
template <typename Request, typename Response, typename Handler>
provider_impl<Transport>::context<Request, Response, Handler>::context (const rpc_impl<Request, Response> &service_rpc, const Handler &handler) :
	m_service_rpc (service_rpc),
	m_handler (handler)
{
	YAIL_LOG_FUNCTION (this << m_service_rpc.get_name () << m_service_rpc.get_type_name ());
}

template <typename Transport>
template <typename Request, typename Response, typename Handler>
provider_impl<Transport>::context<Request, Response, Handler>::~context ()
{
	YAIL_LOG_FUNCTION (this);
}

//
// provider_impl
//
template <typename Transport>
provider_impl<Transport>::provider_impl (service_impl<Transport> &service, const std::string &service_name) :
	m_service (service),
	m_service_name (service_name)
{
	YAIL_LOG_FUNCTION (this << m_service_name);

	m_service.get_server ().add_provider (m_service_name);
}

template <typename Transport>
provider_impl<Transport>::~provider_impl ()
{
	YAIL_LOG_FUNCTION (this << m_service_name);

	m_service.get_server ().remove_provider (m_service_name);
}

template <typename Transport>
template <typename Request, typename Response, typename Handler>
void provider_impl<Transport>::add_rpc (const rpc_impl<Request, Response> &service_rpc, const Handler &handler)
{
	auto pctx = std::make_shared<context<Request, Response, Handler>> (service_rpc, handler);

	m_service.get_server ().add_rpc (m_service_name, service_rpc.get_name (), service_rpc.get_type_name (),
		[pctx] (yail::rpc::trans_context &tctx, const std::string &req_data)
		{
			Request req;
			if (pctx->m_service_rpc.deserialize (req, req_data))
			{
				pctx->m_handler (tctx, req);
			}
			else
			{
				YAIL_LOG_WARNING ("request deserialization failed");
			}
		});
}

template <typename Transport>
template <typename Request, typename Response>
void provider_impl<Transport>::reply_ok (yail::rpc::trans_context &tctx, const rpc_impl<Request, Response> &service_rpc, const Response &res)
{
	std::string res_data;
	if (service_rpc.serialize (res, &res_data))
	{
		m_service.get_server ().reply (tctx, m_service_name, service_rpc.get_name (), service_rpc.get_type_name (), true, res_data);
	}
	else
	{
		YAIL_LOG_WARNING ("response deserialization failed");
	}
}

template <typename Transport>
template <typename Request, typename Response>
void provider_impl<Transport>::reply_error (yail::rpc::trans_context &tctx, const rpc_impl<Request, Response> &service_rpc, const std::string &errmsg)
{
	m_service.get_server ().reply (tctx, m_service_name, service_rpc.get_name (), service_rpc.get_type_name (), false, errmsg);
}

template <typename Transport>
template <typename Request, typename Response>
void provider_impl<Transport>::reply_delayed (yail::rpc::trans_context &tctx, const rpc_impl<Request, Response> &service_rpc)
{
	m_service.get_server ().reply_delayed (tctx, m_service_name, service_rpc.get_name (), service_rpc.get_type_name ());
}

} // namespace detail
} // namespace rpc
} // namespace yail

#endif // YAIL_RPC_DETAIL_PROVIDER_IMPL_H
