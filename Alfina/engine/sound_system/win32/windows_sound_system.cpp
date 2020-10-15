
#include "windows_sound_system.h"

#include "engine/asserts/asserts.h"

namespace al::engine
{
	Win32SoundSystem::Win32SoundSystem(ApplicationWindow* window, FileSystem* fileSystem, const SoundParameters& parameters, StackAllocator* allocator) noexcept
		: SoundSystem       { window, fileSystem, parameters, allocator }
        , win32window		{ static_cast<Win32ApplicationWindow*>(window) }
		, win32fileSystem	{ static_cast<Win32FileSystem*>(fileSystem) }
	{
		win32flags.clear_flag(Win32SoundSystemFlags::IS_INITED);

		// Start window thread and wait for initialization
		threadArg =
		{
			this,
			::CreateEvent(NULL, TRUE, FALSE, TEXT("SoundInitEvent"))
		};
		soundSystemThread = ::CreateThread(NULL, 0, sound_update, &threadArg, 0, NULL);
		::WaitForSingleObject(threadArg.initEvent, INFINITE);
		::CloseHandle(threadArg.initEvent);

		win32flags.set_flag(Win32SoundSystemFlags::IS_INITED);
	}

	Win32SoundSystem::~Win32SoundSystem() noexcept
	{
		win32flags.clear_flag(Win32SoundSystemFlags::IS_RUNNING);
		
		// Wait for thread
		::WaitForSingleObject(soundSystemThread, INFINITE);
		::CloseHandle(soundSystemThread);
	}

	SoundParameters	Win32SoundSystem::get_valid_parameters() const noexcept
	{
		init_assert();
		return validParameters;
	}

	SoundId Win32SoundSystem::load_sound(SourceType type, const char* path) noexcept
	{
		init_assert();

		switch (type)
		{
			case SoundSystem::SourceType::WAV:
			{
				
				break;
			}
			default:
			{
				al_assert(false);
			}
		}

		return 0;
	}

	SoundSourceId Win32SoundSystem::create_sound_source(SoundId id) noexcept
	{
		init_assert();

		return 0;
	}

	void Win32SoundSystem::play_sound(SoundSourceId id) noexcept
	{
		init_assert();


	}

    void Win32SoundSystem::init_assert() const noexcept
    {
        al_assert(win32flags.get_flag(Win32SoundSystemFlags::IS_INITED));
    }

	DWORD sound_update(LPVOID voidArg)
	{
		// @TODO : add error handling

		SoundThreadArg* soundArgPtr = static_cast<SoundThreadArg*>(voidArg);
		Win32SoundSystem* win32soundSystem = soundArgPtr->soundSystem;

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
		al_assert(result == S_OK);

		// Get default device
		IMMDevice *device;
		result = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
		al_assert(result == S_OK);

		// Get audio client
		IAudioClient *audioClient;
		result = device->Activate
		(
			Win32SoundSystem::IID_IAudioClient,
			CLSCTX_ALL,
			0,
			reinterpret_cast<void**>(&audioClient)
		);
		al_assert(result == S_OK);

		// Describe format, that is needed
		WAVEFORMATEXTENSIBLE wfmt = {};
		wfmt.Format.wFormatTag		= WAVE_FORMAT_EXTENSIBLE;
		wfmt.Format.nChannels		= win32soundSystem->userParameters.channels;
		wfmt.Format.nSamplesPerSec	= win32soundSystem->userParameters.sampleRate;
		wfmt.Format.wBitsPerSample	= win32soundSystem->userParameters.bitsPerSample;
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
			al_assert(result == S_OK);
		}

		// Save valid sound parameters
		win32soundSystem->validParameters.bitsPerSample	= static_cast<size_t>(wfmt.Format.wBitsPerSample);
		win32soundSystem->validParameters.channels		= static_cast<size_t>(wfmt.Format.nChannels);
		win32soundSystem->validParameters.sampleRate	= static_cast<size_t>(wfmt.Format.nSamplesPerSec);

		// Get device period
		REFERENCE_TIME devicePeriod;
		result = audioClient->GetDevicePeriod(nullptr, &devicePeriod);
		al_assert(result == S_OK);

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
		al_assert(result == S_OK);

		// Get buffer size
		uint32_t bufferFrameCount;
		result = audioClient->GetBufferSize(&bufferFrameCount);
		al_assert(result == S_OK);

		// Get audio render
		IAudioRenderClient* audioRenderClient;
		result = audioClient->GetService
		(
			Win32SoundSystem::IID_IAudioRenderClient,
			reinterpret_cast<void**>(&audioRenderClient)
		);
		al_assert(result == S_OK);
		
		// Set flag for the audio system that thread is running
		win32soundSystem->win32flags.set_flag(Win32SoundSystem::Win32SoundSystemFlags::IS_RUNNING);

		// Start audio client
		audioClient->Start();
		
		// Notify main thread
		::SetEvent(soundArgPtr->initEvent);
		// After this soundArgPtr->initEvent will not be valid (handle will be closed)

		// Update loop
		while (win32soundSystem->win32flags.get_flag(Win32SoundSystem::Win32SoundSystemFlags::IS_RUNNING))
		{
			// Sleep for half the buffer duration
			Sleep(devicePeriod / 10000 / 2);
			
			// Get current available frames
			uint32_t framesAvailable;
			result = audioClient->GetCurrentPadding(&framesAvailable);
			al_assert(result == S_OK);

			framesAvailable = bufferFrameCount - framesAvailable;
			BYTE* data;

			// Try get buffer
			if (audioRenderClient->GetBuffer(framesAvailable, &data) == S_OK)
			{
				// Fill buffer with stuff
#if 1
				if (wfmt.Format.nChannels == 1)
				{
					if		(wfmt.Format.wBitsPerSample == 8)	win32soundSystem->dbg_fill_sound_buffer<uint8_t, 1>	(data, framesAvailable, wfmt.Format.nSamplesPerSec);
					else if (wfmt.Format.wBitsPerSample == 16)	win32soundSystem->dbg_fill_sound_buffer<uint16_t, 1>(data, framesAvailable, wfmt.Format.nSamplesPerSec);
					else if (wfmt.Format.wBitsPerSample == 32)	win32soundSystem->dbg_fill_sound_buffer<uint32_t, 1>(data, framesAvailable, wfmt.Format.nSamplesPerSec);
				}
				else // channels == 2
				{
					if		(wfmt.Format.wBitsPerSample == 8)	win32soundSystem->dbg_fill_sound_buffer<uint8_t, 2>	(data, framesAvailable, wfmt.Format.nSamplesPerSec);
					else if (wfmt.Format.wBitsPerSample == 16)	win32soundSystem->dbg_fill_sound_buffer<uint16_t, 2>(data, framesAvailable, wfmt.Format.nSamplesPerSec);
					else if (wfmt.Format.wBitsPerSample == 32)	win32soundSystem->dbg_fill_sound_buffer<uint32_t, 2>(data, framesAvailable, wfmt.Format.nSamplesPerSec);
				}
#else
				win32soundSystem->fill_buffer(data, framesAvailable);
#endif
				// Release buffer
				audioRenderClient->ReleaseBuffer(framesAvailable, 0);
			}
			else
			{
				al_assert(false);
			}
		}
		
		audioClient			->Stop();
		audioRenderClient	->Release();
		audioClient			->Release();
		device				->Release();
		enumerator			->Release();

		::CoUninitialize();

		return 0;
	}
}

#undef AL_SOUND_SYS_INIT_ASSERT
