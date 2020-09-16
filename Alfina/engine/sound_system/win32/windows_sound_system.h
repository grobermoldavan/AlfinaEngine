#ifndef __ALFINA_WINDOWS_SOUND_SYSTEM_H__
#define __ALFINA_WINDOWS_SOUND_SYSTEM_H__

#include <windows.h>
#include <Audioclient.h>
#include <audiopolicy.h>
#include <mmdeviceapi.h>

#include "engine/sound_system/base_sound_system.h"

namespace al::engine
{
	class Win32ApplicationWindow;

	class Win32SoundSystem : public SoundSystem
	{
	public:
		Win32SoundSystem(Win32ApplicationWindow* _win32window);
		~Win32SoundSystem();

		virtual void	init		(const SoundParameters& parameters) override;

		virtual SoundId load_sound	(SourceType type, const char* path)	override;
		virtual void	play_sound	(SoundId id)						override;
	};


}

#if defined(AL_UNITY_BUILD)
#	include "windows_sound_system.cpp"
#else

#endif

#endif
