#ifndef __ALFINA_ALLOCATOR_BASE_H__
#define __ALFINA_ALLOCATOR_BASE_H__

namespace al::engine
{
	using memory_t = void*;

	namespace allocator_sfinae
	{
		// dont know if i really need this

		template<typename T>
		struct HasAllocate
		{
			template<typename U, memory_t(U::*)(size_t, size_t)> struct SFINAE {};
			template<typename U> static char Test(SFINAE<U, &U::allocate>*);
			template<typename U> static int Test(...);
			static const bool value = sizeof(Test<T>(0)) == sizeof(char);
		};

		template<typename T>
		struct HasDeallocate
		{
			template<typename U, void(U::*)(memory_t)> struct SFINAE {};
			template<typename U> static char Test(SFINAE<U, &U::deallocate>*);
			template<typename U> static int Test(...);
			static const bool value = sizeof(Test<T>(0)) == sizeof(char);
		};

		template<typename T>
		struct IsAllocatorType
		{
			static const bool value = (HasAllocate<T>::value && HasDeallocate<T>::value);
		};
	}

	class AllocatorBase
	{
	public:
		virtual memory_t	allocate	(size_t sizeBytes, size_t alignment, const char* file, int line, const char* dbgUid)	= 0;
		virtual void		deallocate	(memory_t ptr, const char* file, int line, const char* dbgUid)							= 0;

		template<typename T, typename... Args>
		T* construct(const char* file, int line, const char* dbgUid, Args ... args)
		{
			T* memory = static_cast<T*>(allocate(sizeof(T), 0, file, line, dbgUid));
			if (memory) new(memory) T(args...);
			return memory;
		}

		template<typename T>
		void destruct(const char* file, int line, const char* dbgUid, T* val)
		{
			if (!val) return;
			val->~T();
			deallocate(val, file, line, dbgUid);
		}
	};
}

#endif
