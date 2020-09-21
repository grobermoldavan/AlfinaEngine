#ifndef __ALFINA_BASE_SOUND_SYSTEM_H__
#define __ALFINA_BASE_SOUND_SYSTEM_H__

#include <cstdint>

#include "engine/engine_utilities/error_info.h"

#include "utilities/dispensable.h"

namespace al::engine
{
	using SoundId = size_t;

	struct SoundParameters
	{
		enum class Channels
		{
			MONO,
			STEREO,
			__end
		};

		enum class BytesPerSample
		{
			ONE,
			TWO,
			FOUR,
			__end
		};

		Dispensable<uint32_t>		sampleRate;
		Dispensable<Channels>		channels;
		Dispensable<BytesPerSample>	bytesPerSample;
	};

	class SoundSystem
	{
	public:
		enum class SourceType
		{
			WAV,
			__end
		};

		virtual ~SoundSystem() = default;

		virtual void	init		(const SoundParameters& parameters) = 0;

		virtual SoundId load_sound	(SourceType type, const char* path)	= 0;
		virtual void	play_sound	(SoundId id)						= 0;
	};

	class ApplicationWindow;

	extern ErrorInfo create_sound_system	(SoundSystem**, ApplicationWindow*);
	extern ErrorInfo destroy_sound_system	(SoundSystem*);
}

#endif
