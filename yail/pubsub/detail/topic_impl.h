#ifndef YAIL_PUBSUB_DETAIL_TOPIC_IMPL_H
#define YAIL_PUBSUB_DETAIL_TOPIC_IMPL_H

#include <yail/pubsub/topic.h>
#include <yail/pubsub/topic_traits.h>

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
	explicit topic_impl (const std::string& name);

	~topic_impl ();

	std::string get_name () const
	{
		return m_name;
	}

	std::string get_type_name () const
	{
		return yail::pubsub::topic_type_support<T>::get_name ();
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
};

} // namespace detail
} // namespace pubsub
} // namespace yail

namespace yail {
namespace pubsub {
namespace detail {

template <typename T>
topic_impl<T>::topic_impl (const std::string& name) :
	m_name {name}
{}

template <typename T>
topic_impl<T>::~topic_impl ()
{}

} // namespace detail
} // namespace pubsub
} // namespace yail

#endif // YAIL_PUBSUB_DETAIL_TOPIC_IMPL_H
