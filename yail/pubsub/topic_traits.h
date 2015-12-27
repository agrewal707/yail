#ifndef YAIL_TRAITS_H
#define YAIL_TRAITS_H

#include <string>

namespace yail {
namespace pubsub {

template <typename T>
class topic_type_support {};

} // namespace pubsub
} // namespace yail

#define REGISTER_TOPIC_TRAITS(T)                                    \
namespace yail {                                                    \
namespace pubsub {                                                  \
	template <> class topic_type_support<T>                           \
	{                                                                 \
	public:                                                           \
		static std::string get_name ()                                  \
		{                                                               \
			return #T;                                                    \
		}                                                               \
		                                                                \
		static bool serialize (const T &t, std::string *data)           \
		{                                                               \
			return t.SerializeToString (data);                            \
		}                                                               \
		                                                                \
		static bool deserialize (T &t, const std::string &data)         \
		{                                                               \
			return t.ParseFromString (data);                              \
		}                                                               \
	};                                                                \
}																																		\
}

#endif // YAIL_TRAITS_H
