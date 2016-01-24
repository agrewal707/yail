#ifndef YAIL_RPC_H
#define YAIL_RPC_H

#include <string>

//
// Forward Declarations
//
namespace yail {
namespace rpc {

template <typename Transport> 
class client;

template <typename Transport> 
class provider;

} // namespace rpc
} // namespace yail

namespace yail {
namespace rpc {
namespace detail {

template <typename Request, typename Response>
class rpc_impl;

} // namespace detail
} // namespace rpc
} // namespace yail

//
// yail::rpc
//
namespace yail {
namespace rpc {

/**
 * @brief Represents a named and typed rpc object.
 * 
 * RPC objects enable rpc client and provider to identify
 * specific RPC operations based on name and type.
 *
 * @ingroup yail_rpc
 */
template <typename Request, typename Response>
class rpc
{
public:
	using impl_type = detail::rpc_impl<Request, Response>;

	/**
	 * @brief Constructs rpc object.
	 *
	 * @param[in] name The name of the rpc.
	 *
	 */
	explicit rpc (const std::string& name);

	/**
	 * @brief Destroys this object.
	 */
	~rpc ();

private:
	template <typename Transport>
	friend class client;

	template <typename Transport>
	friend class provider;
	
	const impl_type& get_impl () const { return *m_impl; }

	std::unique_ptr<impl_type> m_impl;
};

} // namespace rpc
} // namespace yail

#include <yail/rpc/impl/rpc.h>

#endif // YAIL_RPC_H
