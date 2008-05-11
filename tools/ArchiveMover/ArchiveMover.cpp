/******************************************************************************/
/******************************************************************************/
//
//  file:     ArchiveMover.cpp
//  authors:  zizu
//            Dave Rodgers  (aka: trepan)
//            AF (wrote the original java version)
//            Tobi Vollebregt (Linux port; refactors)
//  date:     June 29, 2007
//  desc:     Utility to classify and potentially move Spring
//            archive files to their installation directories
//

#include <cstdio>
#include <cctype>
#include <cassert>
#include <cstring>
#include <string>
#include <sstream>
#include <iostream>
#include <exception>
#include <algorithm>
using std::endl;


//#define ARCHIVE_MOVER_USE_WIN_API

#ifdef ARCHIVE_MOVER_USE_WIN_API
  #ifndef NOMINMAX
    #define NOMINMAX
  #endif
  #include <windows.h>
#endif

#ifdef _MSC_VER
  #ifdef ARCHIVE_MOVER_USE_WIN_API
    #pragma comment(linker, "/SUBSYSTEM:WINDOWS")
  #else
    #pragma comment(linker, "/SUBSYSTEM:CONSOLE")
  #endif
#endif


#include <boost/shared_ptr.hpp>
#include <boost/scoped_array.hpp>
#include <boost/static_assert.hpp>
#include <boost/version.hpp>


BOOST_STATIC_ASSERT(BOOST_VERSION >= 103400);
#include <boost/filesystem.hpp>
#include <boost/filesystem/cerrno.hpp>
#include <boost/filesystem/fstream.hpp>
namespace fs = boost::filesystem;


#include "lib/minizip/unzip.h"
extern "C" {
#include "lib/7zip/7zIn.h"
#include "lib/7zip/7zCrc.h"
#include "lib/7zip/7zExtract.h"
}


#if defined(WIN32) && (defined(UNICODE) || defined(_UNICODE))

	typedef fs::wpath Path;
	typedef fs::filesystem_wpath_error FilesystemPathError;

	typedef wchar_t Char;
	typedef std::wstring String;
	typedef std::wstringstream StringStream;
	std::wostream& cout = std::wcout;

	// Untested with MSVC (works fine with GCC).
	#define _(s) L ## s
	#define fopen _wfopen

#else

	// fs::wpath segfaults on Linux when doing something simple like:
	//		Path p = fs::current_path<Path>();
	// (this is used by fs::system_complete() and others ...)
	typedef fs::path Path;
	typedef fs::filesystem_path_error FilesystemPathError;

	// And converting stuff back and forth between wide chars and narrow chars
	// is a PITA so we just define String to normal string too.
	// (Accented chars are for nubs anyway, in particular in filenames ;-))
	typedef char Char;
	typedef std::string String;
	typedef std::stringstream StringStream;
	std::ostream& cout = std::cout;

	#define _(s) s

#endif


//**************************************************************************************

#define NEW_LIFETIME_MANAGER_NAME(type, name) \
class name##_del{                             \
    void releaseType(type* ptr);              \
public:                                       \
    void operator()(type* ptr){               \
        if(!ptr){                             \
            return;                           \
        }                                     \
        this->releaseType(ptr);               \
        delete ptr;                           \
    }                                         \
};                                            \
typedef boost::shared_ptr<type> name##_ptr;   \
void name##_del::releaseType(type* ptr)


#define NEW_LIFETIME_MANAGER(type) NEW_LIFETIME_MANAGER_NAME(type,type)




NEW_LIFETIME_MANAGER(unzFile){
	if(*ptr){
		unzClose(*ptr);
	}
}

unzFile_ptr unzOpen2_p(zlib_filefunc_def* pzlib_filefunc_def){
	return unzFile_ptr(new unzFile(unzOpen2(0, pzlib_filefunc_def)), unzFile_del());
}



NEW_LIFETIME_MANAGER(CArchiveDatabaseEx){
	SzArDbExFree(ptr, SzFree);
}

CArchiveDatabaseEx_ptr SzArDbExInit_p(){
	CArchiveDatabaseEx_ptr local_ptr(new CArchiveDatabaseEx, CArchiveDatabaseEx_del());
	SzArDbExInit(local_ptr.get());
	return local_ptr;
}



NEW_LIFETIME_MANAGER_NAME(unsigned char*, SzOutBuffer){
	if(*ptr){
		SzFree(*ptr);
	}
}

