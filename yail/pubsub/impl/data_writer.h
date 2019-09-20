#ifndef YAIL_PUBSUB_IMPL_DATA_WRITER_H
#define YAIL_PUBSUB_IMPL_DATA_WRITER_H

#include <yail/pubsub/detail/data_writer_impl.h>

namespace yail {
namespace pubsub {

template <typename T, typename Transport>
data_writer<T, Transport>::data_writer (service<Transport> &service, const topic<T> &topic) :
	m_impl (make_unique<impl_type> (service.get_impl (), topic.get_impl ()))
{
	YAIL_LOG_FUNCTION (this);
}

template <typename T, typename Transport>
data_writer<T, Transport>::~data_writer ()
{
	YAIL_LOG_FUNCTION (this);
}

template <typename T, typename Transport> 
inline void data_writer<T, Transport>::write (const T &t, boost::system::error_code &ec, const uint32_t timeout)
{
	m_impl->write (t, ec, timeout);
}


template <typename T, typename Transport> 
template <typename Handler>
inline void data_writer<T, Transport>::async_write (const T &t, const Handler &handler)
{
	m_impl->async_write (t, handler);
}

} // namespace pubsub
} // namespace yail

#endif // YAIL_PUBSUB_IMPL_DATA_WRITER_H
