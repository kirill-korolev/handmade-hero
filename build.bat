@echo off
setlocal

set BUILD=build
set BIN=%BUILD%\bin
set INTERMEDIATES=%BUILD%\intermediates
set LIBS=user32.lib gdi32.lib
set EXE=handmadehero.exe
set SRC=src\win32.cpp

pushd %HANDMADE_HERO_ROOT%
mkdir %BIN% 2> NUL
mkdir %INTERMEDIATES% 2> NUL
cl %SRC% /Zi /Fo%INTERMEDIATES%\ /link %LIBS% /out:%BIN%\%EXE%
popd

endlocal