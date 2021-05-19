call vcvars64

call cl ^
-Z7 -Od -EHsc -MT ^
user_application\user_application.cpp ^
/std:c++latest /w34996 ^
/I "." /I "%VK_SDK_PATH%\Include" ^
/DAL_DEBUG ^
kernel32.lib user32.lib Gdi32.lib  Ole32.lib ^
%VK_SDK_PATH%\Lib\vulkan-1.lib ^
/link /DEBUG:FULL

call glslangValidator assets\shaders\triangle.vert -o assets\shaders\vert.spv -V -S vert
call glslangValidator assets\shaders\triangle.frag -o assets\shaders\frag.spv -V -S frag
call glslangValidator assets\shaders\test.glsl -o assets\shaders\test.spv -V -S frag

call spirv-dis assets\shaders\vert.spv -o assets\shaders\vert.dis
call spirv-dis assets\shaders\frag.spv -o assets\shaders\frag.dis
call spirv-dis assets\shaders\test.spv -o assets\shaders\test.dis