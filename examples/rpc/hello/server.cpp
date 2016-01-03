#include <iostream>

#include <yail/rpc/service.h>
#include <yail/rpc/rpc_server.h>

#include "topics.h"

#define LOG_INFO(msg) std::cout << msg << std::endl
#define LOG_ERROR(msg) std::cerr << msg << std::endl

using transport = yail::rpc::transport::unix;

int main(int argc, char* argv[])
{
	try
	{
		boost::asio::io_service io_service;
		yail::rpc::service<transport> rpc_service (io_service);
		yail::rpc::provider<transport> rpc_provider (rpc_service, "hello_service");
		yail::rpc::rpc<messages::hello_request, messages::hello_response> hello_rpc ("hello");
		rpc_provider.add_rpc (hello_rpc,
			[&] (yail::rpc::trans_context &tctx, const messages::hello_request &req)
			{
				// fill response 'res'
				// messages::hello_response res;
				rpc_provider->reply_ok (tctx, hello_rpc, res);

				// delayed reply
				rpc_provider->reply_delayed (tctx, hello_rpc);
				async_xyz (xyz,
					[ & ] (const boost::system::error_code &ec)
					{
						// fill response 'res'
						// messages::hello_response res;
						rpc_provider->reply_ok (tctx, hello_rpc, res);
					}

				// reply with error
				rpc_provider->reply_error (tctx, hello_rpc);
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
