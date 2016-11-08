#include <yail/rpc/transport/unix_domain.h>
#include <yail/rpc/transport/detail/unix_domain_impl.h>

#include <boost/optional.hpp>
#include <boost/asio/steady_timer.hpp>
#include <yail/log.h>

namespace yail {
namespace rpc {
namespace transport {
namespace detail {

using namespace std::placeholders;

YAIL_DEFINE_EXCEPTION(server_receive_handler_not_set);

//
// unix_domain_impl::client
//
unix_domain_impl::client::client (yail::io_service &io_service) :
	m_io_service (io_service)
{
	YAIL_LOG_FUNCTION (this);
}

unix_domain_impl::client::~client ()
{
	YAIL_LOG_FUNCTION (this);
}

template <typename SyncReadStream, typename MutableBufferSequence>
std::size_t read_with_timeout (SyncReadStream &s, 
	const MutableBufferSequence &buffers, boost::system::error_code &ec, const uint32_t timeout)
{
	size_t retval;

	// start read timeout timer
	boost::optional<boost::system::error_code> timer_ec;
	boost::asio::steady_timer timer (s.get_io_service ());
	timer.expires_from_now (std::chrono::seconds (timeout));
	timer.async_wait (
		[&timer_ec] (const boost::system::error_code &ec) 
		{ 
			timer_ec.reset (ec); 
		});
	
	// read response size
	boost::optional<boost::system::error_code> read_ec;
	boost::asio::async_read(s, buffers, 
		[&read_ec, &retval] (const boost::system::error_code& ec, size_t bytes_read)
		{ 
			read_ec.reset (ec); 
			retval = bytes_read;
		});

	s.get_io_service ().reset();
	while (s.get_io_service ().run_one())
	{ 
		if (read_ec)
				timer.cancel ();
		else if (timer_ec)
				s.cancel ();
	}
	ec = *read_ec;

	return retval;
}

void unix_domain_impl::client::send_n_receive (const endpoint &ep,
	const yail::buffer &req_buffer, yail::buffer &res_buffer, boost::system::error_code &ec, const uint32_t timeout)
{
	YAIL_LOG_FUNCTION (this << ep.path ());

	boost::asio::io_service io_service;
	stream_protocol::socket socket (io_service);
	socket.connect (ep, ec);
	if (ec) 
	{
		YAIL_LOG_ERROR ("connect: " << ec);
		return;
	}

	// request size buffer
	char size_buffer[4];
	size_buffer[0] = req_buffer.size () >> 24;
	size_buffer[1] = req_buffer.size () >> 16;
	size_buffer[2] = req_buffer.size () >> 8;
	size_buffer[3] = req_buffer.size () >> 0;

	// write buffer sequence = request size buffer + request buffer
	std::vector<boost::asio::const_buffer> bufs;
	bufs.push_back(boost::asio::buffer(size_buffer));
	bufs.push_back(boost::asio::buffer(req_buffer.data (), req_buffer.size ()));
	boost::asio::write(socket, bufs, ec);
	if (ec) 
	{
		YAIL_LOG_ERROR ("request write: " << ec);
		return;
	}
	
	if (timeout)
		read_with_timeout (socket, boost::asio::buffer(size_buffer), ec, timeout);
	else
		boost::asio::read (socket, boost::asio::buffer(size_buffer), ec);

	if (ec)
	{
		if (ec != boost::asio::error::operation_aborted)
		{	
			YAIL_LOG_ERROR ("response size read: " << ec);
		}
		return;
	}

	size_t response_size = 0;
	response_size |= size_buffer[0] << 24;
	response_size |= size_buffer[1] << 16;
	response_size |= size_buffer[2] << 8;
	response_size |= size_buffer[3] << 0;

	// read response
	res_buffer.resize (response_size);
	if (timeout)
		read_with_timeout (socket, boost::asio::buffer(res_buffer.data (), res_buffer.size ()), ec, timeout);
	else
		boost::asio::read (socket, boost::asio::buffer(res_buffer.data (), res_buffer.size ()), ec);

	if (ec) 
	{
		if (ec != boost::asio::error::operation_aborted)
		{	
			YAIL_LOG_ERROR ("response size read: " << ec);
		}
		return;
	}
}

//
// unix_domain_impl::server::session
//
unix_domain_impl::server::session::session(yail::io_service& io_service, server *s, const receive_handler &receive_handler) :
	m_io_service (io_service),
	m_server (s),
	m_socket (io_service),
	m_receive_handler (receive_handler)
{
	YAIL_LOG_FUNCTION (this);
}

unix_domain_impl::server::session::~session()
{
	YAIL_LOG_FUNCTION (this);
}

void unix_domain_impl::server::session::start ()
{
	YAIL_LOG_FUNCTION (this);

	auto self(shared_from_this());

	// read request buffer size
	boost::asio::async_read (m_socket, boost::asio::buffer(m_size_buffer),
		[this, self] (const boost::system::error_code &ec, std::size_t bytes_transferred)
		{
			YAIL_LOG_FUNCTION ("read request size" << ec << bytes_transferred);

			if (!ec)
			{
				size_t request_size = 0;
				request_size |= m_size_buffer[0] << 24;
				request_size |= m_size_buffer[1] << 16;
				request_size |= m_size_buffer[2] << 8;
				request_size |= m_size_buffer[3] << 0;
				YAIL_LOG_TRACE ("read request size: " << request_size);

				// read request buffer
				auto pbuf (std::make_shared<yail::buffer> (request_size));
				boost::asio::async_read (m_socket, boost::asio::buffer(pbuf->data (), pbuf->size ()),
					[this, self, pbuf] (const boost::system::error_code &ec,  std::size_t bytes_transferred)
					{
						YAIL_LOG_FUNCTION ("read request size" << ec << bytes_transferred);

						if (!ec)
						{
							// add to the server session map
							m_server->add_session (self);

							// call client receive handler
							m_receive_handler (this, pbuf);
						}
						else
						{
							YAIL_LOG_ERROR ("read request failed: " << ec);
						}
					});
			}
			else
			{
				YAIL_LOG_ERROR ("read request size failed: " << ec);
			}
		});

}

void unix_domain_impl::server::session::write (const yail::buffer &res_buffer, boost::system::error_code &ec)
{
	YAIL_LOG_FUNCTION (this);

	// response size buffer
	char size_buffer[4];
	size_buffer[0] = res_buffer.size () >> 24;
	size_buffer[1] = res_buffer.size () >> 16;
	size_buffer[2] = res_buffer.size () >> 8;
	size_buffer[3] = res_buffer.size () >> 0;

	// write buffer sequence = response size buffer + response buffer
	std::vector<boost::asio::const_buffer> bufs;
	bufs.push_back(boost::asio::buffer(size_buffer));
	bufs.push_back(boost::asio::buffer(res_buffer.data (), res_buffer.size ()));
	boost::asio::write(m_socket, bufs, ec);
	if (ec) 
	{
		YAIL_LOG_ERROR ("response write: " << ec);
		return;
	}
}

//
// unix_domain_impl::server
//
unix_domain_impl::server::server (yail::io_service &io_service, const endpoint &ep, const receive_handler &handler) :
	m_io_service (io_service),
	m_acceptor (io_service),
	m_receive_handler (handler),
	m_ref_count (1)
{
	YAIL_LOG_FUNCTION (this << ep.path ());

	unlink (ep.path ().c_str ());

	m_acceptor.open ();
	m_acceptor.bind (ep);
	m_acceptor.listen ();

	auto new_session (std::make_shared<session> (m_io_service, this, m_receive_handler));
 	m_acceptor.async_accept (new_session->socket (), std::bind(&server::handle_accept, this, new_session, _1));
}

unix_domain_impl::server::~server ()
{
	YAIL_LOG_FUNCTION (this);

	auto path = m_acceptor.local_endpoint ().path ();
	m_acceptor.close ();
	unlink (path.c_str ());
}

void unix_domain_impl::server::add_session (std::shared_ptr<session> ses)
{
	const auto it = m_session_map.find (ses.get ());
	if (it == m_session_map.end ())
	{
		m_session_map[ses.get()] = ses;
	}
	else
	{
		YAIL_LOG_WARNING ("duplicate session");
	}
}

void unix_domain_impl::server::remove_session (session *ses)
{
	const auto it = m_session_map.find (ses);
	if (it != m_session_map.end ())
	{
		m_session_map.erase (it);	
	}
	else
	{
		YAIL_LOG_WARNING ("unknown session");
	}
}

void unix_domain_impl::server::handle_accept (std::shared_ptr<session> new_session, const boost::system::error_code &ec)
{
	YAIL_LOG_FUNCTION (this << ec);

	try
	{
		if (!ec)
		{
			new_session->start ();
		}
		else
		{
			YAIL_LOG_ERROR ("failed to accept connection: " << ec);
		}

		auto new_session (std::make_shared<session> (m_io_service, this, m_receive_handler));
		m_acceptor.async_accept (new_session->socket (), std::bind(&server::handle_accept, this, new_session, _1));
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
	m_io_service (io_service),
	m_client (io_service),
	m_server_map ()
{
	YAIL_LOG_FUNCTION (this);
}

unix_domain_impl::~unix_domain_impl ()
{
	YAIL_LOG_FUNCTION (this);
}

} // namespace detail
} // namespace transport
} // namespace rpc
} // namespace yail

