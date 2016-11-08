#include <iostream>
#include <fstream>
#include <boost/crc.hpp>
#include <boost/program_options.hpp>

#include <yail/pubsub/service.h>
#include <yail/pubsub/data_writer.h>
#include <yail/pubsub/data_reader.h>

#include "topics.h"

std::ostream *olog = &std::clog;
std::mutex log_mutex;

#define LOG_INFO(msg) do { \
	std::lock_guard<std::mutex> lock (log_mutex); \
	*olog << msg << std::endl; \
} while (0)
	
#define LOG_ERROR(msg) do { \
	std::lock_guard<std::mutex> lock (log_mutex); \
	*olog << msg << std::endl;	\
} while (0)
	
#ifndef NDEBUG
#define LOG_DEBUG(msg) do { \
	std::lock_guard<std::mutex> lock (log_mutex); \
	*olog << __FUNCTION__ << ":" << msg << std::endl; \
} while (0)
#else
#define LOG_DEBUG(msg)
#endif

const size_t MAX_MESSAGE_SIZE = 4096;
size_t writer_count;

namespace po = boost::program_options;
using transport = yail::pubsub::transport::udp;
using address = boost::asio::ip::address;

struct pargs
{
	std::string m_name;
	uint16_t m_local_port;
	std::string m_local_address;
	uint16_t m_multicast_port;
	std::string m_multicast_address;
	size_t m_num_writers;
	size_t m_num_readers;
	size_t m_num_msgs;
	size_t m_data_size;
	std::string m_log_file;
	bool m_multithreaded;

	pargs ():
		m_name (),
		m_local_address ("0.0.0.0"),
		m_multicast_port (40005),
		m_multicast_address ("239.225.0.1"),
		m_num_writers (0),
		m_num_readers (0),
		m_num_msgs (1),
		m_data_size (1024),
		m_log_file (),
		m_multithreaded (false)
	{}

