#include <yail/rpc/transport/unix_domain.h>
#include <yail/rpc/transport/detail/unix_domain_impl.h>

#include <yail/log.h>

namespace yail {
namespace rpc {
namespace transport {

//
// unix_domain
//
unix_domain::unix_domain (yail::io_service &io_service) :
	m_impl (make_unique<detail::unix_domain_impl> (io_service))
{
	YAIL_LOG_FUNCTION (this);
}

unix_domain::~unix_domain()
{
	YAIL_LOG_FUNCTION (this);
}

void unix_domain::client_send_n_receive (const endpoint &ep,
	const yail::buffer &req_buffer, yail::buffer &res_buffer, boost::system::error_code &ec)
{
	m_impl->client_send_n_receive (ep, req_buffer, res_buffer, ec);
}

void unix_domain::server_add (const endpoint &ep)
{
	m_impl->server_add (ep);
}

void unix_domain::server_remove (const endpoint &ep)
{
	m_impl->server_remove (ep);
}

void unix_domain::server_send (void *trctx, const yail::buffer &res_buffer, boost::system::error_code &ec)
{
	m_impl->server_send (trctx, res_buffer, ec);
}

} // namespace transport
} // namespace rpc
} // namespace yail
