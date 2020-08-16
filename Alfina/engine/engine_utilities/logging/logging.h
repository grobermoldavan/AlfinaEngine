#ifndef __ALFINA_LOGGING_H__
#define __ALFINA_LOGGING_H__

#include <cstdint>
#include <iostream>
#include <fstream>

#include "utilities/flags.h"

#define AL_LOG_OVERRIDE_OUTPUT(file)		al::engine::Logger::override_output(file);
#define AL_LOG_CLEAR_OUTPUT_OVERRIDE()		al::engine::Logger::clear_output_override();

#define AL_LOG_NO_DISCARD(type, msg)		al::engine::Logger::log(type, __FILE__, __FUNCTION__, __LINE__, msg);
#define AL_LOG_SHORT_NO_DISCARD(type, msg)	al::engine::Logger::log(type, nullptr, nullptr, 0, msg);

#define AL_LOG_DELIMITER					" : "
#define AL_LOG_LINE_END						std::endl

#if defined(AL_DEBUG)
#	define AL_LOG(type, msg)				al::engine::Logger::log(type, __FILE__, __FUNCTION__, __LINE__, msg);
#	define AL_LOG_SHORT(type, msg)			al::engine::Logger::log(type, nullptr, nullptr, 0, msg);
#else
#	define AL_LOG(type, msg)
#	define AL_LOG_SHORT(type, msg)
#endif

namespace al::engine
{
	class Logger
	{
	public:
		enum Type : uint8_t
		{
			MESSAGE,
			WARNING,
			ERROR_MSG
		};

		static void log(Type type, const char* file, const char* func, const int line, const char* msg)
		{
			switch (type)
			{
			case Type::MESSAGE		: log_message	(file, func, line, msg); break;
			case Type::WARNING		: log_warning	(file, func, line, msg); break;
			case Type::ERROR_MSG	: log_error		(file, func, line, msg); break;
			}
		}

		static void override_output(const char* file)
		{
			outStream.open(file);
			stdCoutBuf = std::cout.rdbuf(outStream.rdbuf());
			stateFlags.set_flag(StateFlags::IS_REDIRECTED);
		}

		static void clear_output_override()
		{
			std::cout.rdbuf(stdCoutBuf);
			outStream.close();
			stateFlags.clear_flag(StateFlags::IS_REDIRECTED);
		}

	private:
		enum StateFlags : uint16_t
		{
			IS_INITIALIZED,
			IS_REDIRECTED
		};

		static Flags32			stateFlags;
		static std::ofstream	outStream;
		static std::streambuf*	stdCoutBuf;

		static void init();
		static void terminate();

		static void log_message	(const char* file, const char* func, const int line, const char* msg);
		static void log_warning	(const char* file, const char* func, const int line, const char* msg);
		static void log_error	(const char* file, const char* func, const int line, const char* msg);
	};
}

al::Flags32			al::engine::Logger::stateFlags;
std::ofstream		al::engine::Logger::outStream;
std::streambuf*		al::engine::Logger::stdCoutBuf = nullptr;

#endif