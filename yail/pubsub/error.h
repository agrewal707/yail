#ifndef YAIL_PUBSUB_ERROR_H
#define YAIL_PUBSUB_ERROR_H

#include <boost/system/error_code.hpp>
#include <yail/config.h>

namespace yail {
namespace pubsub {
namespace error {

enum errors
{
	success,
	system_error,
	unknown_data_writer,
	unknown_data_reader,
	unknown_topic,
	serialization_failed,
	deserialization_failed
};

extern YAIL_API const boost::system::error_category& get_category ();

} // namespace error
} // namespace pubsub
} // namespace yail

namespace boost {
namespace system {

template<> struct is_error_code_enum<yail::pubsub::error::errors>
{
  static const bool value = true;
};

} // namespace system
} // namespace boost

namespace yail {
namespace pubsub {
namespace error {

inline boost::system::error_code make_error_code (errors e)
{
  return boost::system::error_code (static_cast<int>(e), get_category());
}

} // namespace error
} // namespace pubsub
} // namespace yail

#endif // YAIL_PUBSUB_ERROR_H


