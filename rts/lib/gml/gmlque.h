// GML - OpenGL Multithreading Library
// for Spring http://springrts.com
// Author: Mattias "zerver" Radeskog
// (C) Ware Zerver Tech. http://zerver.net
// Ware Zerver Tech. licenses this library
// to be used, distributed and modified 
// freely for any purpose, as long as 
// this notice remains unchanged

#ifndef GMLQUEUE_H
#define GMLQUEUE_H

struct VAdata {
	GLint size;
	GLenum type;
	GLboolean normalized;
	GLsizei stride;
	const GLvoid *pointer;
	GLuint buffer;
	VAdata(){}
	VAdata(GLint si, GLenum ty, GLboolean no, GLsizei st, const GLvoid *po, GLuint buf):
	size(si),type(ty),normalized(no),stride(st),pointer(po),buffer(buf) {}
};

struct VAstruct {
	GLuint target;
	GLint size;
	GLenum type;
	GLboolean normalized;
	GLvoid * pointer;
	GLuint buffer;
	int totalsize;
};


struct gmlQueue {
	std::map<GLuint,VAdata> VAmap;
	std::set<GLuint> VAset;
	
	BYTE *ReadPos;
	BYTE *WritePos;
	BYTE *Pos1;
	BYTE *Pos2;
	
	BYTE *WriteSize;
	BYTE *Size1;
	BYTE *Size2;
	
	BYTE *Read;
	BYTE *Write;
	BYTE *Queue1;
	BYTE *Queue2;
	
	gmlLock Locks1;
	gmlLock Locks2;
	volatile BOOL_ Locked1;
	volatile BOOL_ Locked2;
	
	volatile BOOL_ Reloc;
	BYTE * volatile Sync;
	BOOL_ WasSynced;
	
	GLenum ClientState;
	// VertexPointer
	GLint VPsize;
	GLenum VPtype;
	GLsizei VPstride;
	const GLvoid *VPpointer;
	// ColorPointer
	GLint CPsize;
	GLenum CPtype;
	GLsizei CPstride;
	const GLvoid *CPpointer;
	// EdgeFlagPointer
	GLsizei EFPstride;
	const GLboolean *EFPpointer;
	// IndexPointer
	GLenum IPtype;
	GLsizei IPstride;
	const GLvoid *IPpointer;
	// NormalPointer
	GLenum NPtype;
	GLsizei NPstride;
	const GLvoid *NPpointer;
	// TexCoordPointer
	GLint TCPsize;
	GLenum TCPtype;
	GLsizei TCPstride;
	const GLvoid *TCPpointer;

	GLuint ArrayBuffer;
	GLuint ElementArrayBuffer;
	GLuint PixelPackBuffer;
	GLuint PixelUnpackBuffer;
	
	gmlQueue();
	
	BYTE *Realloc(BYTE **e=NULL);
	BYTE *WaitRealloc(BYTE **e=NULL);
	void ReleaseWrite(BOOL_ final=TRUE);
	BOOL_ GetWrite(BOOL_ critical);
	void ReleaseRead();
	BOOL_ GetRead(BOOL_ critical=FALSE);
	void SyncRequest();
	void Execute();
	void ExecuteSynced(void (gmlQueue::*execfun)() =&gmlQueue::Execute);
	void ExecuteDebug();
};

#endif