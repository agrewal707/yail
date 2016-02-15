#include <iostream>
#include <boost/asio/steady_timer.hpp>

#include <yail/rpc/service.h>
#include <yail/rpc/provider.h>

#include "rpcs.h"

#define LOG_INFO(msg) std::cout << msg << std::endl
#define LOG_ERROR(msg) std::cerr << msg << std::endl

using transport = yail::rpc::transport::unix_domain;

int main(int argc, char* argv[])
{
	try
	{
		boost::asio::io_service io_service;
		yail::rpc::service<transport> rpc_service (io_service);
		yail::rpc::provider<transport> rpc_provider (rpc_service, "greeting_service");
		yail::rpc::rpc<messages::hello_request, messages::hello_response> hello_rpc ("hello");
		yail::rpc::rpc<messages::bye_request, messages::bye_response> bye_rpc ("bye");

		boost::asio::steady_timer timer (io_service);

		rpc_provider.add_rpc (hello_rpc,
			[&] (yail::rpc::trans_context &tctx, const messages::hello_request &req)
			{
#if 1
				// fill response 'res'
				messages::hello_response res;
				res.set_msg ("hey there");
				rpc_provider.reply_ok (tctx, hello_rpc, res);
#endif				

#if 0
				// reply with error
				rpc_provider.reply_error (tctx, hello_rpc);
#endif

#if 0			
				// delayed reply
				rpc_provider.reply_delayed (tctx, hello_rpc);
				timer.expires_from_now (std::chrono::seconds (5));
				timer.async_wait (
					[&] (const boost::system::error_code &ec)
					{
						// fill response 'res'
						messages::hello_response res;
						res.set_msg ("hey there..sorry for the delay !");
						rpc_provider.reply_ok (tctx, hello_rpc, res);
					});
#endif				
			});

		rpc_provider.add_rpc (bye_rpc,
			[&] (yail::rpc::trans_context &tctx, const messages::bye_request &req)
			{
				// fill response 'res'
				messages::bye_response res;
				res.set_msg ("see ya");
				rpc_provider.reply_ok (tctx, bye_rpc, res);
			});
		
		boost::asio::signal_set signals (io_service, SIGINT, SIGTERM);
		signals.async_wait (
			[&] (const boost::system::error_code &ec, int signal) 
				{
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
