#if defined(AL_UNITY_BUILD)

#else
#	include "windows_sound_system.h"
#endif

#include "engine/engine_utilities/asserts.h"

namespace al::engine
{
	ErrorInfo create_sound_system(SoundSystem** soundSystem, ApplicationWindow* window)
	{
		*soundSystem = static_cast<SoundSystem*>(new Win32SoundSystem(static_cast<Win32ApplicationWindow*>(window)));
		return{ ErrorInfo::Code::ALL_FINE };
	}

	ErrorInfo destroy_sound_system(const SoundSystem* soundSystem)
	{
		if (soundSystem) delete soundSystem;
		return{ ErrorInfo::Code::ALL_FINE };
	}

	Win32SoundSystem::Win32SoundSystem(Win32ApplicationWindow* _win32window)
	{

	}

	Win32SoundSystem::~Win32SoundSystem()
	{

	}

	void Win32SoundSystem::init(const SoundParameters& parameters)
	{

		
	}

	SoundId Win32SoundSystem::load_sound(SourceType type, const char* path)
	{
		
		return 0;
	}

	void Win32SoundSystem::play_sound(SoundId id)
	{

	}
}
