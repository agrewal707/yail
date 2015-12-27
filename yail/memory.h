#ifndef YAIL_MEMORY_H
#define YAIL_MEMORY_H

#include <memory>

namespace yail {

template <typename T, typename ...Args>
std::unique_ptr<T> make_unique(Args &&... args)
{
	return std::unique_ptr<T> (new T(std::forward<Args> (args)...));
}

} // namespace

#endif // YAIL_MEMORY_H
