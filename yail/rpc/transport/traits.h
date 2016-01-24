#ifndef YAIL_RPC_TRANSPORT_TRAITS_H
#define YAIL_RPC_TRANSPORT_TRAITS_H

#include <utility>

namespace yail {
namespace rpc {
namespace transport {

template <typename Transport>
struct traits
{
	using transport_endpoint = typename Transport::endpoint;

	std::pair<bool, transport_endpoint> 
	static service_name_to_ep (const std::string &service_name)
	{
		return std::make_pair (false, transport_endpoint ());
	}
};

} // namespace transport
} // namespace rpc
} // namespace yail

#endif // YAIL_RPC_TRANSPORT_TRAITS_H
