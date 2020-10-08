#ifndef __ALFINA_WINDOWS_FILE_SYS_H__
#define __ALFINA_WINDOWS_FILE_SYS_H__

#include <windows.h>

#include "../base_file_system.h"

namespace al::engine
{
	class Win32FileSystem : public FileSystem
	{
	public:
		virtual void read_file(const char* fileName, FileHandle* handle, std::function<uint8_t*(size_t sizeBytes)> allocate) override;
	};
}

#include "windows_file_system.cpp"

#endif
