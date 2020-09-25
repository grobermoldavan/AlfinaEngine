#ifndef __ALFINA_WAV_FORMAT_H__
#define __ALFINA_WAV_FORMAT_H__

#include <cstdint>
#include <cstring>

#include "base_audio_format.h"

#include "engine/platform/base_file_sys.h"
#include "engine/engine_utilities/asserts.h"

namespace al::engine
{
	struct WavFormat
	{
		struct Riff
		{
			uint8_t		chunkId[4];		// must be == "RIFF"
			uint32_t	chunksSize;		// 
			uint8_t		format[4];		// must be == "WAVE"
		} 
		riff;

		struct Format
		{
			uint8_t		chunkId[4];		// must be == "fmt "
			uint32_t	chunksSize;		// 16 ?
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
			uint32_t	chunksSize;		// size of actual wav data
			uint8_t		firstByte;		// first byte of data
		}
		data;

		ErrorInfo validate() const
		{
			constexpr char* WAV_ERROR_HEADER = "Incorrect WAV format :: ";

			AL_ASSERT_MSG(!std::strncmp(reinterpret_cast<const char*>(riff.chunkId)		, "RIFF", 4), WAV_ERROR_HEADER, "Incorrect RIFF chunk id");
			AL_ASSERT_MSG(!std::strncmp(reinterpret_cast<const char*>(riff.format)		, "WAVE", 4), WAV_ERROR_HEADER, "Incorrect RIFF format id");
			AL_ASSERT_MSG(!std::strncmp(reinterpret_cast<const char*>(format.chunkId)	, "fmt ", 4), WAV_ERROR_HEADER, "Incorrect Format chunk id");
			AL_ASSERT_MSG(!std::strncmp(reinterpret_cast<const char*>(data.chunkId)		, "data", 4), WAV_ERROR_HEADER, "Incorrect Data chunk id");

			return { ErrorInfo::Code::ALL_FINE };
		}
	};

	

	class WavFile
	{
	public:
		WavFile(const char* path);
		~WavFile();

		void read_data(uint8_t* dst, size_t framesNum, const SoundParameters& parameters);

	private:
		FileHandle		handle;
		WavFormat*		format;
		SoundParameters parameters;
	};

	WavFile::WavFile(const char* path)
	{
		ErrorInfo result = FileSys::read_file(path, &handle);
		AL_ASSERT(result);

		format = reinterpret_cast<WavFormat*>(handle.get_data());
		result = format->validate();
		AL_ASSERT(result);

		parameters.bytesPerSample	= SoundParameters::convert_bits_to_bytes_per_sample(static_cast<uint32_t>(format->format.bitsPerSample));
		parameters.channels			= SoundParameters::convert_channels(static_cast<uint32_t>(format->format.numChannels));
		parameters.sampleRate		= format->format.sampleRate;
	}

	WavFile::~WavFile()
	{
		ErrorInfo result = FileSys::free_file_handle(&handle);
		AL_ASSERT(result);
	}

	void WavFile::read_data(uint8_t* dst, size_t framesNum, const SoundParameters& parameters)
	{
		AL_ASSERT(handle.get_data());


	}
}

#endif
