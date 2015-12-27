#include <yail/pubsub/service.h>

#if defined(YAIL_PUBSUB_ENABLE_SHMEM_TRANSPORT)
template class yail::pubsub::service<yail::pubsub::transport::shmem>;
#endif

#if defined(YAIL_PUBSUB_ENABLE_UDP_TRANSPORT)
template class yail::pubsub::service<yail::pubsub::transport::udp>;
#endif
