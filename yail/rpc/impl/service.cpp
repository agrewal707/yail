#include <yail/rpc/service.h>

#if defined(YAIL_RPC_ENABLE_UNIX_DOMAIN_TRANSPORT)
template class yail::rpc::service<yail::rpc::transport::unix_domain>;
#endif
