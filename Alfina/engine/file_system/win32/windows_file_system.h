#ifndef __ALFINA_WINDOWS_FILE_SYS_H__
#define __ALFINA_WINDOWS_FILE_SYS_H__

#include <windows.h>

#include "../base_file_system.h"

namespace al::engine
{
	class Win32FileSystem : public FileSystem
	{
	public:
        Win32FileSystem(StackAllocator* allocator);
        ~Win32FileSystem() = default;
    
		virtual void read_file(const char* fileName, FileHandle* handle) override;
	};
}

#include "windows_file_system.cpp"

#endif
