#ifndef YAIL_BUFFER_H
#define YAIL_BUFFER_H

#include <memory>
#include <vector>

#include <yail/config.h>
#include <yail/log.h>


namespace yail {

class YAIL_API buffer
{
public:
	buffer (const size_t max_size);
	buffer (const std::string& data);
	~buffer ();

	buffer (const buffer &) = default;
	buffer& operator= (const buffer &) = default;

	buffer (buffer &&) = default;
	buffer& operator= (buffer &&) = default;

	bool empty () const
	{
		return m_data.empty ();
	}

	char* data ()
	{
		return m_data.data ();
	}

	const char* data () const
	{
		return m_data.data ();
	}

	size_t size () const
	{
		return m_data.size ();
	}

	void resize (size_t size)
	{
		m_data.resize (size);
	}

private:
	std::vector<char> m_data;
};

} // namespace yail

#endif // YAIL_BUFFER_H
