#ifndef AL_UTILITIES_H
#define AL_UTILITIES_H

#ifndef array_size
#   define array_size(array) (sizeof(array) / sizeof(array[0]))
#endif

#include "function.h"
#include "defer.h"
#include "array_view.h"
#include "tuple.h"
#include "fixed_size_buffer.h"
#include "thread_local_storage.h"

#endif
