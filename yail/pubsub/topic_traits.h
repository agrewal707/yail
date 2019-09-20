#ifndef YAIL_TRAITS_H
#define YAIL_TRAITS_H

#include <string>

namespace yail {
namespace pubsub {

template <typename T>
class topic_type_support {};

} // namespace pubsub
} // namespace yail

#define TOPIC_TRAITS_COMMON(T)                                      \
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

#define REGISTER_TOPIC_TRAITS(T)                                    \
namespace yail {                                                    \
namespace pubsub {                                                  \
	template <> class topic_type_support<T>                           \
	{                                                                 \
	public:                                                           \
		TOPIC_TRAITS_COMMON(T)                                          \
	                                                                  \
		static bool is_builtin ()                                       \
		{                                                               \
			return false;                                                 \
		}                                                             	\
	};                                                                \
} \
}

#define REGISTER_BUILTIN_TOPIC_TRAITS(T)                            \
namespace yail {                                                    \
namespace pubsub {                                                  \
	template <> class topic_type_support<T>                           \
	{                                                                 \
	public:                                                           \
		TOPIC_TRAITS_COMMON(T)                                          \
		                                                                \
		static bool is_builtin ()                                       \
		{                                                               \
			return true;                                                  \
		}                                                               \
	};                                                                \
} \
}

#endif // YAIL_TRAITS_H
