#ifndef AL_DISPENSABLE_H
#define AL_DISPENSABLE_H

namespace al
{
	struct      __NonSpecifiedValue { };
	constexpr   __NonSpecifiedValue nonSpecifiedValue;
	
	template<typename T>
	class Dispensable
	{
	public:
		Dispensable(const T& value_ref)
			: isSpecified   { true }
			, value         { value_ref }
		{ }
		
		Dispensable()
			: isSpecified   { false }
			, value         { }
		{ }
		
		Dispensable(const __NonSpecifiedValue& non_specified)
			: isSpecified   { false }
			, value         { }
		{ }

		inline  Dispensable&    operator =  (T newValue)                    { value = newValue;    isSpecified = true;              return *this; } 
		inline  Dispensable&    operator =  (const Dispensable& other)      { value = other.value; isSpecified = other.isSpecified; return *this; } 
		inline                  operator T  ()  const                       { return value; }
		inline  T*              operator -> ()  const                       { return const_cast<T*>(&value); }
		inline  bool            is_specified()  const                       { return isSpecified; }
		inline	const T&		get_value	()	const						{ return value; }

	private:
		T       value;
		bool    isSpecified;
	};
}

#endif