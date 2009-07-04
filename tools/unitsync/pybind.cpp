/* Author: Tobi Vollebregt */
/* Python bindings for unitsync library */

// do not use debug Python since that causes link errors
// need a different set of Python libs for that
#if defined(WIN32) && defined(_DEBUG)
#undef _DEBUG
#include <Python.h>
#define _DEBUG
#else
#include <Python.h>
#endif

#include <cstring>
#include "unitsync.h"
#include "Rendering/Textures/Bitmap.h"
#include "FileSystem/FileSystem.h"
#include "exportdefines.h"

#define NAMEBUF_SIZE 4096



EXPORT(const char* ) GetSpringVersion();
EXPORT(void        ) UnInit();
EXPORT(int         ) ProcessUnits(void);
EXPORT(int         ) ProcessUnitsNoChecksum(void);
EXPORT(int         ) GetUnitCount();
EXPORT(const char* ) GetUnitName(int unit);
EXPORT(const char* ) GetFullUnitName(int unit);
EXPORT(void        ) AddArchive(const char* name);
EXPORT(void        ) AddAllArchives(const char* root);
EXPORT(unsigned int) GetArchiveChecksum(const char* arname);
EXPORT(int         ) GetMapCount();
EXPORT(const char* ) GetMapName(int index);
EXPORT(int         ) GetMapInfoEx(const char* name, MapInfo* outInfo, int version);
EXPORT(int         ) GetMapInfo(const char* name, MapInfo* outInfo);
EXPORT(void*       ) GetMinimap(const char* filename, int miplevel);
EXPORT(int         ) GetMapArchiveCount(const char* mapName);
EXPORT(const char* ) GetMapArchiveName(int index);
EXPORT(unsigned int) GetMapChecksum(int index);
EXPORT(int         ) GetPrimaryModCount();
EXPORT(const char* ) GetPrimaryModName(int index);
EXPORT(const char* ) GetPrimaryModArchive(int index);
EXPORT(int         ) GetPrimaryModArchiveCount(int index);
EXPORT(const char* ) GetPrimaryModArchiveList(int arnr);
EXPORT(int         ) GetPrimaryModIndex(const char* name);
EXPORT(unsigned int) GetPrimaryModChecksum(int index);
EXPORT(int         ) GetSideCount();
EXPORT(const char* ) GetSideName(int side);
EXPORT(int         ) OpenFileVFS(const char* name);
EXPORT(void        ) CloseFileVFS(int handle);
EXPORT(void        ) ReadFileVFS(int handle, void* buf, int length);
EXPORT(int         ) FileSizeVFS(int handle);
EXPORT(int         ) InitFindVFS(const char* pattern);
EXPORT(int         ) FindFilesVFS(int handle, char* nameBuf, int size);
EXPORT(int         ) OpenArchive(const char* name);
EXPORT(void        ) CloseArchive(int archive);
EXPORT(int         ) FindFilesArchive(int archive, int cur, char* nameBuf, int* size);
EXPORT(int         ) OpenArchiveFile(int archive, const char* name);
EXPORT(int         ) ReadArchiveFile(int archive, int handle, void* buffer, int numBytes);
EXPORT(void        ) CloseArchiveFile(int archive, int handle);
EXPORT(int         ) SizeArchiveFile(int archive, int handle);



static PyObject *unitsync_GetSpringVersion(PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	return Py_BuildValue("s", GetSpringVersion());
}

static PyObject *unitsync_Init(PyObject *self, PyObject *args)
{
	int isServer;
	int id;
	if (!PyArg_ParseTuple(args, "ii", &isServer, &id))
		return NULL;
	return Py_BuildValue("i", 1);
}

static PyObject *unitsync_UnInit(PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	UnInit();
	return Py_BuildValue("");
}

static PyObject *unitsync_ProcessUnits(PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	return Py_BuildValue("i", ProcessUnits());
}

static PyObject *unitsync_ProcessUnitsNoChecksum(PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	return Py_BuildValue("i", ProcessUnitsNoChecksum());
}

static PyObject *unitsync_GetUnitCount(PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	return Py_BuildValue("i", GetUnitCount());
}

static PyObject *unitsync_GetUnitName(PyObject *self, PyObject *args)
{
	int unit;
	if (!PyArg_ParseTuple(args, "i", &unit))
		return NULL;
	return Py_BuildValue("s", GetUnitName(unit));
}

static PyObject *unitsync_GetFullUnitName(PyObject *self, PyObject *args)
{
	int unit;
	if (!PyArg_ParseTuple(args, "i", &unit))
		return NULL;
	return Py_BuildValue("s", GetFullUnitName(unit));
}

