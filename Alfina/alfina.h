/*
	In order to use this libraries, user should provide additional include path to the directory, where alfina.h is stored
	* Plus additional include path for GL/glew.h, as well as lib path for glew32s.lib, if build for windows
*/

#include "engine/engine.h"

#include "utilities/constexpr_functions.h"
#include "utilities/defer.h"
#include "utilities/flags.h"
#include "utilities/dispensable.h"
#include "utilities/smooth_average.h"