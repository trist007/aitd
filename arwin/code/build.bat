@echo off

echo Compilation started at %date% %time%
echo,

REM -MT static builds for raylib.lib
REM -MD shared lib builds for raylib.dll

set CommonCompilerFlags=/DEBUG -MD -nologo -fp:fast -Gm- -GR- -EHa- -Od -Oi -WX -W4 -FC -Z7 ^
-wd4201 -wd4100 -wd4189 -wd4244 -wd4996 -wd4456 ^
-DRAYLIB_DLL

set CommonLinkerFlags=-incremental:no -opt:ref /PDB:win32_arwin.pdb ^
..\code\raylib.lib ^
user32.lib gdi32.lib winmm.lib shell32.lib kernel32.lib msvcrt.lib /NODEFAULTLIB:LIBCMT /NODEFAULTLIB:MSVCRTD

REM echo Updating etags
REM echo,
REM etags *.cpp *.h raylib\*.c

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

REM delete pdb because debugger maintains a lock on pdb so pdb cannot be overwritten
del *.pdb > NUL 2> NUL

echo Compiling...
cl %CommonCompilerFlags% ^
..\code\win32_arwin.c ^
..\code\arwin.c ^
/link %CommonLinkerFlags% /out:win32_arwin.exe"

copy ..\code\raylib.dll .
copy ..\code\raylib.pdb .

popd

echo.
if %errorlevel% equ 0 (
                       echo Compilation finished at %date% %time%
                       ) else (
                               echo Compilation failed with errors at %date% %time%
                               )
