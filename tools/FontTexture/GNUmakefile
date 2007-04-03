
CPPFLAGS := -g -W -Wall -Wshadow -I../../rts/System -I../../rts/Rendering

FT_LIBS   = $(shell freetype-config --libs)
CPPFLAGS := $(CPPFLAGS) $(shell freetype-config --cflags)

LIBS = -lm -lIL -lILU

EXEC = fontTexture

ftSRC = ../../rts/Rendering/FontTexture.cpp
ftOBJ = ../../build/rts/Rendering/FontTexture.o


all:	$(EXEC) GNUmakefile

clean:
	rm -f *~ *.o $(EXEC) *.png *.lua

fresh:	clean all
	rm -f $(ftOBJ)
	$(MAKE) clean
	$(MAKE) all

strip:	$(EXEC)
	strip $(EXEC)

test:	$(EXEC)
	./$(EXEC) -d 1 -f test -o 2 -h 20 ../../game/Vera.ttf
	$(MAKE) copy
	
copy:
	cp test.png ../../game/LuaUI/Fonts/test.png            
	cp test.lua ../../game/LuaUI/Fonts/test.lua     


$(EXEC):	main.o $(ftOBJ)
	g++ $(CPPFLAGS) $(FT_CFLAGS) -g -o $(EXEC) main.o $(ftOBJ) $(FT_LIBS) $(LIBS)


$(ftOBJ):	$(ftSRC)
	g++ $(CPPFLAGS) -c $< -o $(ftOBJ)

main.o:	main.cpp
	g++ $(CPPFLAGS) -c $<
