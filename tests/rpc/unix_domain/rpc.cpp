#include <iostream>
#include <fstream>
#include <boost/crc.hpp>
#include <boost/program_options.hpp>
#include <boost/asio/steady_timer.hpp>

#include <yail/rpc/service.h>
#include <yail/rpc/client.h>
#include <yail/rpc/provider.h>

#include "rpcs.h"

std::ostream *olog = &std::clog;

#define LOG_INFO(msg) *olog << msg << std::endl
#define LOG_ERROR(msg) *olog << msg << std::endl
#ifndef NDEBUG
#define LOG_DEBUG(msg) *olog << __FUNCTION__ << ":" << msg << std::endl
#else
#define LOG_DEBUG(msg)
#endif

const size_t MAX_MESSAGE_SIZE = 4096;
size_t client_count;

namespace po = boost::program_options;
using transport = yail::rpc::transport::unix_domain;

struct pargs
{
	std::string m_name;
	size_t m_num_providers;
	size_t m_num_clients;
	size_t m_num_calls;
	enum call_type { SYNC, ASYNC } m_call_type;
	enum reply_type { OK, DELAYED, ERROR } m_reply_type;
	size_t m_data_size;
	std::string m_log_file;

	pargs ():
		m_name (),
		m_num_providers (0),
		m_num_clients (0),
		m_num_calls (1),
		m_call_type (SYNC),
		m_reply_type (OK),
		m_data_size (1024),
		m_log_file ()
	{}

	~pargs ()
	{}

	bool parse (int argc, char* argv[])
	{
		bool retval =  false;

		po::options_description desc("options: ");
		desc.add_options ()
			("help", "print help message")
			("num-clients", po::value<size_t>()->required(), "max num of rpc clients to instantiate.")
			("num-providers", po::value<size_t>()->required(), "max num of rpc providers to instantiate.")
			("num-calls", po::value<size_t>(), "max num number of rpc calls to make")
			("call-type", po::value<unsigned>(), "type of call the client should make")
			("reply-type", po::value<unsigned>(), "type of reply sent that should be sent by the provider")
			("data-size", po::value<size_t>(), "size of data to write in each message")
			("log-file", po::value<std::string>(), "log file")
			;

		try
		{
			po::variables_map vm;
			po::store (po::parse_command_line(argc, argv, desc), vm);
			po::notify (vm);
		
			if (vm.count("help") || argc < 2) 
			{
				LOG_INFO (desc);
				return false;
			}

			m_name = argv[0];

			if (vm.count("num-providers"))
				m_num_providers = vm["num-providers"].as<size_t> ();

			if (vm.count("num-clients"))
				m_num_clients = vm["num-clients"].as<size_t> ();

			if (vm.count("num-calls"))
				m_num_calls = vm["num-calls"].as<size_t> ();

			if (vm.count("call-type"))
				m_call_type = static_cast<call_type> (vm["call-type"].as<unsigned> ());

			if (vm.count("reply-type"))
				m_reply_type = static_cast<reply_type> (vm["reply-type"].as<unsigned> ());

			if (vm.count("data-size"))
				m_data_size = vm["data-size"].as<size_t> ();

			if (vm.count("log-file"))
				m_log_file = vm["log-file"].as<std::string> ();
	
			retval = true;
		} 
		catch (...) 
		{
			LOG_INFO (desc);
		}

		return retval;
	}
};

void client_done ()
{
	if (!client_count--)
	{
		alarm (1);
	}
}

class client
{
public:
	client (const std::string &name,
					yail::rpc::service<transport> &rpc_service,
					yail::rpc::rpc<messages::hello_request, messages::hello_response> &hello_rpc,
					pargs &pa):
		m_name (name),
		m_rpc_client (rpc_service),
		m_hello_rpc (hello_rpc),
		m_pa (pa),
		m_seq (1),
		m_total_ok (0),
		m_total_error (0),
		m_total_valid (0)
	{
		do_call ();
	}

	~client ()
	{}

