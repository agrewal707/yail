#ifndef YAIL_PUBSUB_TOPIC_H
#define YAIL_PUBSUB_TOPIC_H

#include <string>
#include <yail/pubsub/topic_qos.h>

//
// Forward Declarations
//
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

template <typename T>
class topic_impl;

} // namespace detail
} // namespace pubsub
} // namespace yail

//
// yail::topic
//
namespace yail {
namespace pubsub {

/**
 * @brief Represents a named and typed data object.
 *
 * Topics enable data writers and readers to exchange
 * named data objects of specific type.
 *
 * @ingroup yail_pubsub
 */
template <typename T>
class topic
{
public:
	using impl_type = detail::topic_impl<T>;

	/**
	 * @brief Constructs pubsub service with default transport.
	 *
	 * @param[in] name The name of the topic.
	 *
	 */
	explicit topic (const std::string& name, const topic_qos &tq = topic_qos());

	/**
	 * @brief Destroys this object.
	 */
	~topic ();

private:
	template <typename U, typename Transport>
	friend class data_writer;

	template <typename U, typename Transport>
	friend class data_reader;

	const impl_type& get_impl () const { return *m_impl; }

	std::unique_ptr<impl_type> m_impl;
};

} // namespace pubsub
} // namespace yail

#include <yail/pubsub/impl/topic.h>

#endif // YAIL_PUBSUB_TOPIC_H