static PyObject *unitsync_AddArchive(PyObject *self, PyObject *args)
{
	const char* name;
	if (!PyArg_ParseTuple(args, "s", &name))
		return NULL;
	AddArchive(name);
	return Py_BuildValue("");
}

static PyObject *unitsync_AddAllArchives(PyObject *self, PyObject *args)
{
	const char* root;
	if (!PyArg_ParseTuple(args, "s", &root))
		return NULL;
	AddAllArchives(root);
	return Py_BuildValue("");
}

static PyObject *unitsync_GetArchiveChecksum(PyObject *self, PyObject *args)
{
	const char* arname;
	if (!PyArg_ParseTuple(args, "s", &arname))
		return NULL;
	return Py_BuildValue("i", GetArchiveChecksum(arname));
}

static PyObject *unitsync_GetMapCount(PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	return Py_BuildValue("i", GetMapCount());
}

static PyObject *unitsync_GetMapName(PyObject *self, PyObject *args)
{
	int index;
	if (!PyArg_ParseTuple(args, "i", &index))
		return NULL;
	return Py_BuildValue("s", GetMapName(index));
}

static PyObject *unitsync_GetMapInfo(PyObject *self, PyObject *args)
{
	const char* name;
	if (!PyArg_ParseTuple(args, "s", &name))
		return NULL;
	MapInfo out;
	char buf[256];
	// Zero the memory (returning garbage to python seems so unprofessional ;-)
	std::memset(&out, 0, sizeof(out));
	std::memset(buf, 0, sizeof(buf));
	// Apparently unitsync assumes out.description points to a valid buffer in all/some cases.
	out.description = buf;
	char author[200];
	std::memset(author, 0, sizeof(author));
	out.author = author;
	int ret = GetMapInfoEx(name, &out, 1);
	// HACK   :/
	return Py_BuildValue("i{s:s,s:i,s:i,s:f,s:i,s:i,s:i,s:i,s:i,s:i,s:s,s:[(ii)(ii)(ii)(ii)(ii)(ii)(ii)(ii)(ii)(ii)(ii)(ii)(ii)(ii)(ii)(ii)]}",
						 ret,
						 "description",     out.description,
						 "tidalStrength",   out.tidalStrength,
						 "gravity",         out.gravity,
						 "maxMetal",        out.maxMetal,
						 "extractorRadius", out.extractorRadius,
						 "minWind",         out.minWind,
						 "maxWind",         out.maxWind,
						 "width",           out.width,
						 "height",          out.height,
						 "posCount",        out.posCount,
						 "author",			out.author,
						 "positions",
						 out.positions[0].x,  out.positions[0].z,
						 out.positions[1].x,  out.positions[1].z,
						 out.positions[2].x,  out.positions[2].z,
						 out.positions[3].x,  out.positions[3].z,
						 out.positions[4].x,  out.positions[4].z,
						 out.positions[5].x,  out.positions[5].z,
						 out.positions[6].x,  out.positions[6].z,
						 out.positions[7].x,  out.positions[7].z,
						 out.positions[8].x,  out.positions[8].z,
						 out.positions[9].x,  out.positions[9].z,
						 out.positions[10].x, out.positions[10].z,
						 out.positions[11].x, out.positions[11].z,
						 out.positions[12].x, out.positions[12].z,
						 out.positions[13].x, out.positions[13].z,
						 out.positions[14].x, out.positions[14].z,
						 out.positions[15].x, out.positions[15].z
						);
}

#define RM	0x0000F800
#define GM  0x000007E0
#define BM  0x0000001F

#define RED_RGB565(x) ((x&RM)>>11)
#define GREEN_RGB565(x) ((x&GM)>>5)
#define BLUE_RGB565(x) (x&BM)
#define PACKRGB(r, g, b) (((r<<11)&RM) | ((g << 5)&GM) | (b&BM) )

static PyObject *unitsync_GetMinimap(PyObject *self, PyObject *args)
{
	const char* bitmap_filename;
	const char* filename;
	int miplevel;
	if (!PyArg_ParseTuple(args, "sis", &filename, &miplevel, &bitmap_filename))
		return NULL;
	void* minimap = GetMinimap(filename, miplevel);
	if (!minimap)
		return Py_BuildValue(""); // return failure
	int size = 1024 >> miplevel;
	CBitmap bm;
	bm.Alloc(size, size);
	unsigned short *src = (unsigned short*)minimap;
	unsigned char *dst = bm.mem;
	for (int y = 0; y < size; y++) {
		for (int x = 0; x < size; x++)
		{
			dst[0] = RED_RGB565   ((*src)) << 3;
			dst[1] = GREEN_RGB565 ((*src)) << 2;
			dst[2] = BLUE_RGB565  ((*src)) << 3;
			dst[3] = 255;
			++src;
			dst += 4;
		}
	}
	remove(bitmap_filename); //somehow overwriting doesn't work??
	bm.Save(bitmap_filename);
	// check whether the bm.Save succeeded?
	FILE* f = fopen(bitmap_filename, "rb");
	bool success = !!f;
	if (success) {
		fclose(f);
		return Py_BuildValue("s", bitmap_filename); // return success
	}
	return Py_BuildValue(""); // return failure
}