SzOutBuffer_ptr SzOutBuffer_p(unsigned char* arg){
	return SzOutBuffer_ptr(new unsigned char *(arg), SzOutBuffer_del());
}


#undef NEW_LIFETIME_MANAGER_NAME
#undef NEW_LIFETIME_MANAGER

//******************************************************************************


enum ArchiveType {
	R_MOD,
	R_MAP,
	R_OTHER
};

static int run(int argc, const Char* const* argv);
static ArchiveType read_archive_content_sd7(const Path& filename);
static ArchiveType read_archive_content_sdz(const Path& filename);


static std::string string_to_lower(const std::string& s){

	std::string l = s;
	std::transform(l.begin(), l.end(), l.begin(), static_cast<int (*)(int)>(tolower));
	return l;
}


struct ScopedMessage : public StringStream{
	~ScopedMessage(){
		String str = this->str();
		if (!str.empty()) {
#ifdef ARCHIVE_MOVER_USE_WIN_API
			MessageBoxW(NULL, str.c_str(), _("Spring - ArchiveMover"), MB_APPLMODAL);
#else
			cout<<str<<endl;
#endif
		}
	}
};

static bool is_map(const std::string& filename){
	if (filename.substr(0, 4) == "maps") {
		const std::string ext = filename.substr(filename.size() - 4);
		if ((ext == ".sm3") || (ext == ".smf")) {
			return true;
		}
	}
	return false;
}

static bool is_mod(const std::string& filename){
	if(filename == "modinfo.tdf"){
		return true;
	}
	return false;
}

static bool is_map_or_mod(const std::string& filename, ArchiveType& current_filetype){
	ArchiveType filetype = R_OTHER;
	if(is_map(filename)){
		filetype = R_MAP;
	}else if(is_mod(filename)){
		filetype = R_MOD;
	}
	if(filetype != R_OTHER && filetype != current_filetype){
		if(current_filetype == R_OTHER){
			current_filetype = filetype;
		}else{
			return false;
		}
	}
	return true;
}


/******************************************************************************/

#if defined(ARCHIVE_MOVER_USE_WIN_API)
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{

	int argc = 0;
	wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	if(argv == 0){
		return 0;
	}

	int ret = run(argc, argv);
	LocalFree(argv);

	return ret;
}
#elif defined(_MSC_VER) && (defined(UNICODE) || defined(_UNICODE))
int wmain(int argc, const wchar_t* const* argv){
	return run(argc, argv);
}
#else
int main(int argc, const char* const* argv){
/*	//Untested
	mbstate_t mbstate;
	memset((void*)&mbstate, 0, sizeof(mbstate));

	wchar_t** argvW = new wchar_t*[argc];
	for(int i = 0; i < argc; i++){
		const char *indirect_string = argv[i];
		std::size_t num_chars = mbsrtowcs(NULL, &indirect_string, INT_MAX, &mbstate);
		if(num_chars == -1){
			return 1;
		}

		argvW[i] = new wchar_t[num_chars+1];
		num_chars = mbsrtowcs(argvW[i], &indirect_string, num_chars+1, &mbstate);
		if(num_chars == -1){
			return 1;
		}
	}

	int ret = run(argc, argvW);

	for(int i = 0; i < argc; i++){
		delete [] argvW[i];
	}
	delete [] argvW;

	return ret;
*/
	return run(argc, argv);
}
#endif


