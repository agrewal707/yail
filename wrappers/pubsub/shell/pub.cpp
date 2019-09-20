#include <iostream>
#include <boost/program_options.hpp>
#include <yail/pubsub/service.h>
#include <yail/pubsub/data_writer.h>
#include <yail/pubsub/topic_traits.h>
#include <wrappers/pubsub/shell/messages/general.pb.h>

using yail::wrappers::pubsub::shell::messages::general;

REGISTER_TOPIC_TRAITS(yail::wrappers::pubsub::shell::messages::general);

#define LOG_INFO(msg) std::cout << msg << std::endl
#define LOG_ERROR(msg) std::cerr << msg << std::endl

namespace po = boost::program_options;
using transport = yail::pubsub::transport::shmem;

int main(int argc, char* argv[])
{
	try
	{
		po::options_description desc("options: ");
		desc.add_options ()
			("help", "print help message")
			("topic", po::value<std::string> (), "name of the topic.")
			("name", po::value<std::vector<std::string>> (), "name(s) of the parameters.")
			("value", po::value<std::vector<std::string>> (), "value(s) of the parameters.")
			;

		po::variables_map vm;
		po::store (po::parse_command_line(argc, argv, desc), vm);
		po::notify (vm);
	
		if (vm.count("help") || argc < 3) 
		{
			LOG_INFO (desc);
			return -1;
		}

		std::string topic;
		if (vm.count("topic"))
		{
			topic = vm["topic"].as<std::string> ();
		}
		std::vector<std::string> names, values;
		if (vm.count("name"))
		{
			names = vm["name"].as<std::vector<std::string>> ();
		}
		if (vm.count("value"))
		{
			values = vm["value"].as<std::vector<std::string>> ();
		}
		
		if (names.size () != values.size ())
		{
			LOG_ERROR ("provide a value for each name and vice versa");
			return -1;
		}

		boost::asio::io_service io_service;
		yail::pubsub::service<transport> pubsub_service (io_service);
		yail::pubsub::topic<general> general_topic (topic);
		yail::pubsub::data_writer<general> general_dw (pubsub_service, general_topic);

		general msg;
		for (size_t i = 0; i < names.size (); ++i)
		{
			auto *pair = msg.add_pair ();
			pair->set_name (names[i]);
			pair->set_value (values[i]);
		}

		io_service.post([&] ()
		{
			boost::system::error_code ec;
			general_dw.write (msg, ec, 5);
			if (ec)
			{
				LOG_ERROR ("error: " << ec);
			}

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

