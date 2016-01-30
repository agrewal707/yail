#ifndef YAIL_PUBSUB_IMPL_SERVICE_H
#define YAIL_PUBSUB_IMPL_SERVICE_H

#include <yail/pubsub/detail/service_impl.h>

#include <yail/log.h>

namespace yail {
namespace pubsub {

template <typename Transport>
service<Transport>::service (yail::io_service &io_service, const std::string &domain) :
	m_impl (yail::make_unique<impl_type> (io_service, domain))
{
	YAIL_LOG_FUNCTION (this);
}

template <typename Transport>
service<Transport>::service (yail::io_service &io_service, Transport &transport, const std::string &domain) :
	m_impl (make_unique<impl_type> (io_service, transport, domain))
{
	YAIL_LOG_FUNCTION (this);
}

template <typename Transport>
service<Transport>::~service ()
{
	YAIL_LOG_FUNCTION (this);
}

} // namespace pubsub
} // namespace yail

#endif // YAIL_PUBSUB_IMPL_SERVICE_H
