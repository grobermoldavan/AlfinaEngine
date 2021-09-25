#ifndef AL_ENGINE_H
#define AL_ENGINE_H

#include "engine/types.h"
#include "engine/config.h"
#include "engine/math/math.h"
#include "engine/assert/assert.h"
#include "engine/result/result.h"
#include "engine/memory/memory.h"
#include "engine/utilities/utilities.h"
#include "engine/logger/logger.h"
#include "engine/platform/platform.h"
#include "engine/render/renderer.h"
#include "engine/thread_local_globals/thread_local_globals.h"
#include "engine/application.h"

#ifdef AL_IMPLEMENTATION
#   include "engine/assert/assert.cpp"
#   include "engine/result/result.cpp"
#   include "engine/memory/memory.cpp"
#   include "engine/logger/logger.cpp"
#   include "engine/platform/platform.cpp"
#   include "engine/render/renderer.cpp"
#   include "engine/thread_local_globals/thread_local_globals.cpp"
#   include "engine/application.cpp"
#endif

#endif
