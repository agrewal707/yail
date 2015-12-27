#ifndef YAIL_EXCEPTION_H
#define YAIL_EXCEPTION_H

#include <yail/config.h>
#include <exception>
#include <string>

#define YAIL_DECLARE_EXCEPTION(exception_name)		           \
  class YAIL_API exception_name                              \
		: public virtual yail::exception                         \
	{	                                                         \
  public:						                                         \
		exception_name (                                         \
			const std::string& what) throw ();	                   \
		exception_name (                                         \
			const std::string& what,		                           \
			const std::string& where) throw ();	                   \
		exception_name (                                         \
			const std::string& what,		                           \
			const std::string& where,		                           \
			int error_code) throw ();			                         \
		virtual ~exception_name() throw ();			                 \
  };

#define YAIL_DEFINE_EXCEPTION(exception_name)					       \
  exception_name::exception_name (                           \
		const std::string& what) throw()		                     \
    : exception(what)                                        \
  {}						                                             \
  exception_name::exception_name (                           \
		const std::string& what,			                           \
		const std::string& where) throw()	                       \
    : exception(what, where)                                 \
	{}					                                               \
  exception_name::exception_name (                           \
		const std::string& what,			                           \
		const std::string& where,		                             \
		int error_code) throw ()			                           \
    : exception (what, where, error_code)                    \
	{}					                                               \
  exception_name::~exception_name() throw()                  \
	{}

#define YAIL_THROW_EXCEPTION(exception_name, what, error_code) \
	throw exception_name(what, __FILE__, error_code)

namespace yail {

class YAIL_API exception 
	: public virtual std::exception {
public:
	exception (const std::string& what) throw ();

	exception (const std::string& what, const std::string& where) throw ();

	exception (const std::string& what, const std::string& where, int error_code) throw ();

	virtual ~exception() throw ();

	virtual const char* what () const throw ();
	virtual const char* where () const throw ();
	virtual int error_code () const throw ();

private:
	std::string m_what;
	std::string m_where;
	int m_error_code;
};

} // namespace yail

YAIL_API std::ostream& operator<<(std::ostream& os, const yail::exception& e);

//
// General exceptions
//
namespace yail {

YAIL_DECLARE_EXCEPTION (system_error);

} // namespace yail

#endif // YAIL_EXCEPTION_H