static PyObject *unitsync_GetMapArchiveCount(PyObject *self, PyObject *args)
{
	const char* filename;
	if (!PyArg_ParseTuple(args, "s", &filename))
		return NULL;
	return Py_BuildValue("i", GetMapArchiveCount(filename));
}

static PyObject *unitsync_GetMapArchiveName(PyObject *self, PyObject *args)
{
	int index;
	if (!PyArg_ParseTuple(args, "i", &index))
		return NULL;
	return Py_BuildValue("s", GetMapArchiveName(index));
}

static PyObject *unitsync_GetMapChecksum(PyObject *self, PyObject *args)
{
	int index;
	if (!PyArg_ParseTuple(args, "i", &index))
		return NULL;
	return Py_BuildValue("i", GetMapChecksum(index));
}

static PyObject *unitsync_GetPrimaryModCount(PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	return Py_BuildValue("i", GetPrimaryModCount());
}

static PyObject *unitsync_GetPrimaryModName(PyObject *self, PyObject *args)
{
	int index;
	if (!PyArg_ParseTuple(args, "i", &index))
		return NULL;
	return Py_BuildValue("s", GetPrimaryModName(index));
}

static PyObject *unitsync_GetPrimaryModArchive(PyObject *self, PyObject *args)
{
	int index;
	if (!PyArg_ParseTuple(args, "i", &index))
		return NULL;
	return Py_BuildValue("s", GetPrimaryModArchive(index));
}

static PyObject *unitsync_GetPrimaryModArchiveCount(PyObject *self, PyObject *args)
{
	int index;
	if (!PyArg_ParseTuple(args, "i", &index))
		return NULL;
	return Py_BuildValue("i", GetPrimaryModArchiveCount(index));
}

static PyObject *unitsync_GetPrimaryModArchiveList(PyObject *self, PyObject *args)
{
	int index;
	if (!PyArg_ParseTuple(args, "i", &index))
		return NULL;
	return Py_BuildValue("s", GetPrimaryModArchiveList(index));
}

static PyObject *unitsync_GetPrimaryModIndex(PyObject *self, PyObject *args)
{
	const char* name;
	if (!PyArg_ParseTuple(args, "s", &name))
		return NULL;
	return Py_BuildValue("i", GetPrimaryModIndex(name));
}

static PyObject *unitsync_GetPrimaryModChecksum(PyObject *self, PyObject *args)
{
	int index;
	if (!PyArg_ParseTuple(args, "i", &index))
		return NULL;
	return Py_BuildValue("i", GetPrimaryModChecksum(index));
}

static PyObject *unitsync_GetSideCount(PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	return Py_BuildValue("i", GetSideCount());
}

static PyObject *unitsync_GetSideName(PyObject *self, PyObject *args)
{
	int side;
	if (!PyArg_ParseTuple(args, "i", &side))
		return NULL;
	return Py_BuildValue("s", GetSideName(side));
}

static PyObject *unitsync_OpenFileVFS(PyObject *self, PyObject *args)
{
	const char* name;
	if (!PyArg_ParseTuple(args, "s", &name))
		return NULL;
	return Py_BuildValue("i", OpenFileVFS(name));
}

static PyObject *unitsync_CloseFileVFS(PyObject *self, PyObject *args)
{
	int handle;
	if (!PyArg_ParseTuple(args, "i", &handle))
		return NULL;
	CloseFileVFS(handle);
	return Py_BuildValue("");
}

static PyObject *unitsync_ReadFileVFS(PyObject *self, PyObject *args)
{
	// TODO   fix this for binary files
	int handle;
	int length;
	if (!PyArg_ParseTuple(args, "ii", &handle, &length))
		return NULL;
	char* buf = static_cast<char*>(malloc(length + 1));
	ReadFileVFS(handle, buf, length);
	buf[length] = 0;
	PyObject* ret = Py_BuildValue("s", buf);
	free(buf);
	return ret;
}

