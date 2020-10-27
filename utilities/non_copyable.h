#ifndef AL_NON_COPYABLE_H
#define AL_NON_COPYABLE_H

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