	bool parse (int argc, char* argv[])
	{
		bool retval =  false;

		po::options_description desc("options: ");
		desc.add_options ()
			("help", "print help message")
			("local-port", po::value<uint16_t>()->required(), "Local port to use when sending messsages.")
			("local-address", po::value<std::string>(), "Local address to use when sending messsages.")
			("multicast-port", po::value<uint16_t>(), "Multicast port to use for sending/receiving messages to/from other participants.")
			("multicast-address", po::value<std::string>(), "Multicast address to use for sending/receiving messages to/from other participants.")
			("num-writers", po::value<size_t>()->required(), "max num of data writers to instantiate.")
			("num-readers", po::value<size_t>()->required(), "max num of data readers to instantiate.")
			("num-msgs", po::value<size_t>(), "max num number of messages to send")
			("data-size", po::value<size_t>(), "size of data to write in each message")
			("log-file", po::value<std::string>(), "log file")
			("multithreaded", "Reader/writer has separate thread.")
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
			
			if (vm.count("local-port"))
				m_local_port = vm["local-port"].as<uint16_t> ();

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
	
			if (vm.count("multithreaded"))
				m_multithreaded = true;
	
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
		alarm (5);
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
		m_total_sent (0),
		m_thread (),
		m_stopped (false)
	{
		if (m_pa.m_multithreaded) 
		{
			m_thread = std::thread (
				[this] () 
				{
					bool done = false;
					while (!done) 
					{						
						write ();
						if (m_seq == m_pa.m_num_msgs+1)
						{
							writer_done ();
							done = true;
						}
					}
				});
/*			
			m_thread2 = std::thread (
				[&] ()
				{
					while (!m_stopped)
					{
						yail::pubsub::data_writer<messages::hello, transport> hello_dw (pubsub_service, hello_topic);
						usleep(10);
					}
				});
*/
		}
		else 
		{
			do_write ();
		}		
	}
	
	~writer ()
	{
		if (m_pa.m_multithreaded)
		{
			m_thread.join ();
			//m_thread2.join ();
		}
	}
	
	void stop ()
	{
		m_stopped = true;		
	}
		
	void print_stats () const
	{
		LOG_INFO (
			m_pa.m_name << ", " <<
			m_name << ", " <<
			"sent:" << m_total_sent);
	}

private:
	void write ()
	{
		m_value.set_writer (m_name);
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
		
		boost::system::error_code ec;
		m_hello_dw.write (m_value, ec, 2);
		if (!ec)
		{
			LOG_DEBUG ("msg: " << m_value.msg ());
			LOG_DEBUG ("seq: " << m_value.seq ());
			LOG_DEBUG ("data: " << m_value.data ());
			LOG_DEBUG ("crc: " << m_value.crc ());

			m_total_sent++;
		}
		else if (ec != boost::asio::error::operation_aborted)
		{
			LOG_ERROR ("error: " << ec);
		}
	}
	
	void do_write ()
	{
		if (m_seq == m_pa.m_num_msgs+1)
		{
			writer_done ();
			return;
		}

		m_value.set_writer (m_name);
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
	std::thread m_thread;
	//std::thread m_thread2;
	bool m_stopped;
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
		m_last_seq_map (),
		m_total_rcvd (0),
		m_total_dropped (0),
		m_total_valid (0),
		m_thread (),
		m_stopped (false)
	{		
		if (m_pa.m_multithreaded) 
		{
			m_thread = std::thread (
				[this] () 
				{
					while (!m_stopped)
					{
						read ();
					}
				});
/*
			m_thread2 = std::thread (
				[&] ()
				{
					while (!m_stopped)
					{
						yail::pubsub::data_reader<messages::hello, transport> hello_dr (pubsub_service, hello_topic);
						usleep(10);
					}
				});
*/
		}
		else 
		{
			do_read ();
		}
	}
		
	~reader ()
	{
		if (m_pa.m_multithreaded)
		{
			m_thread.join ();
			//m_thread2.join ();
		}
	}
	
	void stop ()
	{
		m_stopped = true;
	}
	
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
	void read ()
	{
		boost::system::error_code ec;
		m_hello_dr.read (m_value, ec, 2);
		if (!ec)
		{
			LOG_DEBUG ("writer: " << m_value.writer ());
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

			const auto it = m_last_seq_map.find (m_value.writer ());
			if (it == m_last_seq_map.end ())
			{				
				m_last_seq_map[m_value.writer ()] = 0;
			}
			
			if (m_value.seq () != m_last_seq_map[m_value.writer ()]+1) 
			{
				m_total_dropped += m_value.seq () - m_last_seq_map[m_value.writer ()] - 1;
			}

			if (valid) 
				m_total_valid++;
				
			m_last_seq_map[m_value.writer ()] = m_value.seq ();
		}
		else if (ec != boost::asio::error::operation_aborted)
		{
			LOG_ERROR ("error: " << ec);
		}
	}
	
	void do_read ()
	{
		m_hello_dr.async_read (m_value,
			[ this ] (const boost::system::error_code &ec)
			{
				if (!ec)
				{
					do_read ();
					
					LOG_DEBUG ("writer: " << m_value.writer ());
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

					const auto it = m_last_seq_map.find (m_value.writer ());
					if (it == m_last_seq_map.end ())
					{
						m_last_seq_map[m_value.writer ()] = 0;
					}
			
					if (m_value.seq () != m_last_seq_map[m_value.writer ()]+1) 
					{
						m_total_dropped += m_value.seq () - m_last_seq_map[m_value.writer ()] - 1;
					}

					if (valid) 
						m_total_valid++;

					m_last_seq_map[m_value.writer ()] = m_value.seq ();
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
	std::map<std::string, size_t> m_last_seq_map;
	size_t m_total_rcvd;
	size_t m_total_dropped;
	size_t m_total_valid;
	std::thread m_thread;
	//std::thread m_thread2;
	bool m_stopped;
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
	
	writer_count = pa.m_num_writers - 1;

	try
	{
		boost::asio::io_service io_service;

		transport tr (io_service,
			transport::endpoint (address::from_string (pa.m_local_address), pa.m_local_port),
			transport::endpoint (address::from_string (pa.m_multicast_address), pa.m_multicast_port));

		yail::pubsub::service<transport> pubsub_service (io_service, tr);
		yail::pubsub::topic<messages::hello> hello_topic ("greeting");

		// creater readers
		std::vector<std::unique_ptr<reader>> readers;
		for (size_t i = 0; i < pa.m_num_readers; ++i)
		{
			auto r (yail::make_unique<reader> ("reader"+std::to_string(i), pubsub_service, hello_topic, pa));
			readers.push_back (std::move (r));
		}

		// create writers
		std::vector<std::unique_ptr<writer>> writers;
		for (size_t i = 0; i < pa.m_num_writers; ++i)
		{
			auto w (yail::make_unique<writer> ("writer"+std::to_string(i), pubsub_service, hello_topic, pa));
			writers.push_back (std::move(w));
		}

		boost::asio::signal_set signals (io_service, SIGINT, SIGTERM, SIGALRM);
		signals.async_wait (
			[&] (const boost::system::error_code &ec, int signal) 
				{ 
					for (const auto &w : writers) { w->print_stats (); w->stop (); }
					for (const auto &r : readers) { r->print_stats (); r->stop (); }

					io_service.stop ();
				});


		if (pa.m_multithreaded)
		{
			size_t thread_pool_size = pa.m_num_writers + pa.m_num_readers;

			// Create a pool of threads that all call io_service::run.
			std::vector<std::thread> threads;
			for (std::size_t i = 0; i < thread_pool_size; ++i)
			{
				std::thread thd ([&io_service] () { io_service.run (); });			
				threads.push_back (std::move (thd));
			}

			// Wait for all threads in the pool to exit.
			for (std::size_t i = 0; i < threads.size(); ++i)
				threads[i].join ();
		}
		else
		{
			io_service.run ();
		}
	} 
	catch (const std::exception &ex)
	{
		LOG_ERROR (ex.what ());
	}

  return 0;
}

