#ifndef AL_TOGGLE_H
#define AL_TOGGLE_H

namespace al
{
    // @NOTE : This class is a simple wrapper over two instances
    //         of some type and a boolean toggle state.

    template<typename T>
    class Toggle
    {
    public:
        Toggle()
            : state{ true }
        { }

        void toggle() noexcept
        {
            state = !state;
        }

        T& get_current() noexcept
        {
            return state ? first : second;
        }

        T& get_previous() noexcept
        {
            return state ? second : first;
        }

    private:
        T first;
        T second;
        bool state;
    };
}

#endif
