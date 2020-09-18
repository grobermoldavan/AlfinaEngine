#ifndef __ALFINA_BASE_SOUND_SYSTEM_H__
#define __ALFINA_BASE_SOUND_SYSTEM_H__

#include <cstdint>

#include "engine/engine_utilities/error_info.h"

#include "utilities/dispensable.h"

namespace al::engine
{
	using SoundId = uint32_t;

	struct SoundParameters
	{
		enum Channels
		{
			MONO,
			STEREO,
			__end
		};

		Dispensable<uint32_t>	sampleRate;
		Dispensable<Channels>	channels;
		Dispensable<uint32_t>	bytesPerSample;
	};

	class SoundSystem
	{
	public:
		enum SourceType
		{
			WAV,
			__end
		};

		virtual void	init		(const SoundParameters& parameters) = 0;

		virtual SoundId load_sound	(SourceType type, const char* path)	= 0;
		virtual void	play_sound	(SoundId id)						= 0;
	};

	class ApplicationWindow;

	extern ErrorInfo create_sound_system(SoundSystem**, ApplicationWindow*);
	extern ErrorInfo destroy_sound_system(const SoundSystem*);
}

#endif