static PyObject *unitsync_FileSizeVFS(PyObject *self, PyObject *args)
{
	int handle;
	if (!PyArg_ParseTuple(args, "i", &handle))
		return NULL;
	return Py_BuildValue("i", FileSizeVFS(handle));
}

static PyObject *unitsync_InitFindVFS(PyObject *self, PyObject *args)
{
	const char* pattern;
	if (!PyArg_ParseTuple(args, "s", &pattern))
		return NULL;
	return Py_BuildValue("i", InitFindVFS(pattern));
}

static PyObject *unitsync_FindFilesVFS(PyObject *self, PyObject *args)
{
	int handle;
	if (!PyArg_ParseTuple(args, "i", &handle))
		return NULL;
	char nameBuf[NAMEBUF_SIZE] = "";
	int ret = FindFilesVFS(handle, nameBuf, NAMEBUF_SIZE);
	return Py_BuildValue("is", ret, nameBuf); // return (new_handle, name)
}

static PyObject *unitsync_OpenArchive(PyObject *self, PyObject *args)
{
	const char* name;
	if (!PyArg_ParseTuple(args, "s", &name))
		return NULL;
	return Py_BuildValue("i", OpenArchive(name));
}

static PyObject *unitsync_CloseArchive(PyObject *self, PyObject *args)
{
	int archive;
	if (!PyArg_ParseTuple(args, "i", &archive))
		return NULL;
	CloseArchive(archive);
	return Py_BuildValue("");
}

static PyObject *unitsync_FindFilesArchive(PyObject *self, PyObject *args)
{
	int archive;
	int cur;
	if (!PyArg_ParseTuple(args, "ii", &archive, &cur))
		return NULL;
	// TODO   fix buffer overflow (strcpy in FindFilesArchive)
	char nameBuf[NAMEBUF_SIZE] = "";
	int size;
	int ret = FindFilesArchive(archive, cur, nameBuf, &size);
	return Py_BuildValue("isi", ret, nameBuf, size); // return tuple (new_handle, name, size)
}

static PyObject *unitsync_OpenArchiveFile(PyObject *self, PyObject *args)
{
	int archive;
	const char* name;
	if (!PyArg_ParseTuple(args, "is", &archive, &name))
		return NULL;
	return Py_BuildValue("i", OpenArchiveFile(archive, name));
}

static PyObject *unitsync_ReadArchiveFile(PyObject *self, PyObject *args)
{
	// TODO   fix this for binary files
	int archive;
	int handle;
	int numBytes;
	if (!PyArg_ParseTuple(args, "iii", &archive, &handle, &numBytes))
		return NULL;
	char* buffer = static_cast<char*>(malloc(numBytes + 1));
	ReadArchiveFile(archive, handle, buffer, numBytes);
	buffer[numBytes] = 0;
	PyObject* ret = Py_BuildValue("s", buffer);
	free(buffer);
	return ret;
}

static PyObject *unitsync_CloseArchiveFile(PyObject *self, PyObject *args)
{
	int archive;
	int handle;
	if (!PyArg_ParseTuple(args, "ii", &archive, &handle))
		return NULL;
	CloseArchiveFile(archive, handle);
	return Py_BuildValue("");
}

static PyObject *unitsync_SizeArchiveFile(PyObject *self, PyObject *args)
{
	int archive;
	int handle;
	if (!PyArg_ParseTuple(args, "ii", &archive, &handle))
		return NULL;
	return Py_BuildValue("i", SizeArchiveFile(archive, handle));
}

////////////////////////////////////////////////////////////
////////// Functions not in other bindings (TODO)

// GetWritableDataDirectory implements half of this now in C interface

static PyObject *unitsync_GetDataDirectories(PyObject *self, PyObject *args)
{
	// unitsync.GetDataDirectories(bool write = False)
	int write = 0;
	if (/*!PyArg_ParseTuple(args, "") &&*/ !PyArg_ParseTuple(args, "i", &write))
		return NULL;
	std::vector<std::string> f;
	if (write)
		f.push_back(FileSystemHandler::GetInstance().GetWriteDir());
	else
		f = FileSystemHandler::GetInstance().GetDataDirectories();
	PyObject *ret = PyList_New(f.size());
	for (unsigned i = 0; i < f.size(); ++i) {
		PyList_SET_ITEM(ret, i, PyString_FromString(f[i].c_str()));
	}
	return ret;
}


