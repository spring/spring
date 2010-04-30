CPPFLAGS := -g -W -Wall -Wshadow -I../../rts/System -I../../rts/Rendering -I../../mingwlibs/include/
LIBS = -L../../mingwlibs/lib -lm -lDevIL -lILU -lfreetype -lboost_thread-mt
EXEC = fontTexture
ftSRC = ../../rts/Rendering/FontTexture.cpp ../../rts/System/LogOutput.cpp 
ftOBJ = ../../build/rts/Rendering/FontTexture.o ../../build/rts/System/LogOutput.o 


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
	cp test.png ../../dist/LuaUI/Fonts/test.png            
	cp test.lua ../../dist/LuaUI/Fonts/test.lua     


$(EXEC):	main.o $(ftOBJ)
	g++ $(CPPFLAGS) $(FT_CFLAGS) -g -o $(EXEC) main.o $(ftOBJ) $(FT_LIBS) $(LIBS)


$(ftOBJ):	$(ftSRC)
	g++ $(CPPFLAGS) -c $< -o $(ftOBJ)

main.o:	main.cpp
	g++ $(CPPFLAGS) -c $<
