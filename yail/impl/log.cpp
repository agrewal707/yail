#include <yail/log.h>

namespace yail {

std::ostream *logger = nullptr;

void set_logger (std::ostream *l)
{
	logger = l;	
}

} // namespace
