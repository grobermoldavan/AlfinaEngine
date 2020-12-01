#ifndef AL_DEBUG_OUTPUT_H
#define AL_DEBUG_OUTPUT_H

#include <iostream>
#include <fstream>

namespace al::engine::debug
{
    using DebugOutput = std::ostream;
    using DebugFileOutput = std::ofstream;

    DebugOutput* GLOBAL_LOG_OUTPUT{ &std::cout };
    DebugOutput* GLOBAL_PROFILE_OUTPUT{ &std::cout };

    DebugFileOutput USER_FILE_LOG_OUTPUT;
    DebugFileOutput USER_FILE_PROFILE_OUTPUT;

    void close_log_output()
    {
        try
        {
            if (USER_FILE_LOG_OUTPUT.is_open())
            {
                USER_FILE_LOG_OUTPUT.flush();
                USER_FILE_LOG_OUTPUT.close();
            }
        }
        catch(const std::exception& e) { /* @TODO : handle exception */ }
    }

    void override_log_output(const char* filename)
    {
        close_log_output();

        try
        {
            USER_FILE_LOG_OUTPUT.open(filename, std::ofstream::out | std::ofstream::trunc);
        }
        catch(const std::exception& e) { /* @TODO : handle exception */ }
        GLOBAL_LOG_OUTPUT = &USER_FILE_LOG_OUTPUT;
    }

    void close_profile_output()
    {
        try
        {
            if (USER_FILE_PROFILE_OUTPUT.is_open())
            {
                USER_FILE_PROFILE_OUTPUT.flush();
                USER_FILE_PROFILE_OUTPUT.close();
            }
        }
        catch(const std::exception& e) { /* @TODO : handle exception */ }
    }

    void override_profile_output(const char* filename)
    {
        close_profile_output();

        try
        {
            USER_FILE_PROFILE_OUTPUT.open(filename, std::ofstream::out | std::ofstream::trunc);
        }
        catch(const std::exception& e) { /* @TODO : handle exception */ }
        GLOBAL_PROFILE_OUTPUT = &USER_FILE_PROFILE_OUTPUT;
    }
}

#endif
