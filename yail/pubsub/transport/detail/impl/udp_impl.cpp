#include <yail/pubsub/transport/udp.h>
#include <yail/pubsub/transport/detail/udp_impl.h>

#include <yail/log.h>

namespace yail {
namespace pubsub {
namespace transport {
namespace detail {

//
// udp_impl::sender
//
udp_impl::sender::sender (
		yail::io_service &io_service, const endpoint &local_ep, const endpoint &multicast_ep) :
	m_io_service (io_service),
	m_socket (io_service, local_ep),
	m_multicast_ep (multicast_ep)
{
	YAIL_LOG_FUNCTION (this);
}

udp_impl::sender::~sender ()
{
	YAIL_LOG_FUNCTION (this);
}

//
// udp_impl::receiver
//
udp_impl::receiver::receiver (
	yail::io_service &io_service, const endpoint &local_ep, const endpoint &multicast_ep) :
	m_io_service (io_service),
	m_socket (io_service)
{
	YAIL_LOG_FUNCTION (this);

	// listen on local address and multicast port
	endpoint listen_ep (local_ep.address (), multicast_ep.port ());
	m_socket.open (listen_ep.protocol());
  m_socket.set_option (boost::asio::ip::udp::socket::reuse_address (true));
  m_socket.bind (listen_ep);

	// join multicast group
  m_socket.set_option (boost::asio::ip::multicast::join_group (multicast_ep.address ()));
}

udp_impl::receiver::~receiver ()
{
	YAIL_LOG_FUNCTION (this);
}

//
// udp_impl
//
udp_impl::udp_impl (
	yail::io_service &io_service, const endpoint &local_ep, const endpoint &ctrl_multicast_ep) :
	m_sender (io_service, local_ep, ctrl_multicast_ep),
	m_receiver (io_service, local_ep, ctrl_multicast_ep)
{
	YAIL_LOG_FUNCTION (this);
}

udp_impl::~udp_impl ()
{
	YAIL_LOG_FUNCTION (this);
}

} // namespace detail
} // namespace transport
} // namespace pubsub
} // namespace yail

