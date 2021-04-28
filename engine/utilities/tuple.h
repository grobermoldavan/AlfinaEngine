#ifndef AL_TUPLE_H
#define AL_TUPLE_H

#include "engine/types.h"

namespace al
{
    namespace impl
    {
        template<uSize It, typename T, typename ... Other>
        struct NthElementType : NthElementType<It - 1, Other...>
        { };

        template<typename T, typename ... Other>
        struct NthElementType<0, T, Other...>
        {
            using Type = T;
        };

        template<uSize Count, typename T, typename ... Other>
        struct ImplTuple
        {
            T value;
            ImplTuple<Count - 1, Other...> inner;
            operator T ()
            {
                return value;
            }
        };

        template<typename T, typename ... Other>
        struct ImplTuple<0, T, Other...>
        {
            T value;
            operator T ()
            {
                return value;
            }
        };

        template<uSize Index, uSize Size, typename T, typename ... Other>
        auto get(ImplTuple<Size, T, Other...>& tuple) -> typename NthElementType<Index, T, Other...>::Type&
        {
            if constexpr (Index != 0)
            {
                return get<Index - 1>(tuple.inner);
            }
            else
            {
                return tuple.value;
            }
        }
    }

    template<typename T, typename ... Other>
    struct Tuple : impl::ImplTuple<sizeof...(Other), T, Other...>
    { };

    template<uSize Index, typename T, typename ... Other>
    auto get(Tuple<T, Other...>& tuple) -> typename impl::NthElementType<Index, T, Other...>::Type&
    {
        return impl::get<Index>(tuple);
    }
}

#endif
