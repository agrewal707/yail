#ifndef YAIL_PUBSUB_DATA_WRITER_H
#define YAIL_PUBSUB_DATA_WRITER_H

#include <yail/pubsub/service.h>
#include <yail/pubsub/topic.h>

//
// Forward Declarations
//
namespace yail {
namespace pubsub {
namespace transport {

class shmem;

} // namespace transport
} // namespace pubsub
} // namespace yail

namespace yail {
namespace pubsub {
namespace detail {

template <typename T, typename Transport>
class data_writer_impl;

} // namespace detail
} // namespace pubsub
} // namespace yail

//
// yail::pubsub::data_writer
//
namespace yail {
namespace pubsub {

/**
 * @brief A typed accessor to underlying publisher functionality.
 * 
 * @ingroup yail_pubsub
 *
 * The user application uses data writer to write values of
 * a specific data object that is published to subscribers.
 */
template <typename T, typename Transport = transport::shmem>
class YAIL_API data_writer
{
public:
	using impl_type = detail::data_writer_impl<T, Transport>;
	
	/**
	 * @brief Constructs typed data writer.
	 *
	 * @param[in] service The pubsub service object.
	 *
	 * @param[in] topic The typed topic.
	 */
	data_writer (service<Transport> &service, topic<T> &topic);

	/**
	 * @brief data writer is not copyable.
	 */
	data_writer (const data_writer&) = delete;
	data_writer& operator= (const data_writer&) = delete;

	/**
	 * @brief data writer is movable.
	 */
	data_writer (data_writer&&) = default;
	data_writer& operator= (data_writer&&) = default;

	/**
	 * @brief Destroys this data writer object.
	 */
	~data_writer ();

	/**
	 * @brief Writes data asynchronously.
	 *
	 * @param[in] t The value of data object to write.
	 *
	 * @param[in] handler The handler to be called on completion of write.
	 */
	template <typename Handler>
	void async_write (const T &t, const Handler &handler);

private:
	std::unique_ptr<impl_type> m_impl;
};

} // namespace pubsub
} // namespace yail


#include <yail/pubsub/impl/data_writer.h>

#endif // YAIL_PUBSUB_IMPL_DATA_WRITER_H
