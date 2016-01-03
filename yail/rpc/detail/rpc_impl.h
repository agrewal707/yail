#ifndef YAIL_RPC_DETAIL_RPC_IMPL_H
#define YAIL_RPC_DETAIL_RPC_IMPL_H

#include <yail/rpc/rpc.h>
#include <yail/rpc/rpc_traits.h>

//
// yail::detail::rpc_impl
//
namespace yail {
namespace rpc {
namespace detail {

template <typename Request, typename Response>
class rpc_impl
{
public:
	explicit rpc_impl (const std::string& name);

	~rpc_impl ();

	std::string get_name () const
	{
		return m_name;
	}

	std::string get_type_name () const
	{
		return yail::rpc::rpc_type_support<Request, Response>::get_name ();
	}

	bool serialize (const Request &req, std::string *out) const
	{
		return yail::rpc::rpc_type_support<Request, Response>::serialize (req, out);
	}

	bool serialize (const Response &res, std::string *out) const
	{
		return yail::rpc::rpc_type_support<Request, Response>::serialize (res, out);
	}

	bool deserialize (Request &req, const std::string &in) const
	{
		return yail::rpc::rpc_type_support<Request, Response>::deserialize (req, in);
	}

	bool deserialize (Response &res, const std::string &in) const
	{
		return yail::rpc::rpc_type_support<Request, Response>::deserialize (res, in);
	}

private:
	std::string m_name;
};

} // namespace detail
} // namespace rpc
} // namespace yail

namespace yail {
namespace rpc {
namespace detail {

template <typename Request, typename Response>
rpc_impl<Request, Response>::rpc_impl (const std::string& name) :
	m_name {name}
{}

template <typename Request, typename Response>
rpc_impl<Request, ResponseT>::~rpc_impl ()
{}

} // namespace detail
} // namespace rpc
} // namespace yail

#endif // YAIL_RPC_DETAIL_RPC_IMPL_H
