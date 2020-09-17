#ifndef __ALFINA_DEFAULT_ALLOCATOR_H__
#define __ALFINA_DEFAULT_ALLOCATOR_H__

#include "../allocator_base.h"

#include "engine/engine_utilities/logging/logging.h"

#define AL_DEFAULT_ALLOC(bytes, alignment, uid)	al::engine::default_allocator.allocate(bytes, alignment, __FILE__, __LINE__, uid)
#define AL_DEFAULT_DEALLOC(ptr, uid)			al::engine::default_allocator.deallocate(ptr, __FILE__, __LINE__, uid)

#define AL_DEFAULT_CONSTRUCT(type, uid, ...)	al::engine::default_allocator.construct<type>(__FILE__, __LINE__, uid, __VA_ARGS__)
#define AL_DEFAULT_DESTRUCT(ptr, uid)			al::engine::default_allocator.destruct(__FILE__, __LINE__, uid, ptr)

namespace al::engine
{
	class DefaultHeapAllocator : public AllocatorBase
	{
	public:
		virtual memory_t allocate(size_t sizeBytes, size_t alignment, const char* file, int line, const char* dbgUid) override
		{
			dbgCount++;
			AL_LOG(al::engine::Logger::MESSAGE, dbgUid, " :: Allocating at file ", file, " line ", line, ". Count is ", dbgCount)
			return std::malloc(sizeBytes);
		}

		virtual void deallocate(memory_t ptr, const char* file, int line, const char* dbgUid) override
		{ 
			dbgCount--;
			AL_LOG(al::engine::Logger::MESSAGE, dbgUid, " :: Deallocating at file ", file, " line ", line, ". Count is ", dbgCount)
			std::free(ptr);
		}

		static size_t dbgCount;
	};

	size_t DefaultHeapAllocator::dbgCount = 0;

	DefaultHeapAllocator default_allocator;
}

#endif