static int run(int argc, const Char* const* argv){

	ScopedMessage message;
	try{
		bool show_usage = false;
		bool quiet = false;
		const Char* source_file_arg = NULL;

		for (int i = 1; i < argc; ++i) {
			String arg = argv[i];
			if (arg[0] == '-') {
				if (arg == _("--help") || arg == _("-h"))
					show_usage = true;
				else if (arg == _("--quiet") || arg == _("-q"))
					quiet = true;
			} else {
				// multiple file arguments is erroneous
				if (source_file_arg != NULL)
					show_usage = true;
				source_file_arg = argv[i];
			}
		}

		if(show_usage || source_file_arg == NULL){
			message<<_("Usage: ")<<fs::system_complete(argv[0]).leaf()<<_(" [--quiet] <filename>")<<endl<<endl;
			return 1;
		}

		const Path source_file = fs::system_complete(source_file_arg);
		Path target_dir = fs::system_complete(argv[0]).branch_path();

		if(!fs::exists(source_file)){
			message<<source_file<<endl<<_("File not found.")<<endl;
			return 1;
		}


		ArchiveType content = read_archive_content_sd7(source_file);
		if(content == R_OTHER){
			content = read_archive_content_sdz(source_file);
		}

		if(content == R_OTHER){
			message<<_("'")<<source_file.leaf()<<_("' is not a valid map/mod ")
			_("it may be corrupted, try to redownload it.")<<endl;
			return 1;
		}


		//for(Path test_path = source_file; test_path.has_root_directory(); ){
		//	if(fs::equivalent(test_path, target_dir)){
		//		message<<_("'")<<source_file.leaf()<<_("' already exists in the Spring directory.")<<endl;
		//		return 1;
		//	}
		//	test_path = test_path.branch_path();
		//}


		if(content == R_MAP){
			//message<<"isMap: "<<filename<<endl;
			target_dir /= _("maps");
		}else if(content == R_MOD){
			//message<<"isMod: "<<filename<<endl;
			target_dir /= _("mods");
		}else{
			assert(false);
		}

		if(!fs::exists(target_dir)){
			message<<_("The target directory '")<<target_dir<<_("' doesn't exist.")<<endl;
			return 1;
		}

		// stash existing files away
		if (fs::exists(target_dir / source_file.leaf())) {
			int i = 0;
			Path target_test;
			do {
				String stashed = (source_file.leaf() + _(".old"));
				if (i > 0) {
					StringStream tmp;
					tmp << i;
					stashed += _(".")+tmp.str();
				}
				target_test = target_dir / stashed;
				++i;
			} while (fs::exists(target_test));
			fs::rename(target_dir / source_file.leaf(), target_test);
			message << "File with same name found. It has been moved to " << endl
				<< target_test << endl << endl;
		}

		target_dir /= source_file.leaf();
		fs::rename(source_file, target_dir);

		if (!quiet) {
			message<<_("The ")<<(content == R_MAP? _("map '") : _("mod '"))<<source_file.leaf()
			<<_("' has been saved succesfully to '")<<target_dir.branch_path()<<_("'.")<<endl
			<<_("Use the reload mods/maps button in the lobby to make Spring find it.")<<endl;
		}

	}
	catch(FilesystemPathError& error) {
		fs::errno_type error_nr = fs::lookup_errno(error.system_error());
		message<<_("Cannot move file: ");
		switch(error_nr){
			case EPERM:
			case EACCES:{
				message<<_("Permission denied.");
				break;
			}
			case ENOSPC:{
				message<<_("Not enough free disk space.");
				break;
			}
			case ENOENT:{
				message<<_("Invalid path.");
				break;
			}
			case EROFS:{
				message<<_("Target path is read only.");
				break;
			}
			case EEXIST:{
				message<<_("Target folder already contains a file named '")<<error.path1().leaf()<<_("'.");
				break;
			}
			default:{
				message<<strerror(error_nr)<<_(".");
			}
		}
		message<<endl<<endl;
		message<<_("Source file: ")<<error.path1()<<endl; 
		message<<_("Target folder: ")<<error.path2().branch_path()<<endl;
	}
	catch(fs::filesystem_error& error) {
		message<<_("Filesystem error in: ")<<error.what()<<endl;
	}
	catch(std::exception& error) {
		message<<_("Found an exception with: ")<<error.what()<<endl;
	}
	catch(...) {
		message<<_("Found an unknown exception.")<<endl;
	}

	return 0;
}



//******************************************************************************

static voidpf ZCALLBACK sdz_open_file_unicode(voidpf opaque, const char *filename, int mode){

	FILE* file = NULL;
	const Char* mode_fopen = NULL;
	if ((mode & ZLIB_FILEFUNC_MODE_READWRITEFILTER)==ZLIB_FILEFUNC_MODE_READ){
		mode_fopen = _("rb");
	}else if (mode & ZLIB_FILEFUNC_MODE_EXISTING){
		mode_fopen = _("r+b");
	}else if (mode & ZLIB_FILEFUNC_MODE_CREATE){
		mode_fopen = _("wb");
	}

	if(filename == NULL && mode_fopen != NULL && opaque != NULL){
		file = fopen(static_cast<const Path*>(opaque)->string().c_str(), mode_fopen);
	}
	return file;
}

