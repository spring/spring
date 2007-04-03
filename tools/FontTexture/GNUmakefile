
CPPFLAGS = -g -W -Wall -Wshadow -I../../rts/System -I../../rts/Rendering

FT_LIBS   = $(shell freetype-config --libs)
CPPFLAGS := $(CPPFLAGS) $(shell freetype-config --cflags)

LIBS = -lm -lIL -lILU

EXEC = fontTexture

all:	$(EXEC) GNUmakefile

clean:
	rm -f *~ *.o $(EXEC) *.png *.fmt

fresh:	clean all

test:	$(EXEC)
	./$(EXEC) -d 1 -f test -o 2 -h 20 ../../game/Vera.ttf
	$(MAKE) copy
	
copy:
	cp test.png ../../game/LuaUI/Fonts/test.png            
	cp test.fmt ../../game/LuaUI/Fonts/test.fmt     

ftSRC = ../../rts/Rendering/FontTexture.cpp
ftOBJ = ../../build/rts/Rendering/FontTexture.o

fontTexture: 	main.o ../../build/rts/Rendering/FontTexture.o
	g++ $(CPPFLAGS) $(FT_CFLAGS) -g -o $(EXEC) main.o $(ftOBJ) $(FT_LIBS) $(LIBS)

$(EXEC):	main.o $(ftOBJ)

ftOBJ:	ftSRC

main.o:	main.cpp
	g++ $(CPPFLAGS) -c $<
