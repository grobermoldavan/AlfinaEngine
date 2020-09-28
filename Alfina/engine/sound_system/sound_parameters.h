#ifndef __ALFINA_SOUND_PARAMETERS_H__
#define __ALFINA_SOUND_PARAMETERS_H__

namespace al::engine
{
	struct SoundParameters
	{
		size_t sampleRate		= 44100;
		size_t channels			= 2;
		size_t bitsPerSample	= 32;
	};
}

#endif
