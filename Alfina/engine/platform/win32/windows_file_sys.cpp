#if defined(AL_UNITY_BUILD)

#else
#	include "windows_file_sys.h"
#endif

#include <cstdint>

#include "engine/engine_utilities/logging/logging.h"
#include "engine/allocation/allocation.h"

#include "windows_utilities.h"
#include "utilities/defer.h"

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
			AL_LOG_NO_DISCARD(Logger::Type::WARNING, "Unable to open file for read. File name is \"", fileName, "\"");
			return{ al::engine::ErrorInfo::Code::INCORRECT_INPUT_DATA };
		}

		defer({
			BOOL isHandleClosed = ::CloseHandle(hFile);
			AL_ASSERT_MSG_NO_DISCARD(isHandleClosed, "Unable to close file. File name is \"", fileName, "\"")
		});

		DWORD fileSize;
		fileSize = ::GetFileSize(hFile, &fileSize);
		AL_ASSERT_MSG_NO_DISCARD(fileSize != INVALID_FILE_SIZE, "Unable to get the file size. File name is \"", fileName, "\"")

		handle->data = static_cast<uint8_t*>(AL_DEFAULT_ALLOC((fileSize + 1) * sizeof(uint8_t), 0, "FILE_READING"));
		if (!handle->data)
		{
			return{ ErrorInfo::Code::BAD_ALLOC };
		}

		DWORD bytesRead;
		BOOL isFileRead = ::ReadFile(hFile, handle->data, fileSize, &bytesRead, NULL);
		AL_ASSERT_MSG_NO_DISCARD(isFileRead != FALSE, "Unable to read file. File name is \"", fileName, "\" ", get_last_error_str())

		handle->data[fileSize] = 0;
		handle->fileSize = fileSize;

		return{ al::engine::ErrorInfo::Code::ALL_FINE };
	}

	ErrorInfo FileSys::free_file_handle(FileHandle* handle)
	{
		if (handle->data)
		{
			AL_DEFAULT_DEALLOC(handle->data, "FILE_READING");
			handle->data = nullptr;
		}

		return{ al::engine::ErrorInfo::Code::ALL_FINE };
	}
}
