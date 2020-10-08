#ifndef __ALFINA_BASE_SOUND_SYSTEM_H__
#define __ALFINA_BASE_SOUND_SYSTEM_H__

#include <cstdint>

#include "sound_parameters.h"
#include "audio_formats/audio_formats.h"

namespace al::engine
{
	using SoundId		= size_t;
	using SoundSourceId = size_t;

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

		// SoundId represents actual sound data stored in sound system
		// SoundSourceId represents sound source. There could be multiple sound sources using the same SoundId
		virtual SoundId			load_sound			(SourceType type, const char* path)			= 0;
		virtual SoundSourceId	create_sound_source	(SoundId id)								= 0;
		virtual void			play_sound			(SoundSourceId id)							= 0;

	protected:
		static constexpr const size_t MAX_SOUNDS		= 64;
		static constexpr const size_t MAX_SOUND_SOURCES	= 128;
	};

	class ApplicationWindow;
	class FileSystem;

	extern ErrorInfo create_sound_system	(SoundSystem**, ApplicationWindow*, FileSystem*, std::function<uint8_t* (size_t sizeBytes)> allocate);
	extern ErrorInfo destroy_sound_system	(SoundSystem*, std::function<void(uint8_t* ptr)> deallocate);
}

#endif
