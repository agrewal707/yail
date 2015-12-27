#include <iostream>
#include <sstream>

#include <boost/program_options.hpp>

#include <yail/rpc_client.h>

namespace po = boost::program_options;

struct pargs
{
	bool parse (int argc, char* argv[]);
};

bool pargs::parse (int argc, char* argv[])
{
	bool retval =  false;

 	po::options_description desc("hello-pub program options are:");
  desc.add_options()
    ("help", "produce help message")
    ;

  po::variables_map vm;        
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);    
  
  try 
	{
    if (vm.count("help") || argc == 1) {
      std::cout << desc << "\n";
      return false;
    }
    
		retval = true;
  } 
  catch (...) 
	{
    std::cout << desc << "\n";
  }

  return retval;
}


int main(int argc, char* argv[])
{
	pargs args;
  if (!args.parse (argc, argv))
    return 1;

	boost::asio:io_service io_service;


	yail::rpc_client<hello_request, hello_response> hello_client (io_service);

	hello_request hreq ("hi");
	yail::endpoint server_ep ("hello_server");
  hello_client.async_send (server_ep, hreq,
		[ ] (const boost::system::error &ec, const hello_response &rsp)
			{
				if (!ec)
				{
					std::cout << "response: " << rsp.msg () << std::endl;
				}
				else (ec != boost::asio::operation_aborted)
				{
					std::cout << "error: " << ec << std::endl;
				}
			});

	io_service.run ();

  return 0;
}
