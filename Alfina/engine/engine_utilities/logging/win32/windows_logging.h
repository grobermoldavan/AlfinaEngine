#ifndef __ALFINA_WIN32_LOGGING_H__
#define __ALFINA_WIN32_LOGGING_H__

#include <Windows.h>

#include <iostream>
#include <cstdlib>

#include "../logging.h"

#define INIT_CHECK if(!stateFlags.get_flag(StateFlags::IS_INITIALIZED)) { init(); stateFlags.set_flag(StateFlags::IS_INITIALIZED); }

#define OUT_MSG(file, func, line, msg)	if (file)	std::cout << file << AL_LOG_DELIMITER;								\
										if (func)	std::cout << func << AL_LOG_DELIMITER << line << AL_LOG_DELIMITER;	\
													std::cout << msg << AL_LOG_LINE_END;

void al::engine::Logger::init()
{
	std::at_quick_exit(terminate);
	std::atexit(terminate);
}

void al::engine::Logger::terminate()
{
	HANDLE outHandle = ::GetStdHandle(STD_OUTPUT_HANDLE);
	::SetConsoleTextAttribute(outHandle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

	if (stateFlags.get_flag(StateFlags::IS_REDIRECTED)) clear_output_override();
}

void al::engine::Logger::log_message(const char* file, const char* func, const int line, const char* msg)
{
	INIT_CHECK

	HANDLE outHandle = ::GetStdHandle(STD_OUTPUT_HANDLE);
	::SetConsoleTextAttribute(outHandle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	OUT_MSG(file, func, line, msg)
}

void al::engine::Logger::log_warning(const char* file, const char* func, const int line, const char* msg)
{
	INIT_CHECK
	
	HANDLE outHandle = ::GetStdHandle(STD_OUTPUT_HANDLE);
	::SetConsoleTextAttribute(outHandle, FOREGROUND_RED | FOREGROUND_GREEN);
	std::cout << "Warning" << AL_LOG_DELIMITER;
	OUT_MSG(file, func, line, msg)
}

void al::engine::Logger::log_error(const char* file, const char* func, const int line, const char* msg)
{
	INIT_CHECK

	HANDLE outHandle = ::GetStdHandle(STD_OUTPUT_HANDLE);
	::SetConsoleTextAttribute(outHandle, FOREGROUND_RED);
	std::cout << "Error" << AL_LOG_DELIMITER;
	OUT_MSG(file, func, line, msg)
	std::quick_exit(0);
}

#undef INIT_CHECK
#undef OUT_MSG

#endif // __ALFINA_WIN32_LOGGING_H__