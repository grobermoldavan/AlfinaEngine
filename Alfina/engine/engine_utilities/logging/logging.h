#ifndef __ALFINA_LOGGING_H__
#define __ALFINA_LOGGING_H__

#include <cstdint>
#include <iostream>
#include <fstream>

#include "utilities/flags.h"

#define AL_LOG_OVERRIDE_OUTPUT(file)			al::engine::Logger::override_output(file);
#define AL_LOG_CLEAR_OUTPUT_OVERRIDE()			al::engine::Logger::clear_output_override();

#define AL_LOG_NO_DISCARD(type, ...)			al::engine::Logger::log(type, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__);
#define AL_LOG_SHORT_NO_DISCARD(type, ...)		al::engine::Logger::log(type, nullptr, nullptr, 0, __VA_ARGS__);

#define AL_LOG_DELIMITER						" : "
#define AL_LOG_LINE_END							std::endl

#if defined(AL_DEBUG)
#	define AL_LOG(type, ...)					al::engine::Logger::log(type, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__);
#	define AL_LOG_SHORT(type, ...)				al::engine::Logger::log(type, nullptr, nullptr, 0, __VA_ARGS__);
#else
#	define AL_LOG(type, ...)
#	define AL_LOG_SHORT(type, ...)
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

		template<typename... Args>
		static void log(Type type, const char* file, const char* func, const int line, Args... args)
		{
			#define OUT_MSG_INFO(file, func, line)	if (file)	std::cout << file << AL_LOG_DELIMITER;								\
													if (func)	std::cout << func << AL_LOG_DELIMITER << line << AL_LOG_DELIMITER;

			#define ASS_HACK_EXPAND_ARGS_PRINT		using expander = int[];															\
													(void)expander{ 0, (void(std::cout << std::forward<Args>(args)), 0)... };

			switch (type)
			{
				case Type::MESSAGE:
				{
					prepare_log_message();
					OUT_MSG_INFO(file, func, line)
					ASS_HACK_EXPAND_ARGS_PRINT
					std::cout << std::endl;
					break;
				}
				case Type::WARNING:
				{
					prepare_log_warning();
					OUT_MSG_INFO(file, func, line)
					ASS_HACK_EXPAND_ARGS_PRINT
					std::cout << std::endl;
					break;
				}
				case Type::ERROR_MSG:
				{
					prepare_log_error();
					OUT_MSG_INFO(file, func, line)
					ASS_HACK_EXPAND_ARGS_PRINT
					std::cout << std::endl;
					std::quick_exit(0);
					break;
				}
			}

			#undef OUT_MSG_INFO
			#undef ASS_HACK_EXPAND_ARGS_PRINT
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

		static void al::engine::Logger::init()
		{
			std::at_quick_exit(terminate);
			std::atexit(terminate);
		}

		// must be implemented in platform side
		static void terminate			();
		static void prepare_log_message	();
		static void prepare_log_warning	();
		static void prepare_log_error	();
	};
}

al::Flags32			al::engine::Logger::stateFlags;
std::ofstream		al::engine::Logger::outStream;
std::streambuf*		al::engine::Logger::stdCoutBuf = nullptr;

#endif