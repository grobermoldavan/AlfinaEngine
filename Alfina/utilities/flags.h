#ifndef __ALFINA_FLAGS_H__
#define __ALFINA_FLAGS_H__

#include <cstdint>

namespace al
{
	class IFlags
	{
	public:
		virtual void set_flag   (uint16_t flag)         = 0;
		virtual void clear_flag (uint16_t flag)         = 0;
		virtual bool get_flag   (uint16_t flag) const   = 0;
		virtual void clear      ()                      = 0;
	};

	class Flags32 : public IFlags
	{
	public:
		Flags32()
			: flags{ }
		{ }

		Flags32(uint32_t initial_flags)
			: flags{ initial_flags }
		{ }

		inline void set_flag(uint16_t flag) override
		{
			flags |= 1 << flag;
		}

		inline bool get_flag(uint16_t flag) const override
		{
			return static_cast<bool>(flags & (1 << flag));
		}

		inline void clear_flag(uint16_t flag) override
		{
			flags &= ~(1 << flag);
		}

		inline void clear() override
		{
			flags = 0;
		}

	private:
		uint32_t flags;
	};
}

#endif