static ArchiveType read_archive_content_sdz(const Path& filename){

	zlib_filefunc_def file_api;
	fill_fopen_filefunc(&file_api);
	file_api.opaque = &const_cast<Path&>(filename);
	file_api.zopen_file = sdz_open_file_unicode;

	unzFile_ptr sdz = unzOpen2_p(&file_api);
	if (*sdz == NULL) {
		return R_OTHER;
	}

	if (unzGoToFirstFile(*sdz) != UNZ_OK) {
		return R_OTHER;
	}

	const int buffer_size = 1024*1024;
	ArchiveType content = R_OTHER;
	boost::scoped_array<char> read_buf(new char[buffer_size]);


	while(1){
		unz_file_info file_info;
		char name_buf[256] = {0};

		if (unzGetCurrentFileInfo(*sdz, &file_info, name_buf, 256, NULL, 0, NULL, 0) != UNZ_OK) {
			return R_OTHER;
		}
		const std::string& filename = string_to_lower(name_buf);

		if(!fs::is_directory(filename)){

			if(!is_map_or_mod(filename, content)){
				return R_OTHER;
			}

			if(unzOpenCurrentFile(*sdz) != UNZ_OK){
				return R_OTHER;
			}

			int bytes_read = 0;
			do{
				bytes_read = unzReadCurrentFile(*sdz, read_buf.get(), buffer_size);
			}while(bytes_read > 0);

			if(unzCloseCurrentFile(*sdz) != UNZ_OK){
				return R_OTHER;
			}

			if(bytes_read != UNZ_OK ){
				return R_OTHER;
			}
		}

		int error = unzGoToNextFile(*sdz);
		if(error == UNZ_END_OF_LIST_OF_FILE){
			break;
		}else if(error != UNZ_OK){
			return R_OTHER;
		}
	}

	return content;
}


/******************************************************************************/

struct SD7Stream {
	ISzInStream funcs;
	fs::ifstream file;
};


static SZ_RESULT sd7_read_file(void *object, void *buffer, size_t size, size_t *processedSize){

	fs::ifstream& file = static_cast<SD7Stream*>(object)->file;
	file.read((char*)buffer, (std::streamsize)size);
	if(processedSize != 0){
		*processedSize = file.gcount();
	}
	return SZ_OK;
}


static SZ_RESULT sd7_seek_file(void *object, CFileSize pos){

	fs::ifstream& file = static_cast<SD7Stream*>(object)->file;
	file.seekg(pos, std::ios_base::beg);
	if(file.fail()){
		return SZE_FAIL;
	}
	return SZ_OK;
}


static ArchiveType read_archive_content_sd7(const Path& filename){

	SD7Stream stream;
	stream.funcs.Read = sd7_read_file;
	stream.funcs.Seek = sd7_seek_file;
	stream.file.open(filename, std::ios_base::binary);

	if(!stream.file.good() || !stream.file.is_open()){
		return R_OTHER;
	}


	InitCrcTable();
	CArchiveDatabaseEx_ptr db = SzArDbExInit_p();

	ISzAlloc alloc_main;
	ISzAlloc alloc_temp;
	alloc_main.Free  = SzFree;
	alloc_main.Alloc = SzAlloc;
	alloc_temp.Free  = SzFreeTemp;
	alloc_temp.Alloc = SzAllocTemp;


	BOOST_STATIC_ASSERT(offsetof(SD7Stream, funcs) == 0);
	int retval = SzArchiveOpen(&stream.funcs, db.get(), &alloc_main, &alloc_temp);
	if (retval != SZ_OK) {
		return R_OTHER;
	}


	unsigned int block_index = 0;
	SzOutBuffer_ptr out_buffer = SzOutBuffer_p(0);
	size_t out_buffer_size = 0;
	ArchiveType content = R_OTHER;

	for (unsigned int i = 0; i < db->Database.NumFiles; ++i) {
		CFileItem* fi = db->Database.Files + i;
		if(fi->IsDirectory){
			continue;
		}

		const std::string& filename = string_to_lower(fi->Name);
		if(!is_map_or_mod(filename, content)){
			return R_OTHER;
		}

		size_t offset = 0;
		size_t out_size_processed = 0;
		if(SzExtract(&stream.funcs, db.get(), i, &block_index,
					out_buffer.get(), &out_buffer_size, &offset, &out_size_processed,
					&alloc_main, &alloc_temp) != SZ_OK){

			return R_OTHER;
		}
	}

	return content;
}
