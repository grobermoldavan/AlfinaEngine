#ifndef __ALFINA_NON_COPYABLE_H__
#define __ALFINA_NON_COPYABLE_H__

namespace al
{
	class NonCopyable
	{
	protected:
		NonCopyable() = default;
		~NonCopyable() = default;

		NonCopyable(const NonCopyable&) = delete;
		NonCopyable& operator = (const NonCopyable&) = delete;
	};
}

#endif