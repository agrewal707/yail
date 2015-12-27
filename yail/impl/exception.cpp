#include <yail/exception.h>
#include <ostream>

namespace yail {

exception::exception (const std::string& what) throw() : 
	m_what (what), 
  m_where ("unknown"),
  m_error_code (0)
{}

exception::exception (const std::string& what, const std::string& where) throw() :
	m_what (what),
	m_where (where),
	m_error_code (0)
{}

exception::exception ( const std::string& what, const std::string& where, int error_code) throw() : 
	m_what (what),
  m_where (where),
  m_error_code (error_code)
{}

exception::~exception() throw()
{}

const char* exception::what() const throw() 
{
  return m_what.c_str();
}

const char* exception::where() const throw() 
{
  return m_where.c_str();
}

int exception::error_code() const throw()
{
  return m_error_code;
}

std::ostream& operator<<(std::ostream& os, const yail::exception& e)
{
  os << "What: " << e.what()
     << "Where: " << e.where()
     << "Errno:" << e.error_code()
     << std::endl;

  return os;
}

} // namespace yail

namespace yail {

YAIL_DEFINE_EXCEPTION (system_error);

} // namespace yail
