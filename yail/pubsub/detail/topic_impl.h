#ifndef YAIL_PUBSUB_DETAIL_TOPIC_IMPL_H
#define YAIL_PUBSUB_DETAIL_TOPIC_IMPL_H

#include <yail/pubsub/topic.h>
#include <yail/pubsub/topic_traits.h>
#include <yail/pubsub/detail/topic_info.h>

//
// yail::detail::topic_impl
//
namespace yail {
namespace pubsub {
namespace detail {

template <typename T>
class topic_impl
{
public:
	explicit topic_impl (const std::string& name, const topic_qos &tq = topic_qos());

	~topic_impl ();

	topic_info get_info () const
	{
		return topic_info (
			m_name,
			yail::pubsub::topic_type_support<T>::get_name (),
			yail::pubsub::topic_type_support<T>::is_builtin (),
			m_topic_qos);
	}

	bool serialize (const T &t, std::string *out) const
	{
		return yail::pubsub::topic_type_support<T>::serialize (t, out);
	}

	bool deserialize (T &t, const std::string &in) const
	{
		return yail::pubsub::topic_type_support<T>::deserialize (t, in);
	}

private:
	std::string m_name;
	topic_qos m_topic_qos;
};

} // namespace detail
} // namespace pubsub
} // namespace yail

namespace yail {
namespace pubsub {
namespace detail {

template <typename T>
topic_impl<T>::topic_impl (const std::string& name, const topic_qos &tq) :
	m_name {name},
	m_topic_qos (tq)
{}

template <typename T>
topic_impl<T>::~topic_impl ()
{}

} // namespace detail
} // namespace pubsub
} // namespace yail

#endif // YAIL_PUBSUB_DETAIL_TOPIC_IMPL_H
