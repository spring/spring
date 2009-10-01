What this is
------------

These stubs can be used to run OpenGL applications headlessly, ie without
needing X or a graphics card.

How to use
----------

To use them, instead of linking with libGL, libGLU, and so on, build the stubs into
one or more libraries (eg glstub.c -> libGLstub.a), link with that/those libraries,
and then additionally continue linking with GLEW, but make sure to link AFTER 
linking with the glewstub.c object.

Then, any calls to gl and so on will simply do nothing.

Note that the API is NOT complete, just sufficieint for Spring, so if you use it 
with other applications, you may need to stub out more functions.

Adding new stubs
----------------

Ok, so let's say you link these with your application but it complains that there is
a missing simple SDL_somesymbol, what you do is:

- open the correspondign include, for example if it is SDL, it mmight be /usr/include/SDL
/SDL.h
- search for the appropriate missing function
- copy and paste the function declarataion into the appropriat stub file, eg if it is
an SDL function, paste it into stlstub.c
- add an empty body for it.

So you'll end up with something like:

int SDL_somesymbol(int some param ){
   return 0;
}

Rebuild your application and hopefully it will work now!

