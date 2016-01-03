#ifndef YAIL_RPC_TRAITS_H
#define YAIL_RPC_TRAITS_H

#include <string>

namespace yail {
namespace rpc {

template <typename Request, typename Response>
class rpc_type_support {};

} // namespace rpc
} // namespace yail

#define REGISTER_RPC_TRAITS(Request, Response)                              \
namespace yail {                                                            \
namespace rpc {                                                             \
	template <> class rpc_type_support<Request, Response>                     \
	{                                                                         \
	public:                                                                   \
		static std::string get_name ()                                          \
		{                                                                       \
			return std::string(#Request) + std::string(#Response);                \
		}                                                                       \
		                                                                        \
		static bool serialize (const Request &req, std::string *data)           \
		{                                                                       \
			return req.SerializeToString (data);                                  \
		}                                                                       \
		                                                                        \
		static bool serialize (const Response &res, std::string *data)          \
		{                                                                       \
			return res.SerializeToString (data);                                  \
		}                                                                       \
		                                                                        \
		static bool deserialize (Request &req, const std::string &data)         \
		{                                                                       \
			return req.ParseFromString (data);                                    \
		}                                                                       \
		                                                                        \
		static bool deserialize (Response &res, const std::string &data)        \
		{                                                                       \
			return res.ParseFromString (data);                                    \
		}                                                                       \
	};                                                                        \
}																																		        \
}

#endif // YAIL_RPC_TRAITS_H
