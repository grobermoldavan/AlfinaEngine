#ifndef __ALFINA_WAV_FORMAT_H__
#define __ALFINA_WAV_FORMAT_H__

#include <cstdint>
#include <cstring>

#include "base_audio_format.h"

#include "engine/file_system/base_file_system.h"

namespace al::engine
{
	struct WavFormat
	{
		struct Riff
		{
			uint8_t		chunkId[4];		// must be == "RIFF"
			uint32_t	chunkSize;		// 
			uint8_t		format[4];		// must be == "WAVE"
		} 
		riff;

		struct Format
		{
			uint8_t		chunkId[4];		// must be == "fmt "
			uint32_t	chunkSize;		// 16 ?
			uint16_t	audioFormat;	// == 1 for PCM (non-compressed data)
			uint16_t	numChannels;	//
			uint32_t	sampleRate;		//
			uint32_t	byteRate;		// bytes per second
			uint16_t	blockAlign;		// size of one sample (all channels)
			uint16_t	bitsPerSample;	// 
		} 
		format;

		struct Data
		{
			uint8_t		chunkId[4];		// must be == "data"
			uint32_t	chunkSize;		// size of actual wav data
			uint8_t		firstByte;		// first byte of data
		}
		data;

		ErrorInfo validate() const
		{
			constexpr char* WAV_ERROR_HEADER = "Incorrect WAV format :: ";

			//AL_ASSERT_MSG(!std::strncmp(reinterpret_cast<const char*>(riff.chunkId)		, "RIFF", 4), WAV_ERROR_HEADER, "Incorrect RIFF chunk id");
			//AL_ASSERT_MSG(!std::strncmp(reinterpret_cast<const char*>(riff.format)		, "WAVE", 4), WAV_ERROR_HEADER, "Incorrect RIFF format id");
			//AL_ASSERT_MSG(!std::strncmp(reinterpret_cast<const char*>(format.chunkId)	, "fmt ", 4), WAV_ERROR_HEADER, "Incorrect Format chunk id");
			//AL_ASSERT_MSG(!std::strncmp(reinterpret_cast<const char*>(data.chunkId)		, "data", 4), WAV_ERROR_HEADER, "Incorrect Data chunk id");
			//AL_ASSERT_MSG(format.audioFormat == 1, WAV_ERROR_HEADER, "Only PCM formats are currently supported");

			return { ErrorInfo::Code::ALL_FINE };
		}
	};

	struct PlaybackPointer
	{
		size_t index;
	};

	class WavFile
	{
	public:
		WavFile(const char* path);
		~WavFile();

		void			read_data(uint8_t* buffer, size_t frames, const SoundParameters& destParameters, PlaybackPointer& playbackPtr);
		PlaybackPointer get_playback_ptr();

	private:
		FileHandle		handle;
		WavFormat*		format;
		SoundParameters parameters;

		template<size_t FileChannels = 1, size_t DestChannels = 1, typename DestType>
		inline void fill_buffer(uint8_t* buffer, size_t frames, const SoundParameters& destParameters, PlaybackPointer& playbackPtr, DestType(*func)(uint8_t*));
	};

	WavFile::WavFile(const char* path)
	{
		//ErrorInfo result = FileSys::read_file(path, &handle);
		////AL_ASSERT(result);
		//
		//format = reinterpret_cast<WavFormat*>(handle.get_data());
		//result = format->validate();
		////AL_ASSERT(result);
		//
		//parameters.bitsPerSample	= static_cast<size_t>(format->format.bitsPerSample);
		//parameters.channels			= static_cast<size_t>(format->format.numChannels);
		//parameters.sampleRate		= static_cast<size_t>(format->format.sampleRate);
	}

	WavFile::~WavFile()
	{
		//ErrorInfo result = FileSys::free_file_handle(&handle);
		//AL_ASSERT(result);
	}

	inline uint32_t sample_int24_to_int32(uint8_t* data)
	{
		return ((data[2] << 24) | (data[1] << 16) | (data[0] << 8));
	}

