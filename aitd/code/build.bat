@echo off

echo Compilation started at %date% %time%
echo,


REM set CFLAGS=/std:c11 /W3 /WX /DDEBUG /Zi -wd4996
REM set CFLAGS= /W3 /WX /DDEBUG /EHsc /Zi -wd4996
set CFLAGS= /W3 /WX /EHsc /Zi -wd4996
set LIBS=opengl32.lib glu32.lib user32.lib gdi32.lib

cl %CFLAGS% win32_aitd.cpp %LIBS% /Fe:win32_aitd.exe
REM cl %CFLAGS% obj_to_mdl.c /Fe:obj_to_mdl.exe

echo.
if %errorlevel% equ 0 (
    echo Compilation finished at %date% %time%
) else (
      echo Compilation failed with errors at %date% %time%
)
