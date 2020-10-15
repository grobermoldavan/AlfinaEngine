#ifndef __ALFINA_BASE_FILE_SYS_H__
#define __ALFINA_BASE_FILE_SYS_H__

#include <cstdint>
#include <functional>

#include "engine/memory/stack_allocator.h"
#include "engine/engine_utilities/error_info.h"
#include "utilities/non_copyable.h"

namespace al::engine
{
	class FileHandle;

	class FileSystem
	{
	public:
        FileSystem(StackAllocator* allocator);
        virtual ~FileSystem() = default;

		virtual void read_file(const char* fileName, FileHandle* handle) = 0;

    protected:
        StackAllocator* allocator;
	};

    FileSystem::FileSystem(StackAllocator* allocator)
        : allocator{ allocator }
    { }

	class FileHandle : NonCopyable
	{
	public:
		FileHandle()
			: data{ nullptr }
			, fileSize{ 0 }
		{ }

		~FileHandle() = default;

		const	uint8_t*	get_data() const	{ return data; }
				uint8_t*	get_data()			{ return data; }
		const	size_t		get_size() const	{ return fileSize; }

	public:
		uint8_t*	data;
		size_t		fileSize;

		friend FileSystem;
	};
}

#endif