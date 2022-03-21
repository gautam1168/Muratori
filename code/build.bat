@echo off
mkdir ..\..\build

REM where cl 
REM if %ERRORLEVEL% neq 0 call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

set CompilerFlags=-nologo -GR- -EHa- -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4456 -DHANDMADE_INTERNAL=1 -DHANDMADE_SLOW=1 -FC -Z7
set LinkerFlags=user32.lib Gdi32.lib Winmm.lib

IF NOT EXIST ..\..\build mkdir ..\..\build
pushd ..\..\build
del *.pdb > NUL 2 > NUL
cl %CompilerFlags% C:\Users\gauta\Codes\Muratori\code\handmade.cpp -Fmhandmade.map /LD /link -incremental:no /PDB:hadnmade_%date:~-4,4%%date:~-10,2%%date:~-7,2%%time:~1,1%%time:~3,2%%time:~6,2%.pdb /EXPORT:GameGetSoundSamples /EXPORT:GameUpdateAndRender
cl %CompilerFlags% C:\Users\gauta\Codes\Muratori\code\win32_handmade.cpp /link %LinkerFlags%
popd
