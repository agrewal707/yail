#ifndef YAIL_RPC_ERROR_H
#define YAIL_RPC_ERROR_H

#include <boost/system/error_code.hpp>
#include <yail/config.h>

namespace yail {
namespace rpc {
namespace error {

enum errors
{
	success,
	system_error,
	unknown_rpc,
	failure_response,
	invalid_response,
	serialization_failed,
	deserialization_failed
};

extern YAIL_API const boost::system::error_category& get_category ();

} // namespace error
} // namespace rpc
} // namespace yail

namespace boost {
namespace system {

template<> struct is_error_code_enum<yail::rpc::error::errors>
{
  static const bool value = true;
};

} // namespace system
} // namespace boost

namespace yail {
namespace rpc {
namespace error {

inline boost::system::error_code make_error_code (errors e)
{
  return boost::system::error_code (static_cast<int>(e), get_category());
}

} // namespace error
} // namespace rpc
} // namespace yail

#endif // YAIL_RPC_ERROR_H


