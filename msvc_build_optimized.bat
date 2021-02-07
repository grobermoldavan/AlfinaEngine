call vcvars64

call cl ^
-O2 -EHsc -MT ^
user_application\user_application.cpp ^
/std:c++latest /w34996 ^
/I "." /I "engine\3d_party_libs\glew\include" ^
kernel32.lib user32.lib Gdi32.lib Opengl32.lib Ole32.lib ^
engine\3d_party_libs\glew\lib\Release\x64\glew32s.lib ^
/link /DEBUG:NONE

REM @TODO : currently this is crashing because program tries to load assets from
REM         assets folder directly and crashes because can't find it

REM if not exist ".\msvc_build\" mkdir msvc_build
REM move /y .\user_application.exe .\msvc_build\
REM move /y .\user_application.ilk .\msvc_build\
REM move /y .\user_application.obj .\msvc_build\
REM move /y .\user_application.pdb .\msvc_build\

pause
