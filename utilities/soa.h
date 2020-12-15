#ifndef AL_SOA_H
#define AL_SOA_H

// @NOTE :  This header contains Soa class as well as some macros for basic reflection information in C++.
//          In this case Soa is a templated class which stores each type provided in teplate parameters in a
//          separate array. User can accsess data of Soa array trough element arrays (via get_element_array method)
//          or by retrieveing all values at the given index at once (via get_as method). In order to use get_as
//          method given type must declare special method template via al_declare_get_method() macro. But in order
//          for this macro to work type must declare other reflection info via other macros.
//
//          ==========================================================================================
//           BASIC REFLECTION INFO
//          ==========================================================================================
//
//          For example, full correct type with reflection info for vector of three float elements will look like this :
//          struct vec3
//          {
//              al_declare_reflection(vec3) // creates initial reflection info
//
//              al_reflective(float, x)     // creates float x member with corresponding reflection info
//              al_reflective(float, y)     // creates float y member with corresponding reflection info
//              al_reflective(float, z)     // creates float z member with corresponding reflection info
//
//              al_declare_set_method()     // creates setter method template (this will be used by Soa type in get_as method)
//              al_declare_get_method()     // creates getter method template (this will be used by Soa type in set_as method)
//
//              al_declare_soa_type()       // creates a type info about Soa type which will be able to hold every reflected member
//                                          //         of a struct as a separate array. In other words, for this struct it will be
//                                          //         like this : vec3::Soa::type == Soa<float, float, float>
//          };
//
//          ==========================================================================================
//           BASIC SOA INFO
//          ==========================================================================================
//
//          For this vec3 type SOA array can be declared the next way :
//                  using SoaT = typename vec3::Soa::type;
//                  SoaT soaInstance{ memory, numberOfElements };
//
//          Element arrays can be retrieved the next way :
//                  auto elementZeroArray = soaInstance.get_element_array<0>();
//                  for (auto element : elementZeroArray)
//                  {
//                      // Element arrays can be iterated like standard arrays
//                  }
//
//          To retrieve all values at a given index user can use the following method : 
//                  vec3 value = soaInstance.get_as<vec3>(index);
//
//          To set all values at a given index user can use the following method :
//                  vec3 value{ 0, 1, 2 };
//                  soaInstance.set_as<vec3>(index, &value);
//
//          ==========================================================================================
//           EXTENDED REFLECTION INFO
//          ==========================================================================================
//
//          Reflected types give user an ability to retrieve information about type members.
//          This implementation provides the following :
//              boolean value which tells if info is valid  :: SomeReflectedType::element_info<index>::is_enabled
//              typedef of a type at given index            :: SomeReflectedType::element_info<index>::type
//              type name as a cstring                      :: SomeReflectedType::element_info<index>::type_name
//              element (type member) name as cstr          :: SomeReflectedType::element_info<index>::element_name
//              static getter and setter methods            :: using Type = typename SomeReflectedType::element_info<index>::type;
//                                                          :: Type SomeReflectedType::element_info<index>::get(SomeReflectedType* instance)
//                                                          :: void SomeReflectedType::element_info<index>::set(SomeReflectedType* instance, Type value)
//
//          For example, vec3 reflection info will look like this :
//
//          vec3::element_info<0>::is_enabled   == true
//          vec3::element_info<0>::type         == float
//          vec3::element_info<0>::type_name    == "float"
//          vec3::element_info<0>::element_name == "x"
//          float vec3::element_info<0>::get(vec3* inst) { return inst->x; }
//          void vec3::element_info<0>::set(vec3* inst, float value) { inst->x = value; }
//
//          vec3::element_info<1>::is_enabled   == true
//          vec3::element_info<1>::type         == float
//          vec3::element_info<1>::type_name    == "float"
//          vec3::element_info<1>::element_name == "y"
//          float vec3::element_info<1>::get(vec3* inst) { return inst->y; }
//          void vec3::element_info<1>::set(vec3* inst, float value) { inst->y = value; }
//
//          vec3::element_info<2>::is_enabled   == true
//          vec3::element_info<2>::type         == float
//          vec3::element_info<2>::type_name    == "float"
//          vec3::element_info<2>::element_name == "z"
//          float vec3::element_info<2>::get(vec3* inst) { return inst->z; }
//          void vec3::element_info<2>::set(vec3* inst, float value) { inst->z = value; }
//
//          
//          vec3::element_info<3>::is_enabled                       == false        // This info will be retrieved at invalid indexes
//          vec3::element_info<3>::type                             == void         // This info will be retrieved at invalid indexes
//          vec3::element_info<3>::type_name                        == "ERROR"      // This info will be retrieved at invalid indexes
//          vec3::element_info<3>::element_name                     == "ERROR"      // This info will be retrieved at invalid indexes
//          float vec3::element_info<3>::get(vec3* inst)            == nullptr      // This info will be retrieved at invalid indexes
//          void vec3::element_info<3>::set(vec3* inst, int value)  == nullptr      // This info will be retrieved at invalid indexes


