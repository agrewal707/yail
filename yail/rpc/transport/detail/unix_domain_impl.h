#ifndef YAIL_RPC_TRANSPORT_DETAIL_UNIX_DOMAIN_IMPL_H
#define YAIL_RPC_TRANSPORT_DETAIL_UNIX_DOMAIN_IMPL_H

#include <yail/rpc/transport/unix_domain.h>

#include <functional>
#include <unordered_map>
#include <yail/memory.h>

//
// yail::detail::unix_domain_impl
//
namespace yail {
namespace rpc {
namespace transport {
namespace detail {

using boost::asio::local::stream_protocol;

class unix_domain_impl
{
public:
	using endpoint = unix_domain::endpoint;

	//
	// Client
	//
	class client
	{
	public:
		client (yail::io_service &io_service);
		~client ();

		void send_n_receive (const endpoint &ep,
			const yail::buffer &req_buffer, yail::buffer &res_buffer, boost::system::error_code &ec);

		template <typename Handler>
		void async_send_n_receive (const endpoint &ep,
			const yail::buffer &req_buffer, yail::buffer &res_buffer, const Handler &handler);

	private:
		template <typename Handler>
		struct operation
		{
			YAIL_API operation (yail::io_service &io_service, yail::buffer &res_buffer, const Handler &handler);
			YAIL_API ~operation ();

			yail::buffer &m_res_buffer;
			Handler m_handler;
			char m_size_buffer[4];
			std::vector<boost::asio::const_buffer> m_bufs;
			stream_protocol::socket m_socket;
		};

		yail::io_service &m_io_service;
	};

	//
	// Server
	//
	class server
	{
	public:
		using receive_handler = std::function<void(void*, std::shared_ptr<yail::buffer>)>;

		class session : public std::enable_shared_from_this<session>
		{
		public:
			session (yail::io_service &io_service, const receive_handler &receive_handler);
			~session ();

			stream_protocol::socket& socket() { return m_socket; }

			void start ();
			void write (const yail::buffer &buf, boost::system::error_code &ec);

		private:
			void handle_read (const boost::system::error_code &ec, size_t bytes_read, std::shared_ptr<yail::buffer> pbuf);

			yail::io_service &m_io_service;
			stream_protocol::socket m_socket;
			const receive_handler &m_receive_handler;
		};

		server (yail::io_service &io_service, const endpoint &ep, const receive_handler &receive_handler);
		~server ();

		uint32_t get_ref_count () { return m_ref_count; }
		void incr_ref_count () { ++m_ref_count; }
		void decr_ref_count () { --m_ref_count; }

	private:
		void handle_accept (std::shared_ptr<session> new_session, const boost::system::error_code &ec);

		yail::io_service &m_io_service;
  	stream_protocol::acceptor m_acceptor;
		const receive_handler m_receive_handler;
		uint32_t m_ref_count;
	};

	//
	// unix_domain_impl
	//
	unix_domain_impl (yail::io_service &io_service);
	~unix_domain_impl ();

	//
	// Client API
	//
	void client_send_n_receive (const endpoint &ep,
		const yail::buffer &req_buffer, yail::buffer &res_buffer, boost::system::error_code &ec)
	{
		m_client.send_n_receive (ep, req_buffer, res_buffer, ec);
	}

	template <typename Handler>
	void async_client_send_n_receive (const endpoint &ep,
		const yail::buffer &req_buffer, yail::buffer &res_buffer, const Handler &handler)
	{
		m_client.async_send_n_receive (ep, req_buffer, res_buffer, handler);
	}

	//
	// Server API
	//
	template <typename Handler>
	void server_set_receive_handler (const Handler &handler)
	{
		m_server_receive_handler = handler;	
	}

	void server_add (const endpoint &ep)
	{
		if (m_server_receive_handler)
		{
			// TODO
		}

		const auto it = m_server_map.find (ep.path ());
		if (it == m_server_map.end ())
		{
			auto srv (yail::make_unique<server> (m_io_service, ep, m_server_receive_handler));
			m_server_map[ep.path ()] = std::move (srv);
		}
		else
		{
			it->second->incr_ref_count ();
		}
	}

