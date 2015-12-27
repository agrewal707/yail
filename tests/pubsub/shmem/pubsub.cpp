#include <iostream>
#include <fstream>
#include <boost/crc.hpp>
#include <boost/program_options.hpp>

#include <yail/pubsub/service.h>
#include <yail/pubsub/data_writer.h>
#include <yail/pubsub/data_reader.h>

#include "topics.h"

std::ostream *olog;

#define LOG_INFO(msg) *olog << msg << std::endl
#define LOG_ERROR(msg) *olog << msg << std::endl
#ifndef NDEBUG
#define LOG_DEBUG(msg) *olog << __FUNCTION__ << ":" << msg << std::endl
#else
#define LOG_DEBUG(msg)
#endif

const size_t MAX_MESSAGE_SIZE = 4096;
size_t writer_count;

namespace po = boost::program_options;
using transport = yail::pubsub::transport::shmem;

struct pargs
{
	std::string m_name;
	size_t m_num_writers;
	size_t m_num_readers;
	size_t m_num_msgs;
	size_t m_data_size;
	std::string m_log_file;

	pargs ():
		m_name (),
		m_num_writers (0),
		m_num_readers (0),
		m_num_msgs (1),
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
			("num-writers", po::value<size_t>()->required(), "max num of data writers to instantiate.")
			("num-readers", po::value<size_t>()->required(), "max num of data readers to instantiate.")
			("num-msgs", po::value<size_t>(), "max num number of messages to send")
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
			
			if (vm.count("num-writers"))
				m_num_writers = vm["num-writers"].as<size_t> ();

			if (vm.count("num-readers"))
				m_num_readers = vm["num-readers"].as<size_t> ();

			if (vm.count("num-msgs"))
				m_num_msgs = vm["num-msgs"].as<size_t> ();

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

void writer_done ()
{
	if (!writer_count--)
	{
		alarm (1);
	}
}

class writer
{
public:
	writer (const std::string &name,
					yail::pubsub::service<transport> &pubsub_service, 
					yail::pubsub::topic<messages::hello> &hello_topic, 
					pargs &pa):
		m_name (name),
		m_hello_dw (pubsub_service, hello_topic),
		m_pa (pa),
		m_seq (1),
		m_total_sent (0)
	{
		do_write ();	
	}

	~writer ()
	{}

	void print_stats () const
	{
		LOG_INFO (
			m_pa.m_name << ", " <<
			m_name << ", " <<
			"sent:" << m_total_sent);
	}

private:
	void do_write ()
	{
		if (m_seq == m_pa.m_num_msgs+1)
		{
			writer_done ();
			return;
		}

		m_value.set_msg ("hello");
		m_value.set_seq (m_seq++);
		m_value.set_data (std::string (m_pa.m_data_size, 'A'));
		m_value.clear_crc ();

		// Add CRC to valiate message integrity
		yail::buffer tmp (MAX_MESSAGE_SIZE);
		tmp.resize (m_value.ByteSize ());
		m_value.SerializeToArray (tmp.data (), tmp.size ());
		boost::crc_32_type  result;
		result.process_bytes (tmp.data (), tmp.size ());
		m_value.set_crc (result.checksum ());

		m_hello_dw.async_write (m_value,
			[ this ] (const boost::system::error_code &ec)
			{
				if (!ec)
				{
					LOG_DEBUG ("msg: " << m_value.msg ());
					LOG_DEBUG ("seq: " << m_value.seq ());
					LOG_DEBUG ("data: " << m_value.data ());
					LOG_DEBUG ("crc: " << m_value.crc ());

					m_total_sent++;

					do_write ();
				}
				else if (ec != boost::asio::error::operation_aborted)
				{
					LOG_ERROR ("error: " << ec);
				}
			});
	}

	std::string m_name;
	yail::pubsub::data_writer<messages::hello, transport> m_hello_dw;
	messages::hello m_value;
	pargs m_pa;
	size_t m_seq;
	size_t m_total_sent;
};

class reader
{
public:
	reader (const std::string &name,
					yail::pubsub::service<transport> &pubsub_service, 
					yail::pubsub::topic<messages::hello> &hello_topic, 
					pargs &pa):
		m_name (name),
		m_hello_dr (pubsub_service, hello_topic),
		m_pa (pa),
		m_last_seq (0),
		m_total_rcvd (0),
		m_total_dropped (0),
		m_total_valid (0)
	{
		do_read ();	
	}

	~reader ()
	{}

	void print_stats () const
	{
		LOG_INFO (
			m_pa.m_name << ", " <<
			m_name << ", " <<
			"rcvd:" << m_total_rcvd << 
			", dropped:" << m_total_dropped <<
			", valid:" << m_total_valid);
	}

private:
	void do_read ()
	{
		m_hello_dr.async_read (m_value,
			[ this ] (const boost::system::error_code &ec)
			{
				if (!ec)
				{
					do_read ();

					LOG_DEBUG ("msg: " << m_value.msg ());
					LOG_DEBUG ("seq: " << m_value.seq ());
					LOG_DEBUG ("data: " << m_value.data ());
					LOG_DEBUG ("crc: " << m_value.crc ());
					uint32_t crc = m_value.crc ();
					m_value.clear_crc ();

					// validate message integrity
					yail::buffer tmp (MAX_MESSAGE_SIZE);
					tmp.resize (m_value.ByteSize ());
					m_value.SerializeToArray (tmp.data (), tmp.size ());
					boost::crc_32_type  result;
					result.process_bytes (tmp.data (), tmp.size ());
					bool valid = (result.checksum () ==  crc);
					LOG_DEBUG ("valid: " << valid);

					// update stats
					m_total_rcvd++;

					if (m_value.seq () != m_last_seq+1) 
					{
						m_total_dropped += m_value.seq () - m_last_seq - 1;
					}	

					if (valid) 
						m_total_valid++;

					m_last_seq = m_value.seq ();
				}
				else if (ec != boost::asio::error::operation_aborted)
				{
					LOG_ERROR ("error: " << ec);
				}
			});
	}

	std::string m_name;
	yail::pubsub::data_reader<messages::hello, transport> m_hello_dr;
	messages::hello m_value;
	pargs m_pa;
	size_t m_last_seq;
	size_t m_total_rcvd;
	size_t m_total_dropped;
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
	else
		olog = &std::clog;

	writer_count = pa.m_num_writers - 1;

	try
	{
		boost::asio::io_service io_service;
		transport tr (io_service);
		yail::pubsub::service<transport> pubsub_service (io_service, tr);
		yail::pubsub::topic<messages::hello> hello_topic ("greeting");

		// create writers
		std::vector<std::unique_ptr<writer>> writers;
		for (size_t i = 0; i < pa.m_num_writers; ++i)
		{
			auto w (yail::make_unique<writer> ("writer"+std::to_string(i), pubsub_service, hello_topic, pa));
			writers.push_back (std::move(w));
		}

		// creater readers
		std::vector<std::unique_ptr<reader>> readers;
		for (size_t i = 0; i < pa.m_num_readers; ++i)
		{
			auto r (yail::make_unique<reader> ("reader"+std::to_string(i), pubsub_service, hello_topic, pa));
			readers.push_back (std::move (r));
		}

		boost::asio::signal_set signals (io_service, SIGINT, SIGTERM, SIGALRM);
		signals.async_wait (
			[&] (const boost::system::error_code &ec, int signal) 
				{ 
					for (const auto &w : writers) w->print_stats ();
					for (const auto &r : readers) r->print_stats ();

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

