#ifndef YAIL_RPC_DETAIL_TCTX_IMPL_H
#define YAIL_RPC_DETAIL_TCTX_IMPL_H

#include <yail/rpc/tctx.h>

#include <functional>

//
// yail::detail::context_impl
//
namespace yail {
namespace rpc {
namespace detail {

template <typename Transport>
class server;

class tctx_impl
{
public:
	enum status
	{
		OK,
		DELAYED,
		ERROR
	};

	tctx_impl (void *tctx):
		m_tctx (tctx)
	{}

	~tctx_impl ()
	{}

private:
	template <typename Transport>
	friend class server;

	void *m_trctx;
	uint32_t m_reqid;
	std::string m_res_data;
	status m_status;
};

} // namespace detail
} // namespace rpc
} // namespace yail

#endif // YAIL_RPC_DETAIL_CONTEXT_IMPL_H