	void server_remove (const endpoint &ep)
	{
		const auto it = m_server_map.find (ep.path ());
		if (it != m_server_map.end ())
		{
			it->second->decr_ref_count ();
			if (!it->second->get_ref_count ())
			{
				m_server_map.erase (it);
			}
		}
	}

	void server_send (void *trctx, const yail::buffer &res_buffer, boost::system::error_code &ec)
	{
		server::session *psession = static_cast<server::session*> (trctx);
		psession->write (res_buffer, ec);
	}
	
private:
	yail::io_service &m_io_service;
	client m_client;
	server::receive_handler m_server_receive_handler;
	using server_map = std::unordered_map<std::string, std::unique_ptr<server>>;
	server_map m_server_map;
};

} // namespace detail
} // namespace transport
} // namespace rpc
} // namespace yail

namespace yail {
namespace rpc {
namespace transport {
namespace detail {

//
// unix_domain_impl::client::operation
//
template <typename Handler>
unix_domain_impl::client::operation<Handler>::operation (yail::io_service &io_service, yail::buffer &res_buffer, const Handler &handler) :
	m_res_buffer (res_buffer),
	m_handler (handler),
	m_socket (io_service)
{}	

template <typename Handler>
unix_domain_impl::client::operation<Handler>::~operation ()
{}

//
// unix_domain_impl::client
//
template <typename Handler>
void unix_domain_impl::client::async_send_n_receive (const endpoint &ep,
	const yail::buffer &req_buffer, yail::buffer &res_buffer, const Handler &handler)
{
	auto op = std::make_shared<operation<Handler>> (m_io_service, res_buffer, handler);

	// connect to service
	op->m_socket.async_connect (ep,
		[&req_buffer, op ] (const boost::system::error_code &ec)
		{
			if (!ec)
			{
				// write req size
				op->m_size_buffer[0] = req_buffer.size () >> 24;
				op->m_size_buffer[1] = req_buffer.size () >> 16;
				op->m_size_buffer[2] = req_buffer.size () >> 8;
				op->m_size_buffer[3] = req_buffer.size () >> 0;

				// write buffer sequence = size buffer + request buffer
				op->m_bufs.push_back(boost::asio::buffer(op->m_size_buffer));
				op->m_bufs.push_back(boost::asio::buffer(req_buffer.data (), req_buffer.size ()));
				boost::asio::async_write (op->m_socket, op->m_bufs,
					[op] (const boost::system::error_code &ec, std::size_t bytes_transferred)
					{
						if (!ec)
						{
							// read response size
							boost::asio::async_read (op->m_socket, boost::asio::buffer(op->m_size_buffer),
								[op] (const boost::system::error_code &ec, std::size_t bytes_transferred)
								{
									if (!ec)
									{
										size_t response_size = 0;
										response_size |= op->m_size_buffer[0] << 24;
										response_size |= op->m_size_buffer[1] << 16;
										response_size |= op->m_size_buffer[2] << 8;
										response_size |= op->m_size_buffer[3] << 0;

										// read response
										op->m_res_buffer.resize (response_size);
										boost::asio::async_read (op->m_socket, boost::asio::buffer(op->m_res_buffer.data (), op->m_res_buffer.size ()),
											[op] (const boost::system::error_code &ec,  std::size_t bytes_transferred)
											{
												op->m_handler (ec);
											});
									}
									else
									{
										op->m_handler (ec);
									}
								});
						}
						else
						{
							op->m_handler (ec);
						}
					});
			}
			else
			{
				op->m_handler (ec);
			}
		});
}

} // namespace detail
} // namespace transport
} // namespace rpc
} // namespace yail

#endif // YAIL_RPC_TRANSPORT_DETAIL_UNIX_DOMAIN_IMPL_H
