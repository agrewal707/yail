#include <yail/buffer.h>

namespace yail {

buffer::buffer ()
{}

buffer::buffer (const size_t max_size)
{
	m_data.resize (max_size);
}

buffer::buffer (const std::string& data):
	m_data (data.begin(), data.end())
{}

buffer::~buffer ()
{}

} // namespace yail
