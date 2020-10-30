#define AL_PLATFORM_WIN32
#define AL_DEBUG

#define RUN_TESTS 1

#include "engine/engine.h"

#include "utilities/function.h"

class UserApplication : public al::engine::AlfinaEngineApplication
{
    virtual void InitializeComponents() override;
    virtual void TerminateComponents() override;
    virtual void Run() override;
};

UserApplication applicationInstance;

al::engine::AlfinaEngineApplication* al::engine::create_application(al::engine::CommandLineParams commandLine)
{
    printf("Creating.\n");
    return &applicationInstance;
}

void al::engine::destroy_application(al::engine::AlfinaEngineApplication* application)
{
    printf("Destroying.\n");
}

void UserApplication::InitializeComponents()
{
    printf("Initializing.\n");
    al::engine::AlfinaEngineApplication::InitializeComponents();
}

void UserApplication::TerminateComponents()
{
    printf("Terminating.\n");
    al::engine::AlfinaEngineApplication::TerminateComponents();
}

#if RUN_TESTS
void job_func(al::engine::Job*)
{
    printf("job from %d\n", std::this_thread::get_id());
    std::this_thread::sleep_for(std::chrono::milliseconds{ 100 });
}

struct Inv
{
    void operator () ()
    { 
        printf("operator () overload in a struct\n");
    }
};

void func()
{
    printf("plain old function\n");
}

void function_that_takes_function(al::Function<void()> func)
{
    func();
}
#endif

void UserApplication::Run()
{
    printf("Running.\n");
    al::engine::AlfinaEngineApplication::Run();

#if RUN_TESTS
    {
        Inv inv;
        al::Function<void()> func1{ []() { printf("lambda function\n"); } };
        al::Function<void()> func2{ inv };
        al::Function<void()> func3{ func };
        al::Function<void()> func4{ func1 };
        al::Function<void()> func5{ func2 };
        al::Function<void()> func6{ func3 };
        printf(" ::::::::::::::::::: Calling function objects\n");
        func1();
        func2();
        func3();
        func4();
        func5();
        func6();
        printf(" ::::::::::::::::::: Passing function object to a function\n");
        function_that_takes_function([]() { printf("lambda function\n"); });
        function_that_takes_function(inv);
        function_that_takes_function(func);
        function_that_takes_function(func1);
        function_that_takes_function(func2);
        function_that_takes_function(func3);
        function_that_takes_function(func4);
        function_that_takes_function(func5);
        function_that_takes_function(func6);
        printf(" ::::::::::::::::::: Assigning functions and calling them again\n");
        func1 = []() { printf("lambda function2\n"); };
        func2 = inv;
        func3 = func;
        func4 = func1;
        func5 = func2;
        func6 = func3;
        func1();
        func2();
        func3();
        func4();
        func5();
        func6();
    }

    {
        printf(" ::::::::::::::::::: Job system test\n");
        al::engine::Job jobs[100];
        auto* root = &jobs[0];
        ::new(root) al::engine::Job{ job_func };
        jobSystem->add_job(root);
        for (int i = 1; i < 100; i++)
        {
            auto* job = &jobs[i];
            ::new(job) al::engine::Job{ job_func, root };
            jobSystem->add_job(job);
        }
        jobSystem->wait_for(root);
        printf("Finished running.\n");
    }
#endif
}
