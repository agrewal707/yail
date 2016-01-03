#include <iostream>

#include <yail/rpc/service.h>
#include <yail/rpc/client.h>

#include "messages.h"

#define LOG_INFO(msg) std::cout << msg << std::endl
#define LOG_ERROR(msg) std::cerr << msg << std::endl

using transport = yail::rpc::transport::unix;

int main(int argc, char* argv[])
{
	try
	{
		boost::asio::io_service io_service;
		yail::rpc::service<transport> rpc_service (io_service);
		//rpc_service->set_service_location ("greeting_service", transport::endpoint ("/tmp/rpc_test"));
		//rpc_service->set_service_location ("greeting_service", transport::endpoint ("1.1.1.1", 4000));

		yail::rpc::client<transport> rpc_client (rpc_service);
		yail::rpc::rpc<messages::hello_request, messages::hello_response> hello_rpc ("hello");
		yail::rpc::rpc<messages::byte_request, messages::bye_response> bye_rpc ("bye");

		// sync rpc call
		messages::hello_request req;
		messages::hello_response res;
		boost::system::error_code ec;
		rpc_client.call("greeting_service", hello_rpc, req, res, ec);
		if (!ec)
		{
			//todo
		}
		else
		{
			LOG_ERROR ("error: " << ec);
		}

		// async rpc call
		messages::hello_request req;
		messages::hello_response res;
		boost::system::error_code ec;
		rpc_client.async_call("greeting_service", hello_rpc, req, res,
			[ &req, &res ](this) (const boost::system::error_code &ec)
			{
				if (!ec)
				{
					//todo
				}
				else
				{
					LOG_ERROR ("error: " << ec);
				}
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

