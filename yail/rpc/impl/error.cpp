#include <yail/rpc/error.h>

namespace yail {
namespace rpc {
namespace error {

class category : public boost::system::error_category
{
public:
  const char* name() const noexcept(true) //BOOST_ASIO_ERROR_CATEGORY_NOEXCEPT
  {
    return "yail.rpc";
  }

  std::string message(int value) const
  {
		if (value == error::success)
			return "succes";
		if (value == error::system_error)
			return "system error";
		if (value == error::invalid_response)
			return "invalid response from the provider";
		if (value == error::failure_response)
			return "provider responded with reply_error ()";
		if (value == error::deserialization_failed)
			return "fail to deserialize rpc request/response";

   return "asio.rpc error";
  }
};

const boost::system::error_category& get_category()
{
  static category instance;
  return instance;
}

} // namespace error
} // namespace rpc
} // namespace yail
