#ifndef YAIL_RPC_DETAIL_SERVICE_LOCATOR_H
#define YAIL_RPC_DETAIL_SERVICE_LOCATOR_H

#include <string>
#include <unordered_map>
#include <yail/exception.h>
#include <yail/rpc/transport/traits.h>

namespace yail {
namespace rpc {
namespace detail {

YAIL_DECLARE_EXCEPTION(duplicate_service);
YAIL_DECLARE_EXCEPTION(unknown_service);

template <typename Transport>
class service_locator
{
public:
	using transport_endpoint = typename Transport::endpoint;

	service_locator ();
	~service_locator ();

	void set_service_location (const std::string &service_name, const transport_endpoint &ep)
	{
		const auto it = m_service_map.find (service_name);
		if (it != m_service_map.end ())
		{
			YAIL_THROW_EXCEPTION (
				duplicate_service, "service already exists", 0);
		}
		
		m_service_map[service_name] = ep;
	}

	transport_endpoint get_service_location (const std::string &service_name) const
	{
		const auto it = m_service_map.find (service_name);
		if (it == m_service_map.end ())
		{
			// Check if transport suports derivation of ep from service_name
			const auto result = transport::traits<Transport>::service_name_to_ep (service_name);
			if (result.first)
			{
				return result.second;
			}
			else
			{
				YAIL_THROW_EXCEPTION (
					unknown_service, "service does not exist", 0);
			}
		}
		return it->second;
	}

private:
	using service_map = std::unordered_map<std::string, transport_endpoint>;
	service_map m_service_map;
};

} // namespace detail
} // namespace rpc
} // namespace yail


//
// Template Implementations
//
namespace yail {
namespace rpc {
namespace detail {

template <typename Transport>
service_locator<Transport>::service_locator()
{}

template <typename Transport>
service_locator<Transport>::~service_locator ()
{}

} // namespace detail
} // namespace rpc
} // namespace yail

#endif // YAIL_RPC_DETAIL_SERVICE_LOCATOR_H


