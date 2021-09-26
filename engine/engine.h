#ifndef AL_ENGINE_H
#define AL_ENGINE_H

#include "engine/types.h"
#include "engine/config.h"
#include "engine/math/math.h"
#include "engine/debug/assert.h"
#include "engine/debug/result.h"
#include "engine/debug/logger.h"
#include "engine/memory/memory.h"
#include "engine/utilities/utilities.h"
#include "engine/platform/platform.h"
#include "engine/render/renderer.h"
#include "engine/thread_local_globals/thread_local_globals.h"
#include "engine/application_subsystems.h"
#include "engine/application.h"

#ifdef AL_IMPLEMENTATION
#   include "engine/debug/assert.cpp"
#   include "engine/debug/result.cpp"
#   include "engine/debug/logger.cpp"
#   include "engine/memory/memory.cpp"
#   include "engine/platform/platform.cpp"
#   include "engine/render/renderer.cpp"
#   include "engine/thread_local_globals/thread_local_globals.cpp"
#   include "engine/application.cpp"
#endif

#endif
