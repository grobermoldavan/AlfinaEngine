#if defined(AL_UNITY_BUILD)

#else
#	include "windows_sound_system.h"
#endif

#include "engine/engine_utilities/asserts.h"
#include "engine/allocation/allocation.h"
#include "engine/engine_utilities/thread/thread_utilities.h"

#define AL_SOUND_SYS_INIT_ASSERT AL_ASSERT_MSG(win32flags.get_flag(Win32SoundSystemFlags::IS_INITED), "Sound system must be initialized before use.")

namespace al::engine
{
	ErrorInfo create_sound_system(SoundSystem** soundSystem, ApplicationWindow* window)
	{
		*soundSystem = static_cast<SoundSystem*>(AL_DEFAULT_CONSTRUCT(Win32SoundSystem, Win32SoundSystem::ALLOCATOR_TAG, reinterpret_cast<Win32ApplicationWindow*>(window)));
		if (*soundSystem)
		{
			return{ ErrorInfo::Code::ALL_FINE };
		}
		else
		{
			return{ ErrorInfo::Code::BAD_ALLOC };
		}
	}

	ErrorInfo destroy_sound_system(SoundSystem* soundSystem)
	{
		if (soundSystem) AL_DEFAULT_DESTRUCT(soundSystem, Win32SoundSystem::ALLOCATOR_TAG);
		return{ ErrorInfo::Code::ALL_FINE };
	}

	Win32SoundSystem::Win32SoundSystem(Win32ApplicationWindow* _win32window)
		: win32window{ _win32window }
	{
		win32flags.clear_flag(Win32SoundSystemFlags::IS_INITED);
	}

	Win32SoundSystem::~Win32SoundSystem()
	{
		win32flags.clear_flag(Win32SoundSystemFlags::IS_RUNNING);
		soundSysThread.join();
	}

	void Win32SoundSystem::init(const SoundParameters& parameters)
	{
		userParameters = parameters;

		std::promise<void> creation_promise;
		std::future<void> creation_future = creation_promise.get_future();

		soundSysThread = std::thread{ &Win32SoundSystem::sound_update, this, std::move(creation_promise) };
		set_thread_priority(soundSysThread, ThreadPriority::HIGHEST);
		creation_future.get();

		win32flags.set_flag(Win32SoundSystemFlags::IS_INITED);
	}

	SoundParameters	Win32SoundSystem::get_valid_parameters() const
	{
		AL_SOUND_SYS_INIT_ASSERT;

		return{};
	}

	SoundId Win32SoundSystem::load_sound(SourceType type, const char* path)
	{
		AL_SOUND_SYS_INIT_ASSERT;

		return 0;
	}

	void Win32SoundSystem::play_sound(SoundId id) const
	{
		AL_SOUND_SYS_INIT_ASSERT;

	}

	void Win32SoundSystem::sound_update(std::promise<void> creation_promise)
	{
		HRESULT result;

		::CoInitialize(0);

		// Get device enumerator
		IMMDeviceEnumerator *enumerator;
		result = ::CoCreateInstance
		(
			Win32SoundSystem::CLSID_MMDeviceEnumerator,
			NULL,
			CLSCTX_ALL,
			Win32SoundSystem::IID_IMMDeviceEnumerator,
			reinterpret_cast<void**>(&enumerator)
		);
		AL_ASSERT_MSG(result == S_OK, "result is : ", result);

		// Get default device
		IMMDevice *device;
		result = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
		AL_ASSERT_MSG(result == S_OK, "result is : ", result);

		// Get audio client
		IAudioClient *audioClient;
		result = device->Activate
		(
			Win32SoundSystem::IID_IAudioClient,
			CLSCTX_ALL,
			0,
			reinterpret_cast<void**>(&audioClient)
		);
		AL_ASSERT_MSG(result == S_OK, "result is : ", result);

		static WORD spChannelsToWord[] =
		{
			1,	// MONO
			2	// STEREO
		};

		static WORD spBytesToWord[] =
		{
			8,	// ONE
			16,	// TWO
			32	// FOUR
		};

		// Describe format, that is needed
		WAVEFORMATEXTENSIBLE wfmt = {};
		wfmt.Format.wFormatTag		= WAVE_FORMAT_EXTENSIBLE;
		wfmt.Format.nChannels		= userParameters.channels.is_specified()		? spChannelsToWord[static_cast<int>(userParameters.channels.get_value())]		: 2;
		wfmt.Format.nSamplesPerSec	= userParameters.sampleRate.is_specified()		? userParameters.sampleRate														: 44100;
		wfmt.Format.wBitsPerSample	= userParameters.bytesPerSample.is_specified()	? spBytesToWord[static_cast<int>(userParameters.bytesPerSample.get_value())]	: 16;
		wfmt.Format.nBlockAlign		= wfmt.Format.nChannels * wfmt.Format.wBitsPerSample / 8;
		wfmt.Format.nAvgBytesPerSec = wfmt.Format.nSamplesPerSec * wfmt.Format.nBlockAlign;
		wfmt.Format.cbSize			= sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);

