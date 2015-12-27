#include <iostream>
#include <sstream>

#include <boost/program_options.hpp>

#include <yail/rpc_server.h>

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

	yail::endpoint ep ("hello_server");
	yail::rpc_server<hello_request, hello_response> server (io_service, ep);
  hello_server.async_receive (
		[ &hello_server ] (const boost::system::error &ec, const hello_request &req)
			{
				if (!ec)
				{
					std::cout << "request: " << req.msg () << std::endl;

					hello_response rsp ("Hey");
					hello_server->reply (rsp);
				}
				else (ec != boost::asio::operation_aborted)
				{
					std::cout << "error: " << ec << std::endl;
				}
			});

	io_service.run ();

  return 0;
}
