#ifndef YAIL_PUBSUB_TRANSPORT_SHMEM_H
#define YAIL_PUBSUB_TRANSPORT_SHMEM_H

#include <yail/io_service.h>
#include <yail/buffer.h>
#include <yail/memory.h>

//
// Forward declarations
//
namespace yail {
namespace pubsub {
namespace transport {
namespace detail {

class shmem_impl;

} // namespace detail
} // namespace transport
} // namespace pubsub
} // namespace yail

//
// yail::pubsub::transport::shmem
//
namespace yail {
namespace pubsub {
namespace transport {

/**
 * @brief Provides shared memory transport for pubsub messaging.
 * 
 * @ingroup yail_pubsub_transport
 */
class YAIL_API shmem
{
public:
	using impl_type = detail::shmem_impl;

	/**
	 * @brief Constructs transport.
	 *
	 * @param[in] io_service The io service object.
	 *
	 */
	shmem (yail::io_service &io_service);

	/**
	 * @brief shmem transport is not copyable.
	 */
	shmem (const shmem&) = delete;
	shmem& operator= (const shmem&) = delete;

	/**
	 * @brief shmem transport is movable.
	 */
	shmem (shmem&&) = default;
	shmem& operator= (shmem&&) = default;

	/**
	 * @brief Destroys this object.
	 */
	~shmem ();

	/**
	 * @brief Send message buffer asynchronously.
	 *
	 * @param[in] buffer The buffer to send.
	 *
	 * @param[in] handler The handler to be called on completion of send.
	 */
	template <typename Handler>
	void async_send (const yail::buffer &buffer, const Handler &handler);

	/**
	 * @brief Receive message into the specified buffer asynchronously.
	 *
	 * @param[out] buffer The buffer where received message is stored.
	 *
	 * @param[in] handler The handler to be called on completion of receive.
	 */
	template <typename Handler>
	void async_receive (yail::buffer &buffer, const Handler &handler);
	
private:
	std::unique_ptr<impl_type> m_impl;
};

} // namespace transport
} // namespace pubsub
} // namespace yail

#include <yail/pubsub/transport/impl/shmem.h>

#endif // YAIL_PUBSUB_TRANSPORT_SHMEM_H
