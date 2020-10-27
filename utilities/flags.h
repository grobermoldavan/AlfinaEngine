#ifndef AL_FLAGS_H
#define AL_FLAGS_H

#include <cstdint>

namespace al
{
	class IFlags
	{
	public:
		virtual void set_flag   (uint32_t flag)         = 0;
		virtual void clear_flag (uint32_t flag)         = 0;
		virtual bool get_flag   (uint32_t flag) const   = 0;
		virtual void clear      ()                      = 0;
	};

	class Flags32 : public IFlags
	{
	public:
		Flags32(uint32_t initial_flags = 0)
			: flags{ initial_flags }
		{ }

		inline void set_flag(uint32_t flag) override
		{
			flags |= 1 << flag;
		}

		inline bool get_flag(uint32_t flag) const override
		{
			return static_cast<bool>(flags & (1 << flag));
		}

		inline void clear_flag(uint32_t flag) override
		{
			flags &= ~(1 << flag);
		}

		inline void clear() override
		{
			flags = 0;
		}

		uint32_t get_flags()
		{
			return flags;
		}

	private:
		uint32_t flags;
	};

	class Flags128 : public IFlags
	{
	public:
		Flags128(uint32_t f1 = 0, uint32_t f2 = 0, uint32_t f3 = 0, uint32_t f4 = 0)
			: flags{ f1, f2, f3, f4 }
		{ }

		inline void set_flag(uint32_t flag) override
		{
			const auto id = flag / 32U;
			flags[id] |= 1 << (flag - (id * 32U));
		}

		inline bool get_flag(uint32_t flag) const override
		{
			const auto id = flag / 32U;
			return static_cast<bool>(flags[id] & (1 << (flag - (id * 32U))));
		}

		inline void clear_flag(uint32_t flag) override
		{
			const auto id = flag / 32U;
			flags[id] &= ~(1UL << (flag - (id * 32U)));
		}

		inline void clear() override
		{
			flags[0] = flags[1] = flags[2] = flags[3] = 0;
		}
	private:
		uint32_t flags[4];
	};
}

#endif