#include <yail/pubsub/transport/udp.h>
#include <yail/pubsub/transport/detail/udp_impl.h>

#include <yail/log.h>

namespace yail {
namespace pubsub {
namespace transport {

//
// udp
//
udp::udp (yail::io_service &io_service) :
	m_impl (make_unique<detail::udp_impl> (io_service, endpoint (), endpoint ()))
{
	YAIL_LOG_FUNCTION (this);
}

udp::udp (yail::io_service &io_service,
	const udp::endpoint &local_ep, const udp::endpoint &ctrl_multicast_ep) :
	m_impl (make_unique<detail::udp_impl> (io_service, local_ep, ctrl_multicast_ep))
{
	YAIL_LOG_FUNCTION (this);
}

udp::~udp()
{
	YAIL_LOG_FUNCTION (this);
}

} // namespace transport
} // namespace pubsub
} // namespace yail
