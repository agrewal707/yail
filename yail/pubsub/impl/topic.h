#ifndef YAIL_PUBSUB_IMPL_TOPIC_H
#define YAIL_PUBSUB_IMPL_TOPIC_H

#include <yail/pubsub/detail/topic_impl.h>

namespace yail {
namespace pubsub {

template <typename T>
topic<T>::topic (const std::string& name) :
	m_impl (*(new impl_type {name}))
{}

template <typename T>
topic<T>::~topic ()
{}

} // namespace pubsub
} // namespace yail

#endif // YAIL_PUSUB_IMPL_TOPIC_H
