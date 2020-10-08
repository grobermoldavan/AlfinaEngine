#ifndef __ALFINA_WINDOWS_SOUND_SYSTEM_H__
#define __ALFINA_WINDOWS_SOUND_SYSTEM_H__

#include <windows.h>
#include <Audioclient.h>
#include <audiopolicy.h>
#include <mmdeviceapi.h>
#include <Objbase.h>

#include <cstdint>
#include <limits>

#include "engine/sound_system/base_sound_system.h"
#include "engine/file_system/win32/windows_file_system.h"

#include "utilities/flags.h"

#ifdef max
#	undef max
#endif

namespace al::engine
{
	class Win32ApplicationWindow;
	class Win32SoundSystem;

	struct SoundThreadArg
	{
		Win32SoundSystem*	soundSystem;
		HANDLE				initEvent;
	};

	class Win32SoundSystem : public SoundSystem
	{
	public:
		enum Win32SoundSystemFlags : uint32_t
		{
			IS_INITED,
			IS_RUNNING
		};

		Win32SoundSystem(Win32ApplicationWindow* win32window, Win32FileSystem* win32fileSystem);
		~Win32SoundSystem();

		virtual void			init				(const SoundParameters& parameters)			override;
		virtual SoundParameters	get_valid_parameters()									const	override;

		virtual SoundId			load_sound			(SourceType type, const char* path)			override;
		virtual SoundSourceId	create_sound_source	(SoundId id)								override;
		virtual void			play_sound			(SoundSourceId id)							override;

	private:
		static constexpr const GUID IID_IAudioClient			= { 0x1CB9AD4C, 0xDBFA, 0x4c32, 0xB1, 0x78, 0xC2, 0xF5, 0x68, 0xA7, 0x03, 0xB2 };
		static constexpr const GUID IID_IAudioRenderClient		= { 0xF294ACFC, 0x3146, 0x4483, 0xA7, 0xBF, 0xAD, 0xDC, 0xA7, 0xC2, 0x60, 0xE2 };
		static constexpr const GUID CLSID_MMDeviceEnumerator	= { 0xBCDE0395, 0xE52F, 0x467C, 0x8E, 0x3D, 0xC4, 0x57, 0x92, 0x91, 0x69, 0x2E };
		static constexpr const GUID IID_IMMDeviceEnumerator		= { 0xA95664D2, 0x9614, 0x4F35, 0xA7, 0x46, 0xDE, 0x8D, 0xB6, 0x36, 0x17, 0xE6 };
		static constexpr const GUID PcmSubformatGuid			= { STATIC_KSDATAFORMAT_SUBTYPE_PCM };

		Win32ApplicationWindow* win32window;
		Win32FileSystem*		win32fileSystem;

		SoundParameters			userParameters;		// this parameters are provided by the user
		SoundParameters			validParameters;	// this is actual parameters that are used by sound system
		Flags32					win32flags;
		SoundThreadArg			threadArg;
		HANDLE					soundSystemThread;

		friend DWORD sound_update(LPVOID voidArg);











		void fill_buffer(BYTE* buffer, uint32_t frames)
		{
			if (frames == 0) return;

#if 1
			// silence
			const uint32_t bytesPerFrame = validParameters.bitsPerSample / 8 * validParameters.channels;
			std::memset(buffer, 0, frames * bytesPerFrame);
#endif
		}

		template<typename SampleType, int channels>
		void dbg_fill_sound_buffer(BYTE* buffer, uint32_t frames, DWORD samplesPerSec)
		{
			constexpr float loudness = 0.25f;
			static float pos = 0.0f;
			
			for (uint32_t it = 0; it < frames; ++it)
			{
				const float dK = (440.0f * 6.283185307f) / static_cast<float>(samplesPerSec);
				const float val = std::sin(pos) * (static_cast<float>(std::numeric_limits<SampleType>::max()) * 0.75f) * loudness;
				pos += dK;
				
				if constexpr (channels == 1)
				{
					SampleType* v = reinterpret_cast<SampleType*>(&buffer[it * sizeof(SampleType)]);
					*v = static_cast<SampleType>(val);
				}
				else
				{
					SampleType* v1 = reinterpret_cast<SampleType*>(&buffer[(it * 2 + 0) * sizeof(SampleType)]);
					SampleType* v2 = reinterpret_cast<SampleType*>(&buffer[(it * 2 + 1) * sizeof(SampleType)]);
					*v1 = static_cast<SampleType>(val);
					*v2 = static_cast<SampleType>(val);
				}
			}
		}
	};
}

#include "windows_sound_system.cpp"

#endif
