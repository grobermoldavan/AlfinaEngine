#ifndef __ALFINA_BASE_FILE_SYS_H__
#define __ALFINA_BASE_FILE_SYS_H__

#include <cstdint>

#include "engine/engine_utilities/error_info.h"
#include "engine/engine_utilities/asserts.h"

#include "utilities/non_copyable.h"

namespace al::engine
{
	class FileHandle;

	class FileSys
	{
	public:
		static ErrorInfo read_file(const char* fileName, FileHandle* handle);
		static ErrorInfo free_file_handle(FileHandle* handle);
	};

	class FileHandle : NonCopyable
	{
	public:
		FileHandle()
			: data{ nullptr }
		{ }

		~FileHandle()
		{
			FileSys::free_file_handle(this);
		}

		const	uint8_t*	get_data() const	{ AL_ASSERT(data); return data; }
				uint8_t*	get_data()			{ AL_ASSERT(data); return data; }
		const	size_t		get_size() const	{ AL_ASSERT(data); return fileSize; }

	private:
		uint8_t*	data;
		size_t		fileSize;

		friend FileSys;
	};

	
}

#endif