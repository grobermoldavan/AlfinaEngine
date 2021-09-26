#ifndef AL_APPLICATION_SUBSYSTEMS_H
#define AL_APPLICATION_SUBSYSTEMS_H

#include <type_traits>

#include "engine/types.h"

namespace al
{
    template<typename Subsystem, typename ... Other>
    struct ApplicationSubsystems : ApplicationSubsystems<Other...>
    {
        Subsystem subsystem;
        ApplicationSubsystems<Other...> inner;
    };

    template<typename Subsystem>
    struct ApplicationSubsystems<Subsystem>
    {
        Subsystem subsystem;
    };

    template<typename Subsystem, typename Application> struct SubsystemActionConstruct { void operator() (Subsystem* subsystem, Application* application) { construct(subsystem, application); } };
    template<typename Subsystem, typename Application> struct SubsystemActionDestroy   { void operator() (Subsystem* subsystem, Application* application) { destroy(subsystem, application); } };
    template<typename Subsystem, typename Application> struct SubsystemActionUpdate    { void operator() (Subsystem* subsystem, Application* application) { update(subsystem, application); } };
    template<typename Subsystem, typename Application> struct SubsystemActionResize    { void operator() (Subsystem* subsystem, Application* application) { resize(subsystem, application); } };

    template<typename Subsystem, typename Application> void construct(Subsystem*, Application*) { }
    template<typename Subsystem, typename Application> void destroy(Subsystem*, Application*) { }
    template<typename Subsystem, typename Application> void update(Subsystem*, Application*) { }
    template<typename Subsystem, typename Application> void resize(Subsystem*, Application*) { }

    template<template<typename, typename> typename Action, typename Application, typename Subsystem, typename ... Other>
    void for_each_subsystem(ApplicationSubsystems<Subsystem, Other...>* subsystems, Application* application)
    {
        Action<Subsystem, Application> action;
        action(&subsystems->subsystem, application);
        for_each_subsystem<Action>(&subsystems->inner, application);
    }

    template<template<typename, typename> typename Action, typename Application, typename Subsystem, typename ... Other>
    void for_each_subsystem(ApplicationSubsystems<Subsystem>* subsystems, Application* application)
    {
        Action<Subsystem, Application> action;
        action(&subsystems->subsystem, application);
    }

    template<template<typename, typename> typename Action, typename Application, typename Subsystem, typename ... Other>
    void for_each_subsystem_reversed(ApplicationSubsystems<Subsystem, Other...>* subsystems, Application* application)
    {
        for_each_subsystem_reversed<Action>(&subsystems->inner, application);
        Action<Subsystem, Application> action;
        action(&subsystems->subsystem, application);
    }

    template<template<typename, typename> typename Action, typename Application, typename Subsystem, typename ... Other>
    void for_each_subsystem_reversed(ApplicationSubsystems<Subsystem>* subsystems, Application* application)
    {
        Action<Subsystem, Application> action;
        action(&subsystems->subsystem, application);
    }

    template<typename Target, typename Subsystem, typename ... Other>
    Target* try_get_subsystem(ApplicationSubsystems<Subsystem, Other...>* subsystems)
    {
        if constexpr (std::is_same_v<Target, Subsystem>)
        {
            return &subsystems->subsystem;
        }
        else
        {
            return try_get_subsystem<Target>(&subsystems->inner);
        }
    }

    template<typename Target, typename Subsystem>
    Target* try_get_subsystem(ApplicationSubsystems<Subsystem>* subsystems)
    {
        if constexpr (std::is_same_v<Target, Subsystem>)
        {
            return &subsystems->subsystem;
        }
        else
        {
            return nullptr;
        }
    }
}

#endif