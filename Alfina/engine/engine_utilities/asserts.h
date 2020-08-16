#ifndef __ALFINA_ASSERTS_H__
#define __ALFINA_ASSERTS_H__

#include "logging\logging.h"

#define AL_ASSERT_MSG_NO_DISCARD(condition, msg)	if (!(condition))	AL_LOG_NO_DISCARD(al::engine::Logger::Type::ERROR_MSG, msg)

#if defined(AL_DEBUG)
#	define AL_ASSERT_MSG(condition, msg)	if (!(condition))	AL_LOG(al::engine::Logger::Type::ERROR_MSG, msg)
#	define AL_ASSERT(condition)				if (!(condition))	AL_LOG(al::engine::Logger::Type::ERROR_MSG, "Assertion failed")
#	define AL_NO_IMPLEMENTATION_ASSERT							AL_LOG(al::engine::Logger::Type::ERROR_MSG, "No method implementation provided")
#else
#	define AL_ASSERT_MSG(condition, msg)
#	define AL_ASSERT(condition)
#	define AL_NO_IMPLEMENTATION_ASSERT
#endif

#endif