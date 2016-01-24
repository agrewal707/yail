#include <iostream>

#include <yail/rpc/service.h>
#include <yail/rpc/client.h>

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
		//rpc_service->set_service_location ("greeting_service", transport::endpoint ("/tmp/rpc_test"));
		//rpc_service->set_service_location ("greeting_service", transport::endpoint ("1.1.1.1", 4000));

		yail::rpc::client<transport> rpc_client (rpc_service);
		yail::rpc::rpc<messages::hello_request, messages::hello_response> hello_rpc ("hello");
		yail::rpc::rpc<messages::bye_request, messages::bye_response> bye_rpc ("bye");


		// sync rpc call
		messages::hello_request hello_req;
		hello_req.set_msg ("Hi");
		messages::hello_response hello_res;
		boost::system::error_code ec;
		rpc_client.call("greeting_service", hello_rpc, hello_req, hello_res, ec);
		if (!ec)
		{
			LOG_INFO (hello_res.msg ());
		}
		else
		{
			LOG_ERROR ("error: " << ec);
		}
		
		// async rpc call
		messages::bye_request bye_req;
		bye_req.set_msg ("Bye");
		messages::bye_response bye_res;
		rpc_client.async_call("greeting_service", bye_rpc, bye_req, bye_res,
			[ &bye_req, &bye_res ] (const boost::system::error_code &ec)
			{
				if (!ec)
				{
					LOG_INFO (bye_res.msg ());
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

