#if defined(AL_UNITY_BUILD)

#else
#	include "windows_file_sys.h"
#endif

#include <cstdint>

#include "engine/engine_utilities/logging/logging.h"

#include "windows_utilities.h"

namespace al::engine
{
	ErrorInfo FileSys::read_file(const char* fileName, FileHandle* handle)
	{
		AL_ASSERT_MSG(handle, "Can't read file to null FileHandle")

		HANDLE hFile = CreateFile(	fileName,				// file to open
									GENERIC_READ,			// open for reading
									FILE_SHARE_READ,		// share for reading
									NULL,					// default security
									OPEN_EXISTING,			// existing file only
									FILE_ATTRIBUTE_NORMAL,	// normal file
									NULL);					// no attr. template

		if (hFile == INVALID_HANDLE_VALUE)
		{
			// Maybe change this to a warning?
			AL_LOG_NO_DISCARD(Logger::Type::ERROR_MSG, "Unable to open file for read. File name is \"", fileName, "\"");
			return{ al::engine::ErrorInfo::Code::INCORRECT_INPUT_DATA };
		}

		DWORD fileSize;
		fileSize = ::GetFileSize(hFile, &fileSize);
		AL_ASSERT_MSG_NO_DISCARD(fileSize != INVALID_FILE_SIZE, "Unable to get the file size. File name is \"", fileName, "\"")

		handle->data = new uint8_t[fileSize + 1];

		DWORD bytesRead;
		BOOL isFileRead = ::ReadFile(hFile, handle->data, fileSize, &bytesRead, NULL);
		AL_ASSERT_MSG_NO_DISCARD(isFileRead != FALSE, "Unable to read file. File name is \"", fileName, "\" ", get_last_error_str())

		BOOL isHandleClosed;
		isHandleClosed = ::CloseHandle(hFile);
		AL_ASSERT_MSG_NO_DISCARD(isHandleClosed, "Unable to close file. File name is \"", fileName, "\"")

		handle->data[fileSize] = 0;
		handle->fileSize = fileSize;

		return{ al::engine::ErrorInfo::Code::ALL_FINE };
	}

	ErrorInfo FileSys::free_file_handle(FileHandle* handle)
	{
		AL_LOG(Logger::Type::MESSAGE, "Closing file handle");

		if (handle->data)
		{
			delete handle->data;
			handle->data = nullptr;
		}

		return{ al::engine::ErrorInfo::Code::ALL_FINE };
	}
}
