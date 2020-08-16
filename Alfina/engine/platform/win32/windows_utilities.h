#ifndef __ALFINA_WINDOWS_UTILITIES_H__
#define __ALFINA_WINDOWS_UTILITIES_H__

#include <windows.h>

#include <string>

namespace al::engine
{
	std::string get_last_error_str()
	{
		DWORD error = ::GetLastError();
		if (error == 0)
			return {};

		LPSTR messageBuffer = nullptr;
		size_t size = FormatMessageA(	FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
										NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

		std::string message(messageBuffer, size);
		LocalFree(messageBuffer);
		return message;
	}
}


#endif