#ifndef YAIL_PUBSUB_DATA_READER_H
#define YAIL_PUBSUB_DATA_READER_H

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
class data_reader_impl;

} // namespace detail
} // namespace pubsub
} // namespace yail

//
// yail::pubsub::data_reader
//
namespace yail {
namespace pubsub {

/**
 * @brief A typed accessor to underlying subcriber functionality.
 * 
 * @ingroup yail_pubsub
 *
 * The user application uses data reader to read values of
 * a specific data object that is subscribed from publishers.
 */
template <typename T, typename Transport = transport::shmem>
class YAIL_API data_reader
{
public:
	using impl_type = detail::data_reader_impl<T, Transport>;

	/**
	 * @brief Constructs typed data reader.
	 *
	 * @param[in] service The pubsub service object.
	 *
	 * @param[in] topic The typed topic.
	 */
	data_reader (service<Transport> &service, topic<T> &topic);

	/**
	 * @brief data reader is not copyable.
	 */
	data_reader (const data_reader&) = delete;
	data_reader& operator= (const data_reader&) = delete;

	/**
	 * @brief data reader is movable.
	 */
	data_reader (data_reader&&) = default;
	data_reader& operator= (data_reader&&) = default;

	/**
	 * @brief Destroys this data reader object.
	 */
	~data_reader ();

	/**
	 * @brief Reads data synchronously.
	 *
	 * @param[out] t The value of data object is read into this parameter.
	 * 
	 * @param[out] ec The error code returned on completion of the read operation.
	 * 
	 * @param[in] timeout The timeout in seconds. Defaults to indefinite wait.
	 * 
	 */	
	void read (T &t, boost::system::error_code &ec, const uint32_t timeout = 0);
	
	/**
	 * @brief Reads data asynchronously.
	 *
	 * @param[out] t The value of data object is read into this parameter.
	 *
	 * @param[in] handler The handler to be called on completion of read.
	 */
	template <typename Handler>
	void async_read (T &t, const Handler &handler);

	/**
	 * @brief Cancels pending asynchronous operations
	 */
	void cancel ();

private:
	std::unique_ptr<impl_type> m_impl;
};

} // namespace pubsub
} // namespace yail

#include <yail/pubsub/impl/data_reader.h>

#endif // YAIL_PUBSUB_DATA_READER_H
