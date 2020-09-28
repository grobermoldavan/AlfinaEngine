#ifndef __ALFINA_BASE_SOUND_SYSTEM_H__
#define __ALFINA_BASE_SOUND_SYSTEM_H__

#include <cstdint>

#include "sound_parameters.h"
#include "audio_formats/audio_formats.h"

namespace al::engine
{
	using SoundId = size_t;

	class SoundSystem
	{
	public:
		enum class SourceType
		{
			WAV,
			__end
		};

		virtual ~SoundSystem() = default;

		virtual void			init				(const SoundParameters& parameters)			= 0;
		virtual SoundParameters	get_valid_parameters()									const	= 0;

		virtual SoundId			load_sound			(SourceType type, const char* path)			= 0;
		virtual void			play_sound			(SoundId id)						const	= 0;

		virtual void			dbg_play_single_wav (WavFile* file)								= 0;
	};

	class ApplicationWindow;

	extern ErrorInfo create_sound_system	(SoundSystem**, ApplicationWindow*);
	extern ErrorInfo destroy_sound_system	(SoundSystem*);
}

#endif
