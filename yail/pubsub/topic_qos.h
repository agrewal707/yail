#ifndef YAIL_PUBSUB_TOPIC_QOS_H
#define YAIL_PUBSUB_TOPIC_QOS_H

//
// yail::topic
//
namespace yail {
namespace pubsub {

/**
 * @brief Represents topic qos
 *
 * Specifies how data is delivered from_data writers to readers.
 *
 * @ingroup yail_pubsub
 */
struct topic_qos
{
	/**
	 * @brief Specifies whether the data is volatile or not.
	 *
	 * Specifies durability characteristics of data published on the topic.
   *
	 * NONE : Data is not persisted after publication from data writer
   *
   * TRANSIENT_LOCAL : Data is persisted (in RAM) after publication from data writer
   *                  at the publisher with specified depth.
	 *
	 * @ingroup yail_pubsub
	 */
	struct durability
	{
		enum Type
		{
			NONE,
			TRANSIENT_LOCAL
		};

		durability ():
			m_type (NONE),
			m_depth (0)
		{}

		durability (Type type, size_t depth):
			m_type (type),
			m_depth (depth)
		{}

 		Type m_type;
		size_t m_depth;
	};

	/**
	 * @brief Constructs default topic qos
	 */
	topic_qos()
	{}

	/**
	 * @brief Constructs topic qos with durability
	 */
	topic_qos (const durability &d):
		m_durability (d)
	{}

	durability m_durability;
};

} // namespace pubsub
} // namespace yail

#endif // YAIL_PUBSUB_TOPIC_QOS_H
