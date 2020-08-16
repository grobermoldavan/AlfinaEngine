#ifndef __ALFINA_SMOOTH_AVERAGE_H__
#define __ALFINA_SMOOTH_AVERAGE_H__

namespace al
{
	template<typename T>
	class SmoothAverage
	{
	public:
		SmoothAverage(float constant = 0.5f)
			: last              { }
			, average           { }
			, smoothingConstant { constant }
		{ }

		inline void push(T value)
		{
			average = smoothingConstant * last + (1.0f - smoothingConstant) * average;
			last = value;
		}

		inline T get()
		{
			return average;
		}

	private:
		T       last;
		T       average;
		float   smoothingConstant;
	};
}

#endif