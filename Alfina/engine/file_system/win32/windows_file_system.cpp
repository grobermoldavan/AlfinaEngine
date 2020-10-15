#include "windows_file_system.h"

#include "utilities/defer.h"

namespace al::engine
{
    Win32FileSystem::Win32FileSystem(StackAllocator* allocator)
        : FileSystem{ allocator }
    { }

	void Win32FileSystem::read_file(const char* fileName, FileHandle* handle)
	{
		HANDLE hFile = ::CreateFileA(	fileName,				// file to open
									    GENERIC_READ,			// open for reading
									    FILE_SHARE_READ,		// share for reading
									    NULL,					// default security
									    OPEN_EXISTING,			// existing file only
									    FILE_ATTRIBUTE_NORMAL,	// normal file
									    NULL);					// no attr. template
		if (hFile == INVALID_HANDLE_VALUE)
		{
			// Unable to open file
			return;
		}

		defer({ ::CloseHandle(hFile); });

		DWORD fileSize;
		fileSize = ::GetFileSize(hFile, &fileSize);
		if (fileSize == INVALID_FILE_SIZE)
		{
			// Unable to get file size
			return;
		}

		handle->data = allocator->allocate(fileSize + 1);
		if (!handle->data)
		{
			// Unable to allocate memory
			return;
		}

		DWORD bytesRead;
		BOOL isFileRead = ::ReadFile(hFile, handle->data, fileSize, &bytesRead, NULL);
		if (!isFileRead)
		{
			// Unable to read file
			return;
		}

		handle->data[fileSize] = 0;
		handle->fileSize = fileSize;
	}
}
