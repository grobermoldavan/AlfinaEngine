#ifndef AL_UTILITIES_H
#define AL_UTILITIES_H

#include <type_traits>  // for std::false_type, std::true_type

#ifndef array_size
#   define array_size(array) (sizeof(array) / sizeof(array[0]))
#endif

#define AL_HAS_CHECK_DECLARATION(memberName)                                                \
    template <typename T, typename = int>                                                   \
    struct __has##memberName : std::false_type { };                                         \
    template <typename T>                                                                   \
    struct __has##memberName <T, decltype((void) T::memberName, 0)> : std::true_type { };

#define AL_HAS_CHECK(bindings, memberName) __has##memberName<bindings>::value

#include "function.h"
#include "defer.h"
#include "containers.h"
#include "hash_map.h"
#include "tuple.h"
#include "thread_local_storage.h"
#include "allocations_tracker.h"
#include "bits.h"

#endif
