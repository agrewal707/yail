#ifndef YAIL_PUBSUB_DETAIL_PUBLISHER_H
#define YAIL_PUBSUB_DETAIL_PUBLISHER_H

#include <string>
#include <queue>
#include <functional>
#include <unordered_map>

#include <yail/io_service.h>
#include <yail/exception.h>
#include <yail/buffer.h>
#include <yail/memory.h>
#include <yail/pubsub/error.h>
#include <yail/pubsub/detail/messages/yail.pb.h>

namespace yail {
namespace pubsub {
namespace detail {

//
// Transport independent publisher functionality
//
struct publisher_common
{
	using send_handler = std::function<void (const boost::system::error_code &ec)>;

	publisher_common (yail::io_service &io_service, const std::string &domain);
	~publisher_common ();

	/// Add data writer to the set of data writers that are serviced by this publisher
	YAIL_API void add_data_writer (const void *id, const std::string &topic_name, 
		const std::string &topic_type_name, std::string &topic_id);

	/// Remove data writer from the set of data writers that are serviced by this publisher
	YAIL_API void remove_data_writer (const void *id, const std::string &topic_id);

	/// construct pubsub data message
	YAIL_API bool construct_pubsub_message (const std::string &topic_name,
		const std::string &topic_type_name, const std::string &topic_data, yail::buffer &buffer);

	struct send_operation
	{
		YAIL_API send_operation (const send_handler &handler);
		YAIL_API ~send_operation ();

		send_handler m_handler;
		yail::buffer m_buffer;
	};

	struct dw
	{
		dw (const std::string &topic_name, const std::string &topic_type_name);
		~dw ();

		std::string m_topic_name;
		std::string m_topic_type_name;
		std::queue<std::unique_ptr<send_operation>> m_op_queue;
	};
	using dw_map = std::unordered_map<const void*, std::unique_ptr<dw>>;

	yail::io_service &m_io_service;
	std::string m_domain;
	using topic_map = std::unordered_map<std::string, std::unique_ptr<dw_map>>;
	topic_map m_topic_map;
	int32_t m_mid;
};

//
// publisher
//
template <typename Transport>
class publisher : private publisher_common
{
public:
	publisher (yail::io_service &io_service, Transport &transport, const std::string &domain);
	~publisher ();

	/// Add data writer to the set of data writers that are serviced by this publisher
	void add_data_writer (const void *id, const std::string &topic_name, const std::string &topic_type_name, std::string &topic_id)
	{
		publisher_common::add_data_writer (id, topic_name, topic_type_name, topic_id);
	}

	/// Remove data writer from the set of data writers that are serviced by this publisher
	void remove_data_writer (const void *id, const std::string &topic_id)
	{
		publisher_common::remove_data_writer (id, topic_id);
	}

	/// Send topic data
	template <typename Handler>
	void async_send (const void *id, const std::string &topic_id, const std::string &topic_data, const Handler &handler)
	{
		// lookup data writer map
		auto it = m_topic_map.find (topic_id);
		if (it != m_topic_map.end ())
		{
			auto &dwmap = it->second;

			// look up data writer ctx
			auto it2 = dwmap->find (id);
			if (it2 != dwmap->end ())
			{
				auto &dwctx = it2->second;

				auto op (yail::make_unique<send_operation> (handler));
				if (construct_pubsub_message (dwctx->m_topic_name, dwctx->m_topic_type_name, topic_data, op->m_buffer))
				{
					m_transport.async_send (op->m_buffer,
						[this, &dwctx] (const boost::system::error_code &ec)
							{
								auto op = std::move (dwctx->m_op_queue.front ());
								dwctx->m_op_queue.pop ();

								op->m_handler (ec);
							});

					dwctx->m_op_queue.push (std::move(op));
				}
				else
				{
					m_io_service.post (std::bind (op->m_handler, yail::pubsub::error::system_error));
				}
			}
			else
			{
				m_io_service.post (std::bind (handler, yail::pubsub::error::unknown_data_writer));
			}
		}
		else
		{
			m_io_service.post (std::bind (handler, yail::pubsub::error::unknown_topic));
		}
	}

private:
	Transport &m_transport;
};

} // namespace detail
} // namespace pubsub
} // namespace yail

namespace yail {
namespace pubsub {
namespace detail {

template <typename Transport>
publisher<Transport>::publisher (yail::io_service &io_service, Transport &transport, const std::string &domain) :
	publisher_common (io_service, domain),
	m_transport (transport)
{}

template <typename Transport>
publisher<Transport>::~publisher ()
{}

} // namespace detail
} // namespace pubsub
} // namespace yail

#endif // YAIL_PUBSUB_DETAIL_PUBLISHER_H