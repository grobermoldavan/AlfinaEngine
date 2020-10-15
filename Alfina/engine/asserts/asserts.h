#ifndef __ALFINA_ASSERTS_H__
#define __ALFINA_ASSERTS_H__

#include <cassert>

// @TODO : build normal assertion system. Currently this is just a wrapper over cassert
#define al_assert(condition) assert(condition)

#endif
