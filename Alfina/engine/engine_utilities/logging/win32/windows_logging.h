#ifndef __ALFINA_WIN32_LOGGING_H__
#define __ALFINA_WIN32_LOGGING_H__

#include <Windows.h>

#include <iostream>
#include <cstdlib>

#include "../logging.h"

#define INIT_CHECK if(!stateFlags.get_flag(StateFlags::IS_INITIALIZED)) { init(); stateFlags.set_flag(StateFlags::IS_INITIALIZED); }

void al::engine::Logger::terminate()
{
	HANDLE outHandle = ::GetStdHandle(STD_OUTPUT_HANDLE);
	::SetConsoleTextAttribute(outHandle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

	if (stateFlags.get_flag(StateFlags::IS_REDIRECTED)) clear_output_override();
}

void al::engine::Logger::prepare_log_message()
{
	INIT_CHECK
	HANDLE outHandle = ::GetStdHandle(STD_OUTPUT_HANDLE);
	::SetConsoleTextAttribute(outHandle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
}

void al::engine::Logger::prepare_log_warning()
{
	INIT_CHECK
	HANDLE outHandle = ::GetStdHandle(STD_OUTPUT_HANDLE);
	::SetConsoleTextAttribute(outHandle, FOREGROUND_RED | FOREGROUND_GREEN);
}

void al::engine::Logger::prepare_log_error()
{
	INIT_CHECK
	HANDLE outHandle = ::GetStdHandle(STD_OUTPUT_HANDLE);
	::SetConsoleTextAttribute(outHandle, FOREGROUND_RED);
}

#undef INIT_CHECK
#undef OUT_MSG

#endif // __ALFINA_WIN32_LOGGING_H__