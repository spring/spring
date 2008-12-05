#ifndef GL_CONTEXT_H
#define GL_CONTEXT_H
// GLContext.h: interface for the GLContext namespace.
//
//////////////////////////////////////////////////////////////////////

// NOTE:  all GL persistent state should be initialized/freed within
//        a registered set of context functions. This includes the
//        following types of data:
//
//        - Display Lists
//        - Textures / SubTextures
//        - Vertex Buffers
//        - Frame Buffers
//        - Shaders
//
//        The following names are suggested:
//          Member Functions:
//            InitContext()
//            FreeContext()
//          Class Redirectors:
//            StaticInitContext()
//            StaticFreeContext()
//            (use "data" for the pointer to the instance).


// TODO: add event driven system (like alttab, resolution changed, dualscreen, ...)

namespace GLContext
{
	typedef void (*Func)(void* data);

	void Init();
	void Free();
	void InsertHookSet(Func init, Func free, void* data);
	void RemoveHookSet(Func init, Func free, void* data);
};


#endif /* GL_CONTEXT_H */
