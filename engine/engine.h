#ifndef AL_ENGINE_H
#define AL_ENGINE_H

#include "engine/math/math.h"
#include "engine/memory/memory.h"
#include "engine/platform/platform.h"
#include "engine/render/renderer.h"
#include "engine/utilities/utilities.h"
#include "engine/application.h"
#include "engine/config.h"
#include "engine/types.h"

#ifdef AL_IMPLEMENTATION
#   include "engine/memory/memory.cpp"
#   include "engine/platform/platform.cpp"
#   include "engine/render/renderer.cpp"
#   include "engine/application.cpp"
#endif

#endif
