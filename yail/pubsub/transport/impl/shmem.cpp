#include <yail/pubsub/transport/shmem.h>
#include <yail/pubsub/transport/detail/shmem_impl.h>

#include <yail/log.h>

namespace yail {
namespace pubsub {
namespace transport {

//
// shmem
//
shmem::shmem (yail::io_service &io_service) :
	m_impl (make_unique<detail::shmem_impl> (io_service))
{
	YAIL_LOG_FUNCTION (this);
}

shmem::~shmem()
{
	YAIL_LOG_FUNCTION (this);
}

} // namespace transport
} // namespace pubsub
} // namespace yail
