#ifndef YAIL_RPC_CLIENT_H
#define YAIL_RPC_CLIENT_H

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
class client_impl;

} // namespace detail
} // namespace rpc
} // namespace yail

//
// yail::rpc::client
//
namespace yail {
namespace rpc {

/**
 * @brief RPC client
 * 
 * @ingroup yail_rpc
 *
 * The user application uses client to call RPCs
 * provided by the RPC provider.
 */
template <typename Transport = transport::unix_domain>
class YAIL_API client
{
public:
	using impl_type = detail::client_impl<Transport>;
	
	/**
	 * @brief Constructs typed rpc client.
	 *
	 * @param[in] service The rpc service object.
	 *
	 */
	client (service<Transport> &service);

	/**
	 * @brief rpc client is not copyable.
	 */
	client (const client&) = delete;
	client& operator= (const client&) = delete;

	/**
	 * @brief rpc client is movable.
	 */
	client (client&&) = default;
	client& operator= (client&&) = default;

	/**
	 * @brief Destroys this rpc client object.
	 */
	~client ();

	/**
	 * @brief Call RPC synchronously.
	 *
	 * @param[in] service_name The name of the service provided by the RPC provider.
	 *
	 * @param[in] service_rpc The rpc to be called for the specified service.
	 *
	 * @param[in] req The request object to be sent to provider for the specified rpc.
	 *
	 * @param[out] res The response object to be received from the provider for the specified rpc.
	 *
	 * @param[out] ec The error code returned on completion of the rpc operation.
	 */
	template <typename Request, typename Response>
	void call (const std::string& service_name, const rpc<Request, Response> &service_rpc, 
	           const Request &req, Response &res, boost::system::error_code &ec);

	/**
	 * @brief Call RPC asynchronously.
	 *
	 * @param[in] service_name The name of the service provided by the RPC provider.
	 *
	 * @param[in] service_rpc The rpc to be called for the specified service.
	 *
	 * @param[in] req The request object to be sent to provider for the specified rpc.
	 *
	 * @param[out] res The response object to be received from the provider for the specified rpc.
	 *
	 * @param[out] handler The handler to be called on completion of the rpc operation.
	 */
	template <typename Request, typename Response, typename Handler>
	void async_call (const std::string& service_name, const rpc<Request, Response> &service_rpc, 
	                 const Request &req, Response &res, const Handler &handler);

private:
	std::unique_ptr<impl_type> m_impl;
};

} // namespace rpc
} // namespace yail


#include <yail/rpc/impl/client.h>

#endif // YAIL_RPC_CLIENT_H
