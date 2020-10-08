#include "windows_file_system.h"

#include "utilities/defer.h"

namespace al::engine
{
	ErrorInfo create_file_system(FileSystem** fileSystem, std::function<uint8_t*(size_t sizeBytes)> allocate)
	{
		if (!fileSystem)
		{
			return { ErrorInfo::Code::INCORRECT_INPUT_DATA, ErrorInfo::ERROR_MESSAGES[ErrorInfo::ErrorMessageCode::NULL_PTR_PROVIDED] };
		}

		// Allocate memory and construct object
		FileSystem* win32FileSystem = reinterpret_cast<Win32FileSystem*>(allocate(sizeof(Win32FileSystem)));
		if (!win32FileSystem)
		{
			return { ErrorInfo::Code::ALLOCATION_ERROR, ErrorInfo::ERROR_MESSAGES[ErrorInfo::ErrorMessageCode::UNABLE_TO_ALLOCATE_MEMORY] };
		}

		win32FileSystem = new(win32FileSystem) Win32FileSystem();
		*fileSystem = win32FileSystem;

		return { ErrorInfo::Code::ALL_FINE, ErrorInfo::ERROR_MESSAGES[ErrorInfo::ErrorMessageCode::ALL_FINE] };
	}

	ErrorInfo destroy_file_system(FileSystem* fileSystem, std::function<void(uint8_t* ptr)> deallocate)
	{
		if (!fileSystem)
		{
			return { ErrorInfo::Code::INCORRECT_INPUT_DATA, ErrorInfo::ERROR_MESSAGES[ErrorInfo::ErrorMessageCode::NULL_PTR_PROVIDED] };
		}

		Win32FileSystem* win32fileSystem = static_cast<Win32FileSystem*>(fileSystem);

		// Destruct and deallocate window
		win32fileSystem->~Win32FileSystem();
		deallocate(reinterpret_cast<uint8_t*>(win32fileSystem));

		return { ErrorInfo::Code::ALL_FINE, ErrorInfo::ERROR_MESSAGES[ErrorInfo::ErrorMessageCode::ALL_FINE] };
	}

	void Win32FileSystem::read_file(const char* fileName, FileHandle* handle, std::function<uint8_t* (size_t sizeBytes)> allocate)
	{
		HANDLE hFile = CreateFile(	fileName,				// file to open
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

		handle->data = allocate(fileSize + 1);
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