	inline uint32_t sample_int16_to_int32(uint8_t* data)
	{
		return ((data[1] << 24) | (data[0] << 16));
	}

	template<typename T>
	inline T sample_same_size(uint8_t* data)
	{
		return *reinterpret_cast<T*>(data);
	}

	void WavFile::read_data(uint8_t* buffer, size_t frames, const SoundParameters& destParameters, PlaybackPointer& playbackPtr)
	{
		//AL_ASSERT(handle.get_data());

		if (parameters.channels == destParameters.channels)
		{
			if (parameters.bitsPerSample == 16 && destParameters.bitsPerSample == 32)
			{
				fill_buffer(buffer, frames, destParameters, playbackPtr, sample_int16_to_int32);
			}
			else if (parameters.bitsPerSample == 24 && destParameters.bitsPerSample == 32)
			{
				fill_buffer(buffer, frames, destParameters, playbackPtr, sample_int24_to_int32);
			}
			else if (parameters.bitsPerSample == 32 && destParameters.bitsPerSample == 32)
			{
				fill_buffer(buffer, frames, destParameters, playbackPtr, sample_same_size<uint32_t>);
			}
			else
			{
				// @TODO: @NOTE: Add other bits variations if that assertion happends
				//AL_ASSERT_MSG(false, parameters.bitsPerSample, " :: ", destParameters.bitsPerSample, " : ", format->format.blockAlign);
			}
		}
		else if (parameters.channels == 1 && destParameters.channels == 2)
		{
			// @TODO: implement this
			//AL_NO_IMPLEMENTATION_ASSERT
		}
		else if (parameters.channels == 2 && destParameters.channels == 1)
		{
			// @TODO: implement this
			//AL_NO_IMPLEMENTATION_ASSERT
		}
		else
		{
			// Invalid code path
			//AL_NO_IMPLEMENTATION_ASSERT
		}
	}

	PlaybackPointer WavFile::get_playback_ptr()
	{
		return { 0 };
	}

	template<size_t FileChannels, size_t DestChannels, typename DestType>
	inline void WavFile::fill_buffer(uint8_t* buffer, size_t frames, const SoundParameters& destParameters, PlaybackPointer& playbackPtr, DestType(*func)(uint8_t*))
	{
		constexpr float PLAYBACK_SPEED = 1;

		const float		sampleRateRatio			= static_cast<float>(parameters.sampleRate) / static_cast<float>(destParameters.sampleRate);
		const uint8_t*	endData					= &format->data.firstByte + format->data.chunkSize;
		const size_t	fileBytesPerSample		= parameters.bitsPerSample / 8;
		const size_t	targetBytesPerSample	= destParameters.bitsPerSample / 8;

		for (size_t it = 0; it < frames; ++it)
		{
			size_t currentFrameId = static_cast<size_t>(static_cast<float>(playbackPtr.index++) * sampleRateRatio * PLAYBACK_SPEED);
			uint8_t* frame = &format->data.firstByte + currentFrameId * fileBytesPerSample * parameters.channels;
			if (frame >= endData)
			{
				playbackPtr.index = 0;
				frame = &format->data.firstByte;
			}

			if constexpr (FileChannels == DestChannels)
			{
				for (size_t channel = 0; channel < parameters.channels; ++channel)
				{
					DestType value = func(frame);
					frame += fileBytesPerSample;
					*reinterpret_cast<DestType*>(buffer) += value;
					buffer += targetBytesPerSample;
				}
			}
			else if constexpr (FileChannels == 1 && DestChannels == 2)
			{
				DestType value = func(frame);
				for (size_t channel = 0; channel < 2; ++channel)
				{
					*reinterpret_cast<DestType*>(buffer) += value;
					buffer += targetBytesPerSample;
				}
			}
			else if constexpr (FileChannels == 2 && DestChannels == 1)
			{
				// Just use the first channel
				DestType value = func(frame);
				*reinterpret_cast<DestType*>(buffer) += value;
			}
			else
			{
				static_assert(false, "Unsupported channel format");
			}
		}
	}
}

#endif