	void print_stats () const
	{
		LOG_INFO (
			m_pa.m_name << ", " <<
			m_name << ", " <<
			"call:" << m_seq-1 <<
			", reply-ok:" << m_total_ok <<
			", reply-error:" << m_total_error <<
			", valid:" << m_total_valid);
	}

private:
	void do_call ()
	{
		if (m_seq == m_pa.m_num_calls+1)
		{
			client_done ();
			return;
		}

		if (pargs::SYNC == m_pa.m_call_type)
		{
			messages::hello_request hello_req;
			hello_req.set_msg ("hello");
			hello_req.set_seq (m_seq++);
			hello_req.set_data (std::string (m_pa.m_data_size, 'A'));
			hello_req.clear_crc ();

			// Add CRC to valiate message integrity
			yail::buffer tmp (MAX_MESSAGE_SIZE);
			tmp.resize (hello_req.ByteSize ());
			hello_req.SerializeToArray (tmp.data (), tmp.size ());
			boost::crc_32_type  result;
			result.process_bytes (tmp.data (), tmp.size ());
			hello_req.set_crc (result.checksum ());

			messages::hello_response hello_res;
			boost::system::error_code ec;
			m_rpc_client.call("greeting_service", m_hello_rpc, hello_req, hello_res, ec);
			if (!ec)
			{
				LOG_DEBUG ("msg: " << hello_res.msg ());
				LOG_DEBUG ("seq: " << hello_res.seq ());
				LOG_DEBUG ("data: " << hello_res.data ());
				LOG_DEBUG ("crc: " << hello_res.crc ());

				uint32_t crc = hello_res.crc ();
				hello_res.clear_crc ();

				// validate message integrity
				yail::buffer tmp (MAX_MESSAGE_SIZE);
				tmp.resize (hello_res.ByteSize ());
				hello_res.SerializeToArray (tmp.data (), tmp.size ());
				boost::crc_32_type  result;
				result.process_bytes (tmp.data (), tmp.size ());
				bool valid = (result.checksum () ==  crc);
				LOG_DEBUG ("valid: " << valid);

				m_total_ok++;

				if (valid)
					m_total_valid++;

				do_call ();
			}
			else if (ec == yail::rpc::error::failure_response)
			{
				m_total_error++;

				do_call ();
			}
			else if (ec != boost::asio::error::operation_aborted)
			{
				LOG_ERROR ("error: " << ec);
			}
		}
		else if (pargs::ASYNC == m_pa.m_call_type)
		{
			auto hello_req (std::make_shared<messages::hello_request> ());
			hello_req->set_msg ("hello");
			hello_req->set_seq (m_seq++);
			hello_req->set_data (std::string (m_pa.m_data_size, 'A'));
			hello_req->clear_crc ();

			// Add CRC to valiate message integrity
			yail::buffer tmp (MAX_MESSAGE_SIZE);
			tmp.resize (hello_req->ByteSize ());
			hello_req->SerializeToArray (tmp.data (), tmp.size ());
			boost::crc_32_type  result;
			result.process_bytes (tmp.data (), tmp.size ());
			hello_req->set_crc (result.checksum ());

			auto hello_res (std::make_shared<messages::hello_response> ());
			m_rpc_client.async_call("greeting_service", m_hello_rpc, *hello_req, *hello_res,
				[ this, hello_req, hello_res ] (const boost::system::error_code &ec)
				{
					if (!ec)
					{
						LOG_DEBUG ("msg: " << hello_res->msg ());
						LOG_DEBUG ("seq: " << hello_res->seq ());
						LOG_DEBUG ("data: " << hello_res->data ());
						LOG_DEBUG ("crc: " << hello_res->crc ());

						uint32_t crc = hello_res->crc ();
						hello_res->clear_crc ();
						
						// validate message integrity
						yail::buffer tmp (MAX_MESSAGE_SIZE);
						tmp.resize (hello_res->ByteSize ());
						hello_res->SerializeToArray (tmp.data (), tmp.size ());
						boost::crc_32_type  result;
						result.process_bytes (tmp.data (), tmp.size ());
						bool valid = (result.checksum () == crc);
						LOG_DEBUG ("valid: " << valid);

						m_total_ok++;

						if (valid)
							m_total_valid++;

						do_call ();
					}
					else if (ec == yail::rpc::error::failure_response)
					{
						m_total_error++;

						do_call ();
					}
					else if (ec != boost::asio::error::operation_aborted)
					{
						LOG_ERROR ("error: " << ec);
					}
				});
		}
	}

	std::string m_name;
	yail::rpc::client<transport> m_rpc_client;
	yail::rpc::rpc<messages::hello_request, messages::hello_response> &m_hello_rpc;
	pargs m_pa;
	size_t m_seq;
	size_t m_total_ok;
	size_t m_total_error;
	size_t m_total_valid;
};

