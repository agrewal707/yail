#ifndef YAIL_RPC_TRANSPORT_UNIX_DOMAIN_H
#define YAIL_RPC_TRANSPORT_UNIX_DOMAIN_H

#include <yail/io_service.h>
#include <yail/buffer.h>
#include <yail/memory.h>

//
// Forward declarations
//
namespace yail {
namespace rpc {
namespace transport {
namespace detail {

class unix_domain_impl;

} // namespace detail
} // namespace transport
} // namespace rpc
} // namespace yail

//
// yail::unix_domain
//
namespace yail {
namespace rpc {
namespace transport {

/**
 * @brief Provides UNIX DOMAIN transport for rpc messaging.
 * 
 * @ingroup yail_rpc_transport
 */
class YAIL_API unix_domain
{
public:
	using endpoint = boost::asio::local::stream_protocol::endpoint;
	using impl_type = detail::unix_domain_impl;

	/**
	 * @brief Constructs transport.
	 *
	 * @param[in] io_service The io service object.
	 *
	 */
	unix_domain (yail::io_service &io_service);

	/**
	 * @brief unix_domain transport is not copyable.
	 */
	unix_domain (const unix_domain&) = delete;
	unix_domain& operator= (const unix_domain&) = delete;

	/**
	 * @brief unix_domain transport is movable.
	 */
	unix_domain (unix_domain&&) = default;
	unix_domain& operator= (unix_domain&&) = default;

	/**
	 * @brief Destroys this object.
	 */
	~unix_domain ();

	/**
	 * @brief Send and receive message buffer synchronously from client to server.
	 *
	 * @param[in] ep The server transport endpoint.
	 *
	 * @param[in] req_buffer The buffer to send.
	 *
	 * @param[out] res_buffer The buffer to receive the message into.
	 *
	 * @param[out] ec The error code returned on completion of the operation.
	 */
	void client_send_n_receive (const endpoint &ep,
		const yail::buffer &req_buffer, yail::buffer &res_buffer, boost::system::error_code &ec, const uint32_t timeout);

	/**
	 * @brief Send and receive message buffer asynchronously from client to server.
	 *
	 * @param[in] ep The server transport endpoint.
	 *
	 * @param[in] req_buffer The buffer to send.
	 *
	 * @param[out] res_buffer The buffer to receive the message into.
	 *
	 * @param[in] handler The handler to call on completion of the operation.
	 */
	template <typename Handler>
	void async_client_send_n_receive (const endpoint &ep,
		const yail::buffer &req_buffer, yail::buffer &res_buffer, const Handler &handler);

	/**
	 * @brief Install handler in the server to receive messages from clients
	 *
	 * @param[in] handler The handler to call on receiving messages from clients.
	 */
	template <typename Handler>
	void server_set_receive_handler (const Handler &handler);

	/**
	 * @brief Add server to run on given endpoint
	 *
	 * @param[in] ep The server transport endpoint.
	 *
	 */
	void server_add (const endpoint &ep);

	/**
	 * @brief Remove server running on given endpoint
	 *
	 * @param[in] ep The server transport endpoint.
	 *
	 */
	void server_remove (const endpoint &ep);

	/**
	 * @brief Send message buffer synchronously from server to client.
	 *
	 * @param[in] trctx The transport session context.
	 *
	 * @param[in] res_buffer The buffer to send.
	 *
	 * @param[out] ec The error code returned on completion of the operation.
	 */
	void server_send (void *trctx, const yail::buffer &res_buffer, boost::system::error_code &ec);

private:
	std::unique_ptr<impl_type> m_impl;
};

} // namespace transport
} // namespace rpc
} // namespace yail

#include <yail/rpc/transport/impl/unix_domain.h>

#endif // YAIL_RPC_TRANSPORT_UNIX_DOMAIN_H
