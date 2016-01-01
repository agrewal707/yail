#ifndef YAIL_PUBSUB_TRANSPORT_DETAIL_UDP_IMPL_H
#define YAIL_PUBSUB_TRANSPORT_DETAIL_UDP_IMPL_H

#include <yail/pubsub/transport/udp.h>

//
// yail::detail::udp_impl
//
namespace yail {
namespace pubsub {
namespace transport {
namespace detail {

class udp_impl
{
public:
	using endpoint = yail::pubsub::transport::udp::endpoint;

	class sender
	{
	public:
		sender (yail::io_service &io_service, const endpoint &local_ep, const endpoint &ctrl_multicast_ep);
		~sender ();

		template <typename Handler>
		void async_send (const yail::buffer &buffer, const Handler &handler)
		{
			auto op = std::make_shared<send_operation<Handler>> (handler);

			m_socket.async_send_to (boost::asio::buffer (buffer.data (), buffer.size ()), m_multicast_ep,
				[ this, op ] (const boost::system::error_code &ec, size_t bytes_sent)
				{
					op->m_handler (ec);
				}
			);
		}

	private:
		template <typename Handler>
		struct send_operation
		{
			YAIL_API send_operation (const Handler &handler);
			YAIL_API ~send_operation ();

			Handler m_handler;
		};

		yail::io_service &m_io_service;
		boost::asio::ip::udp::socket m_socket;
		endpoint m_multicast_ep;
	};

	class receiver
	{
	public:
		receiver (yail::io_service &io_service, const endpoint &local_ep, const endpoint &ctrl_multicast_ep);
		~receiver ();

		template <typename Handler>
		void async_receive (yail::buffer &buffer, const Handler &handler)
		{
			auto op = std::make_shared<receive_operation<Handler>> (handler);

			buffer.resize (YAIL_PUBSUB_MAX_MSG_SIZE);
			m_socket.async_receive_from (boost::asio::buffer (buffer.data (), buffer.size ()), m_sender_endpoint,
				[ this, op, &buffer] (const boost::system::error_code &ec, size_t bytes_recvd)
				{
					buffer.resize (bytes_recvd);
					op->m_handler (ec);
				}
			);
		}

	private:
		template <typename Handler>
		struct receive_operation
		{
			YAIL_API receive_operation (const Handler &handler);
			YAIL_API ~receive_operation ();

			Handler m_handler;
		};

		yail::io_service &m_io_service;
		boost::asio::ip::udp::socket m_socket;
		endpoint m_sender_endpoint;
	};

	udp_impl (yail::io_service &io_service, const endpoint &local_ep, const endpoint &ctrl_multicast_ep);
	~udp_impl ();

	template <typename Handler>
	void async_send (const yail::buffer &buffer, const Handler &handler)
	{
		m_sender.async_send (buffer, handler);
	}

	template <typename Handler>
	void async_receive (yail::buffer &buffer, const Handler &handler)
	{
		m_receiver.async_receive (buffer, handler);
	}

private:
	sender m_sender;
	receiver m_receiver;
};

} // namespace detail
} // namespace transport
} // namespace pubsub
} // namespace yail

namespace yail {
namespace pubsub {
namespace transport {
namespace detail {

//
// udp_impl::sender::send_operation
//
template <typename Handler>
udp_impl::sender::send_operation<Handler>::send_operation (const Handler &handler) :
	m_handler (handler)
{}	

template <typename Handler>
udp_impl::sender::send_operation<Handler>::~send_operation ()
{}

//
// udp_impl::receiver::receive_operation
//
template <typename Handler>
udp_impl::receiver::receive_operation<Handler>::receive_operation (const Handler &handler) :
	m_handler (handler)
{}	

template <typename Handler>
udp_impl::receiver::receive_operation<Handler>::~receive_operation ()
{}

} // namespace detail
} // namespace transport
} // namespace pubsub
} // namespace yail

#endif // YAIL_PUBSUB_TRANSPORT_DETAIL_UDP_IMPL_H
