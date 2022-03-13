@echo off
mkdir ..\..\build
@echo off
IF NOT EXIST ..\..\build mkdir ..\..\build
pushd ..\..\build
cl -nologo -GR- -EHa- -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4456 -DHANDMADE_INTERNAL=1 -DHANDMADE_SLOW=1 -FC -Z7 C:\Users\gauta\Codes\Muratori\code\win32_handmade.cpp user32.lib Gdi32.lib Winmm.lib
popd