class provider
{
public:
	provider (const std::string &name,
					yail::io_service &io_service,
					yail::rpc::service<transport> &rpc_service,
					yail::rpc::rpc<messages::hello_request, messages::hello_response> &hello_rpc,
					pargs &pa):
		m_name (name),
		m_rpc_provider (rpc_service, "greeting_service"),
		m_hello_rpc (hello_rpc),
		m_timer (io_service),
		m_pa (pa),
		m_last_seq (0),
		m_total_call (0),
		m_total_reply_ok (0),
		m_total_reply_delayed (0),
		m_total_reply_error (0),
		m_total_valid (0)
	{
		m_rpc_provider.add_rpc (m_hello_rpc,
			[this] (yail::rpc::trans_context &tctx, const messages::hello_request &req)
			{
				// we make a copy here since we need to clear out the CRC message validation
				messages::hello_request	hello_req (req);
				LOG_DEBUG ("msg: " << hello_req.msg ());
				LOG_DEBUG ("seq: " << hello_req.seq ());
				LOG_DEBUG ("data: " << hello_req.data ());
				LOG_DEBUG ("crc: " << hello_req.crc ());

				// validate message integrity
				uint32_t crc = hello_req.crc ();
				hello_req.clear_crc ();
				yail::buffer tmp (MAX_MESSAGE_SIZE);
				tmp.resize (hello_req.ByteSize ());
				hello_req.SerializeToArray (tmp.data (), tmp.size ());
				boost::crc_32_type  result;
				result.process_bytes (tmp.data (), tmp.size ());
				bool valid = (result.checksum () ==  crc);
				LOG_DEBUG ("valid: " << valid);

				// update stats
				m_total_call++;

				if (valid)
					m_total_valid++;

				m_last_seq = hello_req.seq ();

				auto hello_res (std::make_shared<messages::hello_response> ());
				hello_res->set_msg ("hey there");
				hello_res->set_seq (m_last_seq);
				hello_res->set_data (std::string (m_pa.m_data_size, 'A'));
				// Add CRC to valiate message integrity
				{
					hello_res->clear_crc ();
					yail::buffer tmp (MAX_MESSAGE_SIZE);
					tmp.resize (hello_res->ByteSize ());
					hello_res->SerializeToArray (tmp.data (), tmp.size ());
					boost::crc_32_type  result;
					result.process_bytes (tmp.data (), tmp.size ());
					hello_res->set_crc (result.checksum ());
				}

				if (m_pa.m_reply_type == pargs::OK)
				{
					m_total_reply_ok++;
					m_rpc_provider.reply_ok (tctx, m_hello_rpc, *hello_res);
				}
				else if (m_pa.m_reply_type == pargs::ERROR)
				{
					m_total_reply_error++;
					m_rpc_provider.reply_error (tctx, m_hello_rpc);
				}
				else
				{
					m_total_reply_delayed++;
					m_rpc_provider.reply_delayed (tctx, m_hello_rpc);
					m_timer.expires_from_now (std::chrono::seconds (1));
					m_timer.async_wait (
						[this, &tctx, hello_res] (const boost::system::error_code &ec)
						{
							m_total_reply_ok++;
							m_rpc_provider.reply_ok (tctx, m_hello_rpc, *hello_res);
						});
				}
			});
	}

	~provider ()
	{}

	void print_stats () const
	{
		LOG_INFO (
			m_pa.m_name << ", " <<
			m_name << ", " <<
			"call:" << m_total_call << 
			", reply-ok:" << m_total_reply_ok <<
			", reply-delayed:" << m_total_reply_delayed <<
			", reply-error:" << m_total_reply_error <<
			", valid:" << m_total_valid);
	}

private:
	std::string m_name;
	yail::rpc::provider<transport> m_rpc_provider;
	yail::rpc::rpc<messages::hello_request, messages::hello_response> &m_hello_rpc;
	boost::asio::steady_timer m_timer;
	pargs m_pa;
	size_t m_last_seq;
	size_t m_total_call;
	size_t m_total_reply_ok;
	size_t m_total_reply_delayed;
	size_t m_total_reply_error;
	size_t m_total_valid;
};

int main (int argc, char* argv[])
{
	pargs pa;
  if (!pa.parse (argc, argv))
	{	
    return -1;
	}

	if (!pa.m_log_file.empty ())
		olog = new std::ofstream (pa.m_log_file.c_str ());

	client_count = pa.m_num_clients - 1;

	try
	{
		boost::asio::io_service io_service;
		yail::rpc::service<transport> rpc_service (io_service);
		yail::rpc::rpc<messages::hello_request, messages::hello_response> hello_rpc ("hello");

		// creater providers
		std::vector<std::unique_ptr<provider>> providers;
		for (size_t i = 0; i < pa.m_num_providers; ++i)
		{
			auto r (yail::make_unique<provider> ("provider"+std::to_string(i), io_service, rpc_service, hello_rpc, pa));
			providers.push_back (std::move (r));
		}

		// create clients
		std::vector<std::unique_ptr<client>> clients;
		for (size_t i = 0; i < pa.m_num_clients; ++i)
		{
			auto w (yail::make_unique<client> ("client"+std::to_string(i), rpc_service, hello_rpc, pa));
			clients.push_back (std::move(w));
		}

		boost::asio::signal_set signals (io_service, SIGINT, SIGTERM, SIGALRM);
		signals.async_wait (
			[&] (const boost::system::error_code &ec, int signal) 
				{ 
					for (const auto &w : clients) w->print_stats ();
					for (const auto &r : providers) r->print_stats ();

					io_service.stop (); 
				});

		io_service.run ();
	} 
	catch (const std::exception &ex)
	{
		LOG_ERROR (ex.what ());
	}

  return 0;
}

