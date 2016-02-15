#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <cerrno>
#include <boost/program_options.hpp>

#define LOG_INFO(a) std::cerr << a << std::endl;
#define LOG_DEBUG(a) std::cerr << a << std::endl;
#define LOG_ERROR(a) std::cerr << a << std::endl;

namespace po = boost::program_options;

std::string get_file_contents(const char *filename)
{
  std::ifstream in(filename, std::ios::in | std::ios::binary);
  if (in)
  {
    std::string contents;
    in.seekg(0, std::ios::end);
    contents.resize(in.tellg());
    in.seekg(0, std::ios::beg);
    in.read(&contents[0], contents.size());
    in.close();
    return(contents);
  }
  throw(errno);
}

struct pargs
{
	size_t m_num_clients;
	size_t m_num_calls;
	enum call_type { SYNC, ASYNC } m_call_type;
	enum reply_type { OK, DELAYED, ERROR, NONE } m_reply_type;
	size_t m_data_size;
	uint32_t m_timeout;

	pargs ():
		m_num_clients (10),
		m_num_calls (1),
		m_call_type (SYNC),
		m_reply_type (OK),
		m_data_size (1024),
		m_timeout (0)
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
			("num-calls", po::value<size_t>(), "max num number of rpc calls to make")
			("call-type", po::value<unsigned>(), "type of call the client should make")
			("reply-type", po::value<unsigned>(), "type of reply sent that should be sent by the provider")
			("data-size", po::value<size_t>(), "size of data to write in each message")
			("timeout", po::value<uint32_t>(), "sync call timeout")
			;

		try 
		{
			po::variables_map vm;
			po::store (po::parse_command_line(argc, argv, desc), vm);
			po::notify (vm);
		
			if (vm.count("help") || argc < 1) 
			{
				LOG_INFO (desc);
				return false;
			}

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

			if (vm.count("timeout"))
				m_timeout = vm["timeout"].as<uint32_t> ();
	
			retval = true;
		} 
		catch (...) 
		{
			LOG_INFO (desc);
		}

		return retval;
	}
};

int main (int argc, char* argv[])
{
	pargs pa;
  if (!pa.parse (argc, argv))
	{	
    return 1;
	}

	pid_t provider = fork ();
	if (0 == provider)
	{
		int rc = execlp(
				"local/bin/rpc_unix_domain",
				"rpc_unix_domain1",
				"--num-providers", "1",
				"--num-clients", "0",
				"--reply-type", std::to_string (pa.m_reply_type).c_str (),
				"--call-type", std::to_string (pa.m_call_type).c_str (),
				"--log-file", "rpc_unix_domain1.log",
				(char*)NULL);

		if (rc < 0)
		{
			LOG_ERROR("provider err: " << strerror(errno));
		}
	}
	else if (provider > 0)
	{
		usleep (10000);
		pid_t client = fork ();
		if (0 == client)
		{
			int rc = execlp(
				"local/bin/rpc_unix_domain",
				"rpc_unix_domain2",
				"--num-providers", "0",
				"--num-clients", std::to_string (pa.m_num_clients).c_str (),
				"--num-calls", std::to_string (pa.m_num_calls).c_str (),
				"--call-type", std::to_string (pa.m_call_type).c_str (),
				"--data-size", std::to_string (pa.m_data_size).c_str (),
				"--timeout", std::to_string (pa.m_timeout).c_str (),
				"--log-file", "rpc_unix_domain2.log",
				(char*)NULL);

			if (rc < 0)
			{
				LOG_ERROR("client err: " << strerror(errno));
			}	
		}
		else if (client > 0)
		{
			int status;
			waitpid (client, &status, 0);
			usleep (10000);
			kill(provider, SIGTERM);
			usleep (10000);
			waitpid (provider, &status, 0);

			LOG_INFO (get_file_contents ("rpc_unix_domain1.log"));
			LOG_INFO (get_file_contents ("rpc_unix_domain2.log"));
		}
		else
		{
			LOG_ERROR("client fork failed: " << client);
		}
	}
	else
	{
		LOG_ERROR("provider fork failed: " << provider);
	}

	return 0;
}

