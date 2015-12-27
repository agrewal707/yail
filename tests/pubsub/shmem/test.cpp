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
	size_t m_num_readers = 1;
	size_t m_num_msgs = 10;
	size_t m_data_size = 1024;

	bool parse (int argc, char* argv[])
	{
		bool retval =  false;

		po::options_description desc("options: ");
		desc.add_options ()
			("help", "print help message")
			("num-readers", po::value<size_t>(), "max num of data readers to instantiate.")
			("num-msgs", po::value<size_t>(), "max num number of messages to send")
			("data-size", po::value<size_t>(), "size of data to write in each message")
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
			
			if (vm.count("num-readers"))
				m_num_readers = vm["num-readers"].as<size_t> ();

			if (vm.count("num-msgs"))
				m_num_msgs = vm["num-msgs"].as<size_t> ();

			if (vm.count("data-size"))
				m_data_size = vm["data-size"].as<size_t> ();
	
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

	pid_t sub = fork ();
	if (0 == sub)
	{
		int rc = execlp(
				"local/bin/pubsub_shmem", 
				"pubsub_shmem2", 
				"--num-writers", "0", 
				"--num-readers", std::to_string(pa.m_num_readers).c_str (),
				"--log-file", "pubsub_shmem2.log",
				(char*)NULL);

		if (rc < 0)
		{
			LOG_ERROR("sub err: " << strerror(errno));
		}
	}
	else if (sub > 0)
	{
		usleep (10000);
		pid_t pub = fork ();
		if (0 == pub) 
		{
			int rc = execlp(
				"local/bin/pubsub_shmem", 
				"pubsub_shmem1", 
				"--num-writers", "1", 
				"--num-readers", std::to_string(pa.m_num_readers).c_str (), 
				"--num-msgs", std::to_string(pa.m_num_msgs).c_str (), 
				"--data-size", std::to_string (pa.m_data_size).c_str (), 
				"--log-file", "pubsub_shmem1.log",
				(char*)NULL);

			if (rc < 0)
			{
				LOG_ERROR("pub err: " << strerror(errno));
			}	
		}
		else if (pub > 0)
		{
			int status;
			waitpid (pub, &status, 0);
			usleep (10000);
			kill(sub, SIGTERM);
			usleep (10000);

			LOG_INFO (get_file_contents ("pubsub_shmem1.log"));
			LOG_INFO (get_file_contents ("pubsub_shmem2.log"));
		}
		else
		{
			LOG_ERROR("pub fork failed: " << pub);
		}
	}
	else
	{
		LOG_ERROR("sub fork failed: " << sub);
	}

	return 0;
}

