#ifndef AL_UTILITIES_H
#define AL_UTILITIES_H

#ifndef array_size
#   define array_size(array) (sizeof(array) / sizeof(array[0]))
#endif

#include "engine/utilities/function.h"
#include "engine/utilities/defer.h"
#include "engine/utilities/array_view.h"
#include "engine/utilities/tuple.h"

#endif