		wfmt.Samples.wValidBitsPerSample = wfmt.Format.wBitsPerSample;
		wfmt.dwChannelMask = 0;
		wfmt.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;

		// Check if described format is supported
		WAVEFORMATEX* closestMatch;
		result = audioClient->IsFormatSupported
		(
			AUDCLNT_SHAREMODE_SHARED,
			reinterpret_cast<WAVEFORMATEX *>(&wfmt),
			&closestMatch
		);
		if (result == S_FALSE)
		{
			// If it is not supported, but system gives us closest match, 
			// then use that closest match
			wfmt.Format = *closestMatch;
			wfmt.Samples.wValidBitsPerSample = wfmt.Format.wBitsPerSample;
		}
		else
		{
			// Else assert if result is not S_OK
			AL_ASSERT_MSG(result == S_OK, "result is : ", result);
		}

		// Save valid sound parameters
		{
			validParameters.bytesPerSample	= SoundParameters::convert_bits_to_bytes_per_sample(static_cast<uint32_t>(wfmt.Format.wBitsPerSample));
			validParameters.channels		= SoundParameters::convert_channels(static_cast<uint32_t>(wfmt.Format.nChannels));
			validParameters.sampleRate		= static_cast<uint32_t>(wfmt.Format.nSamplesPerSec);
		}

		// Get device period
		REFERENCE_TIME devicePeriod;
		result = audioClient->GetDevicePeriod(nullptr, &devicePeriod);
		AL_ASSERT_MSG(result == S_OK, "result is : ", result);

		// Initialize device using resulting settings
		result = audioClient->Initialize
		(
			AUDCLNT_SHAREMODE_SHARED,
			0,
			devicePeriod,
			devicePeriod,
			reinterpret_cast<WAVEFORMATEX *>(&wfmt),
			nullptr
		);
		AL_ASSERT_MSG(result == S_OK, "result is : ", result);

		// Get buffer size
		uint32_t bufferFrameCount;
		result = audioClient->GetBufferSize(&bufferFrameCount);
		AL_ASSERT_MSG(result == S_OK, "result is : ", result);

		// Get audio render
		IAudioRenderClient* audioRenderClient;
		result = audioClient->GetService
		(
			Win32SoundSystem::IID_IAudioRenderClient,
			reinterpret_cast<void**>(&audioRenderClient)
		);
		AL_ASSERT_MSG(result == S_OK, "result is : ", result);
		
		// Set flag for the audio system that thread is running
		win32flags.set_flag(Win32SoundSystemFlags::IS_RUNNING);

		// Start audio client
		audioClient->Start();
		
		// Notify main thread
		creation_promise.set_value();

		// Update loop
		while (win32flags.get_flag(Win32SoundSystemFlags::IS_RUNNING))
		{
			// Sleep for half the buffer duration
			Sleep(devicePeriod / 10000 / 3);
			
			// Get current available frames
			uint32_t framesAvailable;
			result = audioClient->GetCurrentPadding(&framesAvailable);
			AL_ASSERT_MSG(result == S_OK, "result is : ", result);

			framesAvailable = bufferFrameCount - framesAvailable;
			BYTE* data;

			// Try get buffer
			if (audioRenderClient->GetBuffer(framesAvailable, &data) == S_OK)
			{
				// Fill buffer with stuff
#if 1
				if (wfmt.Format.nChannels == 1)
				{
					if		(wfmt.Format.wBitsPerSample == 8)	dbg_fill_sound_buffer<uint8_t, 1>	(data, framesAvailable, wfmt.Format.nSamplesPerSec);
					else if (wfmt.Format.wBitsPerSample == 16)	dbg_fill_sound_buffer<uint16_t, 1>	(data, framesAvailable, wfmt.Format.nSamplesPerSec);
					else if (wfmt.Format.wBitsPerSample == 32)	dbg_fill_sound_buffer<uint32_t, 1>	(data, framesAvailable, wfmt.Format.nSamplesPerSec);
				}
				else // channels == 2
				{
					if		(wfmt.Format.wBitsPerSample == 8)	dbg_fill_sound_buffer<uint8_t, 2>	(data, framesAvailable, wfmt.Format.nSamplesPerSec);
					else if (wfmt.Format.wBitsPerSample == 16)	dbg_fill_sound_buffer<uint16_t, 2>	(data, framesAvailable, wfmt.Format.nSamplesPerSec);
					else if (wfmt.Format.wBitsPerSample == 32)	dbg_fill_sound_buffer<uint32_t, 2>	(data, framesAvailable, wfmt.Format.nSamplesPerSec);
				}
#endif
				// Release buffer
				audioRenderClient->ReleaseBuffer(framesAvailable, 0);
			}
			else
			{
				AL_LOG_SHORT(Logger::MESSAGE, "Can't get buffer")
			}
		}
		
		audioClient->Stop();
		audioRenderClient->Release();
		audioClient->Release();
		device->Release();
		enumerator->Release();

		::CoUninitialize();
	}
}

#undef AL_SOUND_SYS_INIT_ASSERT
