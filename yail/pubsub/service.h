#ifndef YAIL_PUBSUB_SERVICE_H
#define YAIL_PUBSUB_SERVICE_H

#include <yail/config.h>
#include <yail/io_service.h>
#include <yail/memory.h>

//
// Forward declarations
//
namespace yail {
namespace pubsub {
namespace transport {

class shmem;

} // namepspace transport
} // namespace pubsub
} // namespace yail

namespace yail {
namespace pubsub {

template <typename T, typename Transport> 
class data_writer;

template <typename T, typename Transport>
class data_reader;

} // namespace pubsub
} // namespace yail

namespace yail {
namespace pubsub {
namespace detail {

template <typename Transport>
class service_impl;

} // namespace detail
} // namespace pubsub
} // namespace yail

//
// yail::pubsub::service
//
namespace yail {
namespace pubsub {

/**
 * @brief Provides core publish/subscribe (pubsub) functionality.
 * 
 * @ingroup yail_pubsub
 */
template <typename Transport = transport::shmem>
class YAIL_API service
{
public:
	using impl_type = detail::service_impl<Transport>;

	/**
	 * @brief Constructs pubsub service with default transport.
	 *
	 * @param[in] io_service The io service object.
	 *
	 * @param[in] domain The pubsub domain.
	 */
	service (yail::io_service &io_service, const std::string &domain = std::string ());

	/**
	 * @brief Constructs yail pubsub service with specificed transport.
	 *
	 * @param[in] io_service The io service object.
	 *
	 * @param[in] transport The transport to use for pubsub messaging.
	 *
	 * @param[in] domain The pub/sub domain.
	 */
	service (yail::io_service &io_service, Transport &transport, const std::string &domain = std::string ());

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
	 * @brief Destroys this pubsub service object.
	 */
	~service ();

private:
	template <typename T, typename U> 
	friend class data_writer;

	template <typename T, typename U>
	friend class data_reader;

	impl_type& get_impl () { return *m_impl; }

	std::unique_ptr<impl_type> m_impl;
};

} // namespace pubsub
} // namespace yail

#include <yail/pubsub/impl/service.h>

//
// yail pubsub service types explicitly instantiated in the library itself
//
#if defined(YAIL_PUBSUB_ENABLE_SHMEM_TRANSPORT)
#include <yail/pubsub/transport/shmem.h>
extern template class yail::pubsub::service<yail::pubsub::transport::shmem>;
#endif

#if defined(YAIL_PUBSUB_ENABLE_UDP_TRANSPORT)
#include <yail/pubsub/transport/udp.h>
extern template class yail::pubsub::service<yail::pubsub::transport::udp>;
#endif

#endif // YAIL_PUBSUB_SERVICE_H
