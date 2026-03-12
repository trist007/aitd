MSYS_NO_PATHCONV=1
export MSYS_NO_PATHCONV
SHELL = cmd.exe

CC = cl
CFLAGS = /W3 /WX /EHsc /Zi /wd4996
LIBS = opengl32.lib glu32.lib user32.lib gdi32.lib
OBJS = win32_aitd.obj shared.obj 

win32_aitd.exe: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIBS) /Fe:win32_aitd.exe

%.obj: %.cpp
	$(CC) $(CFLAGS) /c $< /Fo:$@

clean:
	del *.obj *.exe *.pdb
