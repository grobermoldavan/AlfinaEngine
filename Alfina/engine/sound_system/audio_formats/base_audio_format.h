#ifndef __ALFINA_BASE_AUDIO_FORMAT_H__
#define __ALFINA_BASE_AUDIO_FORMAT_H__

#include <cstdint>

#include "engine/sound_system/sound_parameters.h"

namespace al::engine
{
	namespace audio_format_private
	{
		template<typename T>
		struct HasReadData
		{
			template<typename U, void(U::*)(uint8_t*, size_t)> struct SFINAE {};
			template<typename U> static char Test(SFINAE<U, &U::read_data>*);
			template<typename U> static int Test(...);
			static const bool value = sizeof(Test<T>(0)) == sizeof(char);
		};
	}

	template<typename T>
	struct IsAudioFormat
	{
		static const bool value = audio_format_private::HasReadData<T>::value;
	};
}

#endif