// @NOTE :  set this to 1 if you want to run SOA test
#define AL_SOA_TEST 0

#include <cstddef>
#include <span>

#if AL_SOA_TEST
#   include <iostream> // used for soa_test output
#endif

#define al_declare_reflection(name)                                             \
    typedef name MainType;                                                      \
    template<std::size_t N, typename Dummy = int>                               \
    struct element_info                                                         \
    {                                                                           \
        static constexpr bool is_enabled = false;                               \
        using type = void;                                                      \
        static constexpr const char* type_name = "ERROR";                       \
        static constexpr const char* element_name = "ERROR";                    \
        static constexpr void (*set)(MainType*, int) = nullptr;                 \
        static constexpr type (*get)(MainType*) = nullptr;                      \
    };                                                                          \
    static constexpr std::size_t COUNT_BEGIN = __COUNTER__;                     \
    static constexpr const char* TYPE_NAME = #name;                             \
    template<std::size_t It = 0>                                                \
    static constexpr std::size_t get_number_of_elements(std::size_t size = 0)   \
    {                                                                           \
        std::size_t result = size;                                              \
        if constexpr (element_info<It>::is_enabled)                             \
        {                                                                       \
            result = get_number_of_elements<It + 1>(result + 1);                \
        }                                                                       \
        return result;                                                          \
    }

#define al_reflective(_type, name)                                                          \
    static constexpr std::size_t COUNT_##name = __COUNTER__ - COUNT_BEGIN - 1;              \
    static constexpr void set_##name(MainType* inst, _type value) { inst->name = value; }   \
    static constexpr _type get_##name(MainType* inst) { return inst->name; }                \
    template<typename Dummy> struct element_info<COUNT_##name, Dummy>                       \
    {                                                                                       \
        static constexpr bool is_enabled = true;                                            \
        using type = _type;                                                                 \
        static constexpr const char* type_name = #_type;                                    \
        static constexpr const char* element_name = #name;                                  \
        static constexpr void (*set)(MainType*, type) = set_##name;                         \
        static constexpr type (*get)(MainType*) = get_##name;                               \
    };                                                                                      \
    _type name;

#define al_declare_set_method()                                             \
    template<std::size_t It>                                                \
    static void set(MainType* inst, typename element_info<It>::type value)  \
    {                                                                       \
        element_info<It>::set(inst, value);                                 \
    }

#define al_declare_get_method()                                 \
    template<std::size_t It>                                    \
    static typename element_info<It>::type get(MainType* inst)  \
    {                                                           \
        return element_info<It>::get(inst);                     \
    }

#define al_declare_soa_type()                                                                   \
    template<std::size_t It, typename ... Elements>                                             \
    struct SoaTypeInner : SoaTypeInner<It - 1, typename element_info<It>::type, Elements...>    \
    { };                                                                                        \
    template<typename ... Elements>                                                             \
    struct SoaTypeInner<0, Elements...>                                                         \
    {                                                                                           \
        using type = al::Soa<element_info<0>::type, Elements...>;                               \
    };                                                                                          \
    struct Soa : SoaTypeInner<MainType::get_number_of_elements() - 1>                           \
    { };


namespace al
{
    template<typename ... Elements>
    class Soa
    {
    private:
        template<std::size_t It, typename T, typename ... OtherElements>
        struct NthElementInner : NthElementInner<It - 1, OtherElements...>
        { };

        template<typename T, typename ... OtherElements>
        struct NthElementInner<0, T, OtherElements...>
        {
            using type = T;
        };

        template<std::size_t It, std::size_t CurrentSize, typename T, typename ... OtherElements>
        struct ElementsSize : ElementsSize<It - 1, CurrentSize + sizeof(T), OtherElements...>
        { };

        template<std::size_t CurrentSize, typename T, typename ... OtherElements>
        struct ElementsSize<1, CurrentSize, T, OtherElements...>
        { 
            static constexpr std::size_t SIZE = CurrentSize + sizeof(T);
        };

    public:
        template<std::size_t It>
        struct NthElement : NthElementInner<It, Elements...>
        { };

        Soa()
            : memory{ nullptr }
            , memorySizeBytes{ 0 }
            , numberOfSoaMembers{ 0 }
        { }

