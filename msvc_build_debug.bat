call vcvars64

call cl ^
-Z7 -Od -EHsc -MT ^
user_application\user_application.cpp ^
/std:c++latest /w34996 ^
/I "." /I "%VK_SDK_PATH%\Include" ^
/DAL_LOGGING_ENABLED /DAL_PROFILING_ENABLED /DAL_DEBUG ^
kernel32.lib user32.lib Gdi32.lib  Ole32.lib ^
%VK_SDK_PATH%\Lib\vulkan-1.lib ^
/link /DEBUG:FULL

REM call %VK_SDK_PATH%\Bin\glslangValidator.exe assets\shaders\triangle.vert -V
REM call %VK_SDK_PATH%\Bin\glslangValidator.exe assets\shaders\triangle.frag -V

call %VK_SDK_PATH%\Bin\glslc.exe assets\shaders\triangle.vert -o assets\shaders\vert.spv
call %VK_SDK_PATH%\Bin\glslc.exe assets\shaders\triangle.frag -o assets\shaders\frag.spv
