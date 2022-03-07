@echo off
mkdir ..\..\build
@echo off
IF NOT EXIST ..\..\build mkdir ..\..\build
pushd ..\..\build
cl -DHANDMADE_SLOW=1 -FC -Zi C:\Users\gauta\Codes\Muratori\code\win32_handmade.cpp user32.lib Gdi32.lib
popd
