#ifndef YAIL_TRANSPORT_TRAITS_H
#define YAIL_TRANSPORT_TRAITS_H

#include <utility>

namespace yail {
namespace rpc {
namespace transport {

template <typename Transport>
struct transport_traits
{
	using transport_endpoint = typename Transport::endpoint;

	std::pair<bool, transport_endpoint> 
	derive_ep_from_service_name (const std::string &service_name)
	{
		return std::make_pair (false, transport_endpoint ());
	}
}

} // namespace transport
} // namespace rpc
} // namespace yail

#endif // YAIL_TRANSPORT_TRAITS_H