// Note to self: sed "s/([^)]*);/ ),/g;s/[ \t]*EXPORT\([^\)]\+\) /PY( /g;"
// TODO   function documentation (last column)
static PyMethodDef unitsyncMethods[] = {
#define PY(name)         { # name , unitsync_ ## name , METH_VARARGS , NULL }
#define PYDOC(name, doc) { # name , unitsync_ ## name , METH_VARARGS , doc  }
	PY( GetSpringVersion ),
	PY( Init ),
	PY( UnInit ),
	PY( ProcessUnits ),
	PY( ProcessUnitsNoChecksum ),
	PY( GetUnitCount ),
	PY( GetUnitName ),
	PY( GetFullUnitName ),
	PY( AddArchive ),
	PY( AddAllArchives ),
	PY( GetArchiveChecksum ),
	PYDOC( GetMapCount, "GetMapCount() -- Updates the list of locally available maps and returns the number of maps.\n"
			"InitArchiveScanner must be called before this function."),
	PYDOC( GetMapName, "GetMapName(n) -- Returns the name of the nth map, e.g. 'SmallDivide.smf'.\n"
			"GetMapCount must be called before this function."),
	PYDOC( GetMapInfo, "GetMapInfo(mapName) -- Returns info about mapName.\n"
			"The return value is a tuple of two values: an integer and a map. The integer\n"
			"is 0 on failure, 1 on succes. The map looks like the following example:\n"
			"{'posCount': 6, 'extractorRadius': 400, 'maxMetal': 0.029999999329447746, 'positions': [(600, 600), (3600, 3600), (3600, 600), (600, 3600), (2000, 600), (2000, 3600), ...], 'maxWind': 20, 'gravity': 130, 'height': 4096, 'minWind': 5, 'tidalStrength': 20, 'width': 4096, 'description': 'Lot of metal in middle but look out for kbot walking over the mountains'}\n"
		 	"InitArchiveScanner must be called before this function."),
	PYDOC( GetMinimap, "GetMinimap(mapName, mipLevel, bitmapFilename) -- Write minimap image of mapName to bitmapFilename.\n"
			"The width and height of the saved image is 1024 >> mipLevel (that is: 1024 / 2^mipLevel). Any format supported by DevIL is supported by this function." ),
	PY( GetMapArchiveCount ),
	PY( GetMapArchiveName ),
	PY( GetMapChecksum ),
	PYDOC( GetPrimaryModCount, "GetPrimaryModCount() -- Updates the list of locally available mods and returns the number of mods.\n"
			"InitArchiveScanner must be called before this function."),
	PYDOC( GetPrimaryModName, "GetPrimaryModName(n) -- Returns the name of the nth mod, e.g. 'XTA v0.66 Pimped Edition V3'.\n"
		 	"GetPrimaryModCount must be called before this function."),
	PYDOC( GetPrimaryModArchive, "GetPrimaryModArchive(n) -- Returns the archive of the nth mod, e.g. 'xtapev3.sd7'.\n"
			"GetPrimaryModCount must be called before this function."),
	PYDOC( GetPrimaryModArchiveCount, "GetPrimaryModArchiveCount(n) -- Updates the list of archives the nth mod depends on.\n"
			"GetPrimaryModCount must be called before this function."),
	PYDOC( GetPrimaryModArchiveList, "GetPrimaryModArchiveList(n) -- Returns the nth archive of the selected mod.\n"
			"GetPrimaryModArchiveCount must be called before this function to select a mod."),
	PYDOC( GetPrimaryModIndex, "GetPrimaryModIndex(modName) -- Returns the index of modName or -1 if it can't be found.\n"
			"GetPrimaryModIndex(GetPrimaryModName(n)) == n for n < GetPrimaryModCount()\n"
			"GetPrimaryModCount must be called before this function."),
	PY( GetPrimaryModChecksum ),
	PY( GetSideCount ),
	PY( GetSideName ),
	PY( OpenFileVFS ),
	PY( CloseFileVFS ),
	PY( ReadFileVFS ),
	PY( FileSizeVFS ),
	PY( InitFindVFS ),
	PY( FindFilesVFS ),
	PY( OpenArchive ),
	PY( CloseArchive ),
	PY( FindFilesArchive ),
	PY( OpenArchiveFile ),
	PY( ReadArchiveFile ),
	PY( CloseArchiveFile ),
	PY( SizeArchiveFile ),
	PY( GetDataDirectories ),
	{NULL, NULL, 0, NULL}        /* Sentinel */
#undef PYDOC
#undef PY
};

// PyMODINIT_FUNC already includes the SHARED_EXPORT equivalent
// and the right calling convention for the host platform.
#ifdef _MSC_VER
PyMODINIT_FUNC initunitsync(void)
#else
PyMODINIT_FUNC __attribute__ ((visibility("default"))) initunitsync(void)
#endif
{
	(void) Py_InitModule("unitsync", unitsyncMethods);
}

