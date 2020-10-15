#ifndef __ALFINA_BASE_SOUND_SYSTEM_H__
#define __ALFINA_BASE_SOUND_SYSTEM_H__

#include <cstdint>

#include "sound_parameters.h"
#include "audio_formats/audio_formats.h"
#include "engine/memory/stack_allocator.h"

namespace al::engine
{
	using SoundId		= size_t;
	using SoundSourceId = size_t;

    class ApplicationWindow;
	class FileSystem;

	class SoundSystem
	{
	public:
		enum class SourceType
		{
			WAV,
			__end
		};

        SoundSystem(ApplicationWindow* window, FileSystem* fileSystem, const SoundParameters& parameters, StackAllocator* allocator);
		virtual ~SoundSystem() = default;

        // Sound system must implement method, which returns actual sound parameters that are used on users machine
        // Theese parameters may be different from userParameters
		virtual SoundParameters	get_valid_parameters()									const   noexcept    = 0;

		// SoundId represents actual sound data stored in sound system
		// SoundSourceId represents sound source. There could be multiple sound sources using the same SoundId
		virtual SoundId			load_sound			(SourceType type, const char* path)			noexcept    = 0;
		virtual SoundSourceId	create_sound_source	(SoundId id)								noexcept    = 0;
		virtual void			play_sound			(SoundSourceId id)							noexcept    = 0;

	protected:
        SoundParameters userParameters;
        StackAllocator* allocator;
	};

    SoundSystem::SoundSystem(ApplicationWindow* window, FileSystem* fileSystem, const SoundParameters& parameters, StackAllocator* allocator)
        : userParameters{ parameters }
        , allocator     { allocator }
    { }
}

#endif
