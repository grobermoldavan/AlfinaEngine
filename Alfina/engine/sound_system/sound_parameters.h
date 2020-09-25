#ifndef __ALFINA_SOUND_PARAMETERS_H__
#define __ALFINA_SOUND_PARAMETERS_H__

#include "utilities/dispensable.h"

namespace al::engine
{
	struct SoundParameters
	{
		enum class Channels
		{
			MONO,
			STEREO,
			__end
		};

		enum class BytesPerSample
		{
			ONE,
			TWO,
			THREE,
			FOUR,
			__end
		};

		Dispensable<uint32_t>		sampleRate;
		Dispensable<Channels>		channels;
		Dispensable<BytesPerSample>	bytesPerSample;

		static BytesPerSample convert_bits_to_bytes_per_sample(uint32_t bitsPerSample)
		{
			const uint32_t bytes = bitsPerSample / 8;
			AL_ASSERT_MSG(bytes > 0 && bytes <= static_cast<uint32_t>(SoundParameters::BytesPerSample::__end), "Unsupported bitsPerSample : ", bitsPerSample);
			return static_cast<SoundParameters::BytesPerSample>(bytes);
		}

		static Channels convert_channels(uint32_t channels)
		{
			AL_ASSERT_MSG(channels > 0 && channels < 3, "Unsupported number of channels : ", channels);
			return static_cast<SoundParameters::Channels>(channels - 1);
		}
	};
}

#endif
