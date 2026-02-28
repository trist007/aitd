@echo off

set CFLAGS=/std:c11 /W3 /WX /DDEBUG /Zi
set LIBS=opengl32.lib glu32.lib user32.lib gdi32.lib

cl %CFLAGS% win32_aitd.c %LIBS% /Fe:win32_aitd.exe
cl %CFLAGS% obj_to_mdl.c /Fe:obj_to_mdl.exe

if %ERRORLEVEL% == 0 (
                      echo Build OK
                      ) else (
                              echo Build FAILED
                              )