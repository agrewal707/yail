#include <yail/rpc/transport/unix_domain.h>
#include <yail/rpc/transport/detail/unix_domain_impl.h>

#include <yail/log.h>

namespace yail {
namespace rpc {
namespace transport {
namespace detail {

using namespace std::placeholders;

//
// unix_domain_impl::client
//
unix_domain_impl::client::client (yail::io_service &io_service) :
	m_io_service (io_service)
{
	YAIL_LOG_TRACE (this);
}

unix_domain_impl::client::~client ()
{
	YAIL_LOG_TRACE (this);
}

void unix_domain_impl::client::send_n_receive (const endpoint &ep,
	const yail::buffer &req_buffer, yail::buffer &res_buffer, boost::system::error_code &ec)
{
	YAIL_LOG_TRACE (this);

	stream_protocol::socket socket (m_io_service);
	socket.connect (ep, ec);
	if (ec) return;

	// request size buffer
	char size_buffer[4];
	size_buffer[0] = req_buffer.size () >> 24;
	size_buffer[1] = req_buffer.size () >> 16;
	size_buffer[2] = req_buffer.size () >> 8;
	size_buffer[3] = req_buffer.size () >> 0;

	// write buffer sequence = request size buffer + request buffer
	std::vector<const_buffer> bufs;
	bufs.push_back(boost::asio::buffer(size_buffer));
	bufs.push_back(boost::asio::buffer(req_buffer.data (), req_buffer.size ()));
	boost::asio::write(socket, bufs, ec);
	if (ec) return;

	// read response size
	boost::asio::read (socket, boost::asio::buffer(size_buffer), ec);
	if (ec) return;

	size_t response_size = 0;
	response_size |= size_buffer[0] << 24;
	response_size |= size_buffer[1] << 16;
	response_size |= size_buffer[2] << 8;
	response_size |= size_buffer[3] << 0;

	// read response
	res_buffer.resize (response_size);
	boost::asio::read (socket, boost::asio::buffer(res_buffer), ec);
}

//
// unix_domain_impl::server::session
//
unix_domain_impl::server::session::session(boost::asio::io_service& io_service, const receive_handler &receive_handler) :
	m_socket (io_service),
	m_receive_handler (receive_handler)
{
	YAIL_LOG_TRACE (this);
}

unix_domain_impl::server::session::~session()
{
	YAIL_LOG_TRACE (this);
}

void unix_domain_impl::server::session::start ()
{
	YAIL_LOG_TRACE (this);

	auto pbuf (std::make_shared<yail::buffer> (YAIL_RPC_MAX_MSG_SIZE));

	m_socket.async_read_some(boost::asio::buffer (pbuf->data (), pbuf->size ()),
		std::bind(&session::handle_read, shared_from_this(), _1, _2, pbuf));
}

void unix_domain_impl::server::session::handle_read (const boost::system::error_code &ec, size_t bytes_read, std::shared_ptr<yail::buffer> pbuf)
{
	YAIL_LOG_TRACE (this << ec << bytes_read);

	if (!ec)
	{
		pbuf->resize (bytes_read);
		m_io_service.post (m_receive_handler, this, pbuf);
	}
	else (ec != boost::asio::error::operation_aborted)
	{
		// TODO
	}
}

void unix_domain_impl::server::session::write (const yail::buffer &buf, boost::system::error_code &ec)
{
	YAIL_LOG_TRACE (this);

	boost::asio::write (m_socket, boost::asio::buffer(buf.data (), buf.size ()), ec);
}

//
// unix_domain_impl::server
//
unix_domain_impl::server::server (yail::io_service &io_service, const endpoint &ep, const receive_handler &handler) :
	m_io_service_(io_service),
  m_acceptor (io_service, stream_protocol::endpoint (ep)),
	m_receive_handler (receive_handler),
	m_ref_count (1)
{
	YAIL_LOG_TRACE (this);

	auto new_session (std::make_shared<session> (m_io_service, m_receive_handler));
 	m_acceptor.async_accept (new_session->socket (),
		std::bind(&server::handle_accept, this, new_session, _1));
}

unix_domain_impl::server::~server ()
{
	YAIL_LOG_TRACE (this);
}

void unix_domain_impl::server::handle_accept (std::shared_ptr<session> new_session, const boost::system::error_code &ec)
{
	YAIL_LOG_TRACE (this << ec);

	try
	{
		if (!ec)
		{
			new_session->start ();
		}

		auto new_session (std::make_shared<session> (m_io_service, m_receive_handler));
		m_acceptor.async_accept (new_session->socket (),
				boost::bind(&server::handle_accept, this, new_session, _1));
	}
	catch (const std::exception &ex)
	{
		YAIL_LOG_ERROR ("failed to accept connection");
	}
}

//
// unix_domain_impl
//
unix_domain_impl::unix_domain_impl (yail::io_service &io_service) :
	m_client (io_service),
	m_server_map ()
{
	YAIL_LOG_TRACE (this);
}

unix_domain_impl::~unix_domain_impl ()
{
	YAIL_LOG_TRACE (this);
}

} // namespace detail
} // namespace transport
} // namespace rpc
} // namespace yail

