@echo off

echo Compilation started at %date% %time%
echo,

set CommonCompilerFlags=-MT -nologo -fp:fast -Gm- -GR- -EHa- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4244 -wd4996 -wd4456 -FC -Z7 %LuaInclude%

set CommonLinkerFlags=-incremental:no -opt:ref raylib.lib user32.lib gdi32.lib winmm.lib shell32.lib kernel32.lib msvcrt.lib /NODEFAULTLIB:LIBCMT 

cl %CommonCompilerFlags% win32_arwin.cpp /link %CommonLinkerFlags%
REM/Fe:win32_arwin.exe

echo.
if %errorlevel% equ 0 (
                       echo Compilation finished at %date% %time%
                       ) else (
                               echo Compilation failed with errors at %date% %time%
                               )
