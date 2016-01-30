#ifndef YAIL_PUBSUB_IMPL_DATA_READER_H
#define YAIL_PUBSUB_IMPL_DATA_READER_H

#include <yail/pubsub/detail/data_reader_impl.h>

namespace yail {
namespace pubsub {

template <typename T, typename Transport>
data_reader<T, Transport>::data_reader (service<Transport> &service, topic<T> &topic) :
	m_impl (make_unique <impl_type> (service.get_impl (), topic.get_impl ()))
{
	YAIL_LOG_FUNCTION (this);
}

template <typename T, typename Transport>
data_reader<T, Transport>::~data_reader ()
{
	YAIL_LOG_FUNCTION (this);
}

template <typename T, typename Transport> 
template <typename Handler>
inline void data_reader<T, Transport>::async_read (T &t, const Handler &handler)
{
	m_impl->async_read (t, handler);
}

} // namespace pubsub
} // namespace yail

#endif // YAIL_PUBSUB_IMPL_DATA_READER_H
