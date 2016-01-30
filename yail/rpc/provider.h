#ifndef YAIL_RPC_PROVIDER_H
#define YAIL_RPC_PROVIDER_H

#include <yail/rpc/service.h>
#include <yail/rpc/rpc.h>

//
// Forward Declarations
//

namespace yail {
namespace rpc {
namespace transport {

class unix_domain;

} // namespace transport
} // namespace rpc
} // namespace yail

namespace yail {
namespace rpc {
namespace detail {

template <typename Transport>
class provider_impl;

} // namespace detail
} // namespace rpc
} // namespace yail

//
// yail::rpc::provider
//
namespace yail {
namespace rpc {

/**
 * @brief RPC provider
 * 
 * @ingroup yail_rpc
 *
 * The user application uses rpc provider to provide
 * one or more RPCs under the given service name.
 */
template <typename Transport = transport::unix_domain>
class YAIL_API provider
{
public:
	using impl_type = detail::provider_impl<Transport>;

	/**
	 * @brief Constructs RPC provider.
	 *
	 * @param[in] service The rpc service object.
	 *
	 * @param[in] service_name The name of the service provided by the RPC provider.
	 */
	provider (service<Transport> &service, const std::string &service_name);

	/**
	 * @brief rpc provider is not copyable.
	 */
	provider (const provider&) = delete;
	provider& operator= (const provider&) = delete;

	/**
	 * @brief rpc provider is movable.
	 */
	provider (provider&&) = default;
	provider& operator= (provider&&) = default;

	/**
	 * @brief Destroys this rpc provider object.
	 */
	~provider ();

	/**
	 * @brief Add RPC to this provider.
	 *
	 * @param[in] service_rpc The rpc to be provided by this provider.
	 *
	 * @param[in] handler The handler to be called on receiving request for the given RPC.
	 */
	template <typename Request, typename Response, typename Handler>
	void add_rpc (const rpc<Request, Response> &service_rpc, const Handler &handler);

	/**
	 * @brief Reply to the RPC call with successful response.
	 *
	 * @param[in] tctx The opaque rpc transaction context passed to the rpc handler.
	 *
	 * @param[in] service_rpc The rpc to be provided by this provider.
	 *
	 * @param[in] res The response to be sent to the rpc client.
	 */
	template <typename Request, typename Response>
	void reply_ok (yail::rpc::trans_context &tctx, const rpc<Request, Response> &service_rpc, const Response &res);

	/**
	 * @brief Reply to the RPC call with error response.
	 *
	 * @param[in] tctx The opaque rpc transaction context passed to the rpc handler.
	 *
	 * @param[in] service_rpc The rpc to be provided by this provider.
	 *
	 * @param[in] errmsg The error message to be sent to the rpc client.
	 */
	template <typename Request, typename Response>
	void reply_error (yail::rpc::trans_context &tctx, const rpc<Request, Response> &service_rpc, const std::string &errmsg = "");

	/**
	 * @brief Indicate to the provider that the reply will be delayed.
	 *
	 * @param[in] tctx The opaque rpc transaction context passed to the rpc handler.
	 *
	 * @param[in] service_rpc The rpc to be provided by this provider.
	 *
	 * This API can be called by the user application to delay the response untill it is available.
	 * Once the response is available, the user application must call reply_ok or reply_error API.
	 */
	template <typename Request, typename Response>
	void reply_delayed (yail::rpc::trans_context &tctx, const rpc<Request, Response> &service_rpc);

private:
	std::unique_ptr<impl_type> m_impl;
};

} // namespace rpc
} // namespace yail

#include <yail/rpc/impl/provider.h>

#endif // YAIL_RPC_PROVIDER_H