        Soa(std::byte* memory, std::size_t memorySizeBytes, std::size_t numberOfSoaMembers)
            : memory{ memory }
            , memorySizeBytes{ memorySizeBytes }
            , numberOfSoaMembers{ numberOfSoaMembers }
        {
            // @TODO : check if memorySizeBytes is enough to store numberOfSoaMembers elements
            set_element_array_pointer(memory);
        }

        ~Soa() = default;

        static constexpr std::size_t calculate_memory_size_for_n_members(std::size_t numberOfMembers)
        {
            return numberOfMembers * SIZE_OF_ELEMENTS;
        }

        template<std::size_t ElementN>
        std::span<typename NthElement<ElementN>::type> get_element_array()
        {
            using Type = typename NthElement<ElementN>::type;
            return std::span<Type>{ reinterpret_cast<Type*>(element_arrays[ElementN]), numberOfSoaMembers };
        }

        template<typename T>
        T get_as(std::size_t index)
        {
            T result;
            get_as_inner(&result, index);
            return result;
        }

        template<typename T>
        void set_as(std::size_t index, T* value)
        {
            set_as_inner(value, index);
        }

    private:
        static constexpr std::size_t NUMBER_OF_ELEMENTS = sizeof...(Elements);
        static constexpr std::size_t SIZE_OF_ELEMENTS = ElementsSize<NUMBER_OF_ELEMENTS, 0, Elements...>::SIZE;

        std::byte* element_arrays[NUMBER_OF_ELEMENTS];
        std::byte* memory;
        std::size_t memorySizeBytes;
        std::size_t numberOfSoaMembers;

        template<std::size_t It = 0>
        void set_element_array_pointer(std::byte* currentPtr)
        {
            if constexpr (It < NUMBER_OF_ELEMENTS)
            {
                element_arrays[It] = currentPtr;
                currentPtr += sizeof(NthElement<It>::type) * numberOfSoaMembers;
                set_element_array_pointer<It + 1>(currentPtr);
            }
        }

        template<typename T, std::size_t It = 0>
        void get_as_inner(T* value, std::size_t index)
        {
            if constexpr (It < NUMBER_OF_ELEMENTS)
            {
                T::set<It>(value, get_element_array<It>()[index]);
                get_as_inner<T, It + 1>(value, index);
            }
        }

        template<typename T, std::size_t It = 0>
        void set_as_inner(T* value, std::size_t index)
        {
            if constexpr (It < NUMBER_OF_ELEMENTS)
            {
                get_element_array<It>()[index] = T::get<It>(value);
                set_as_inner<T, It + 1>(value, index);
            }
        }
    };

#if AL_SOA_TEST
    struct vec3_soa_test
    {
        al_declare_reflection(vec3_soa_test)

        al_reflective(float, x)
        al_reflective(float, y)
        al_reflective(float, z)

        al_declare_set_method()
        al_declare_get_method()

        al_declare_soa_type()
    };

    template<typename T, std::size_t It = 0>
    void soa_test_print_arrays(T* soa)
    {
        if constexpr (It < T::NUMBER_OF_ELEMENTS)
        {
            using ElementT = typename T::NthElement<It>::type;
            for (ElementT element : soa->get_element_array<It>())
            {
                std::cout << element << " ";
            }
            soa_test_print_arrays<T, It + 1>(soa);
        }
    }

    void run_soa_test()
    {
        using vec3_soa = typename vec3_soa_test::Soa::type;

        constexpr std::size_t MEMBERS_NUM = 10;
        std::byte memory[vec3_soa::calculate_memory_size_for_n_members(MEMBERS_NUM)];
        vec3_soa soaInstance{ memory, vec3_soa::calculate_memory_size_for_n_members(MEMBERS_NUM), MEMBERS_NUM };

        constexpr auto res = vec3_soa_test::get_number_of_elements();

        for (std::size_t it = 0; it < MEMBERS_NUM; it++)
        {
            float floatIt = static_cast<float>(it);
            vec3_soa_test v{ floatIt, floatIt / 10.0f, floatIt * 10.0f };
            soaInstance.set_as<vec3_soa_test>(it, &v);
        }

        std::cout << "Print each member of SOA array as vec3 :" << std::endl;
        for (std::size_t it = 0; it < MEMBERS_NUM; it++)
        {
            vec3_soa_test v = soaInstance.get_as<vec3_soa_test>(it);
            std::cout << v.x << " " << v.y << " " << v.z << std::endl;
        }

        std::cout << "Print each element of SOA array as they layed out in memory :" << std::endl;
        soa_test_print_arrays(&soaInstance);
        std::cout << std::endl;
    }
#endif
}

#endif
