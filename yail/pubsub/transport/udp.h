#ifndef YAIL_PUBSUB_TRANSPORT_UDP_H
#define YAIL_PUBSUB_TRANSPORT_UDP_H

#include <yail/io_service.h>
#include <yail/buffer.h>
#include <yail/memory.h>

//
// Forward declarations
//
namespace yail {
namespace pubsub {
namespace transport {
namespace detail {

class udp_impl;

} // namespace detail
} // namespace transport
} // namespace pubsub
} // namespace yail

//
// yail::udp
//
namespace yail {
namespace pubsub {
namespace transport {

/**
 * @brief Provides UDP transport for pubsub messaging.
 * 
 * @ingroup yail_pubsub_transport
 */
class YAIL_API udp
{
public:
	using endpoint = boost::asio::ip::udp::endpoint;
	using impl_type = detail::udp_impl;

	/**
	 * @brief Constructs transport.
	 *
	 * @param[in] io_service The io service object.
	 *
	 */
	udp (yail::io_service &io_service);

	/**
	 * @brief Constructs transport with specified endpoints.
	 *
	 * @param[in] io_service The io service object.
	 *
	 * @param[in] local_ep The local address and port.
	 *
	 * @param[in] multicast_ep The multicast address and port.
	 */
	udp (yail::io_service &io_service, const endpoint &local_ep, const endpoint &ctrl_multicast_ep);

	/**
	 * @brief udp transport is not copyable.
	 */
	udp (const udp&) = delete;
	udp& operator= (const udp&) = delete;

	/**
	 * @brief udp transport is movable.
	 */
	udp (udp&&) = default;
	udp& operator= (udp&&) = default;

	/**
	 * @brief Destroys this object.
	 */
	~udp ();

	/**
	 * @brief Add topic to the transport. 
	 *
	 * @param[in] topic_id The topic id to add.
	 */
	void add_topic (const std::string &topic_id);

	/**
	 * @brief Remove topic from the transport. 
	 *
	 * @param[in] topic_id The topic id to remove.
	 */
	void remove_topic (const std::string &topic_id);

	/**
	 * @brief Send message buffer asynchronously.
	 *
	 * @param[in] buffer The buffer to send.
	 *
	 * @param[out] ec The error code returned on completion of the write operation.
	 */
	void send (const std::string &topic_id, const yail::buffer &buffer, boost::system::error_code &ec, const uint32_t timeout);
	
	/**
	 * @brief Send message buffer asynchronously.
	 *
	 * @param[in] buffer The buffer to send.
	 *
	 * @param[in] handler The handler to be called on completion of send.
	 */
	template <typename Handler>
	void async_send (const std::string &topic_id, const yail::buffer &buffer, const Handler &handler);

	/**
	 * @brief Receive message into the specified buffer asynchronously.
	 *
	 * @param[out] buffer The buffer where received message is stored.
	 *
	 * @param[in] handler The handler to be called on completion of receive.
	 */
	template <typename Handler>
	void async_receive (yail::buffer &buffer, const Handler &handler);

private:
	std::unique_ptr<impl_type> m_impl;
};

} // namespace transport
} // namespace pubsub
} // namespace yail

#include <yail/pubsub/transport/impl/udp.h>

#endif // YAIL_PUBSUB_TRANSPORT_UDP_H
