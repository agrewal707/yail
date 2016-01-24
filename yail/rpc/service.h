#ifndef YAIL_RPC_SERVICE_H
#define YAIL_RPC_SERVICE_H

#include <string>
#include <yail/config.h>
#include <yail/io_service.h>
#include <yail/memory.h>

//
// Forward declarations
//
namespace yail {
namespace rpc {
namespace transport {

class unix_domain;

} // namepspace transport
} // namespace rpc
} // namespace yail

namespace yail {
namespace rpc {

template <typename Transport> 
class client;

template <typename Transport>
class provider;

} // namespace rpc
} // namespace yail

namespace yail {
namespace rpc {
namespace detail {

template <typename Transport>
class service_impl;

} // namespace detail
} // namespace rpc
} // namespace yail

//
// yail::rpc::service
//
namespace yail {
namespace rpc {

/**
 * @brief Provides core remote procedure call (rpc) functionality.
 * 
 * @ingroup yail_rpc
 */
template <typename Transport = transport::unix_domain>
class YAIL_API service
{
public:
	using impl_type = detail::service_impl<Transport>;
	using transport_endpoint = typename Transport::endpoint;


	/**
	 * @brief Constructs rpc service with default transport.
	 *
	 * @param[in] io_service The io service object.
	 *
	 */
	service (yail::io_service &io_service);

	/**
	 * @brief Constructs yail rpc service with specificed transport.
	 *
	 * @param[in] io_service The io service object.
	 *
	 * @param[in] transport The transport to use for rpc messaging.
	 *
	 */
	service (yail::io_service &io_service, Transport &transport);

	/**
	 * @brief service object is not copyable.
	 */
	service (const service&) = delete;
	service& operator= (const service&) = delete;

	/**
	 * @brief service object is movable.
	 */
	service (service&&) = default;
	service& operator= (service&&) = default;

	/**
	 * @brief Destroys this rpc service object.
	 */
	~service ();

	/**
	 * @brief Set the transport endpoint that hosts the rpc service.
	 *
	 * @param[in] service_name The name of the service provided by the RPC provider.
	 *
	 * @param[out] ep The transport endpoint where this service is located.
	 */
	void set_service_location (const std::string &service_name, const transport_endpoint &ep);

private:
	template <typename U> 
	friend class client;

	template <typename U>
	friend class provider;

	impl_type& get_impl () { return *m_impl; }

	std::unique_ptr<impl_type> m_impl;
};

} // namespace rpc
} // namespace yail

#include <yail/rpc/impl/service.h>

//
// yail rpc service types explicitly instantiated in the library itself
//
#if defined(YAIL_RPC_ENABLE_UNIX_DOMAIN_TRANSPORT)
#include <yail/rpc/transport/unix_domain.h>
extern template class yail::rpc::service<yail::rpc::transport::unix_domain>;
#endif

#endif // YAIL_RPC_SERVICE_H
