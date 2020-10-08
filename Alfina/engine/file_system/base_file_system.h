#ifndef __ALFINA_BASE_FILE_SYS_H__
#define __ALFINA_BASE_FILE_SYS_H__

#include <cstdint>
#include <functional>

#include "engine/engine_utilities/error_info.h"
#include "utilities/non_copyable.h"

namespace al::engine
{
	class FileHandle;

	class FileSystem
	{
	public:
		virtual void read_file(const char* fileName, FileHandle* handle, std::function<uint8_t*(size_t sizeBytes)> allocate) = 0;
	};

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

	ErrorInfo	create_file_system	(FileSystem** fileSystem, std::function<uint8_t* (size_t sizeBytes)> allocate);
	ErrorInfo	destroy_file_system	(FileSystem* fileSystem, std::function<void(uint8_t* ptr)> deallocate);
}

#endif