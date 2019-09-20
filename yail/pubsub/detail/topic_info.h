#ifndef YAIL_PUBSUB_DETAIL_TOPIC_INFO_H
#define YAIL_PUBSUB_DETAIL_TOPIC_INFO_H

#include <yail/pubsub/topic_qos.h>

//
// yail::detail::topic_info
//
namespace yail {
namespace pubsub {
namespace detail {

struct topic_info
{
	topic_info (
		const std::string &name,
		const std::string &type_name,
		const bool builtin,
		const topic_qos &tq):
		m_name (name),
		m_type_name (type_name),
		m_builtin (builtin),
		m_qos (tq)
	{}

	bool is_builtin () const
	{ return m_builtin; }
	bool is_durable () const
	{ return m_qos.m_durability.m_type != topic_qos::durability::NONE; }

	std::string m_name;
	std::string m_type_name;
	bool m_builtin;
	topic_qos m_qos;
};

} // namespace detail
} // namespace pubsub
} // namespace yail

#endif // YAIL_PUBSUB_DETAIL_TOPIC_INFO_H
