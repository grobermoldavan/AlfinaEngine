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

for /r %%f in (*.vert) do glslangValidator %%f -o %%f.spv -V -S vert
for /r %%f in (*.frag) do glslangValidator %%f -o %%f.spv -V -S frag
for /r %%f in (*.spv) do spirv-dis %%f -o %%f.dis
