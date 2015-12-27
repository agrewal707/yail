#include <yail/pubsub/error.h>

namespace yail {
namespace pubsub {
namespace error {

class category : public boost::system::error_category
{
public:
  const char* name() const noexcept(true) //BOOST_ASIO_ERROR_CATEGORY_NOEXCEPT
  {
    return "yail.pubsub";
  }

  std::string message(int value) const
  {
		// TODO
    return "asio.pusub error";
  }
};

const boost::system::error_category& get_category()
{
  static category instance;
  return instance;
}

} // namespace error
} // namespace pubsub
} // namespace yail
