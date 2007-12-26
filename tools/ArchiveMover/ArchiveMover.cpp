/******************************************************************************/
/******************************************************************************/
//
//  file:     ArchiveMover.cpp
//  authors:  zizu
//            Dave Rodgers  (aka: trepan)
//            AF (wrote the original java version)
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


#define ARCHIVE_MOVER_USE_WIN_API

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


enum archive_type {
	R_MOD,
	R_MAP,
	R_OTHER
};

static int run(int argc, wchar_t** argv);
static archive_type read_archive_content_sd7(const fs::wpath& filename);
static archive_type read_archive_content_sdz(const fs::wpath& filename);


static std::string string_to_lower(const std::string& s){

	std::string l = s;
	std::transform(l.begin(), l.end(), l.begin(), static_cast<int (*)(int)>(tolower));
	return l;
}


struct scoped_message : public std::wstringstream{
	~scoped_message(){
#ifdef ARCHIVE_MOVER_USE_WIN_API
		MessageBoxW(NULL, this->str().c_str(), L"Spring - ArchiveMover", MB_APPLMODAL);
#else
		std::wcout<<this->str()<<endl;
#endif
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

static bool is_map_or_mod(const std::string& filename, archive_type& current_filetype){
	archive_type filetype = R_OTHER;
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
int wmain(int argc, wchar_t** argv){
	return run(argc, argv);
}
#else
int main(int argc, char** argv){
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
}
#endif


static int run(int argc, wchar_t** argv){

	scoped_message message;
	try{

		if(argc == 0){
			message<<L"Error: Zero arguments."<<endl;
			return 1;
		}else if(argc != 2){
			message<<L"Usage: "<<fs::system_complete(argv[0]).leaf()<<L" <filename>"<<endl<<endl;
			return 1;
		}


		const fs::wpath source_file = fs::system_complete(argv[1]);
		fs::wpath target_dir = fs::system_complete(argv[0]).branch_path();

		if(!fs::exists(source_file)){
			message<<source_file<<endl<<L"File not found."<<endl;
			return 1;
		}


		archive_type content = read_archive_content_sd7(source_file);
		if(content == R_OTHER){
			content = read_archive_content_sdz(source_file);
		}

		if(content == R_OTHER){
			message<<L"'"<<source_file.leaf()<<L"' is not a valid map/mod "
			L"it may be corrupted, try to redownload it."<<endl;
			return 1;
		}


		//for(fs::wpath test_path = source_file; test_path.has_root_directory(); ){
		//	if(fs::equivalent(test_path, target_dir)){
		//		message<<L"'"<<source_file.leaf()<<L"' already exists in the Spring directory."<<endl;
		//		return 1;
		//	}
		//	test_path = test_path.branch_path();
		//}


		if(content == R_MAP){
			//message<<"isMap: "<<filename<<endl;
			target_dir /= L"maps";
		}else if(content == R_MOD){
			//message<<"isMod: "<<filename<<endl;
			target_dir /= L"mods";
		}else{
			assert(false);
		}

		if(!fs::exists(target_dir)){
			message<<L"The target directory '"<<target_dir<<L"' doesn't exist."<<endl;
			return 1;
		}

		// stash existing files away
		if (fs::exists(target_dir / source_file.leaf())) {
			int i = 0;
			fs::wpath target_test;
			do {
				std::wstring stashed = (source_file.leaf() + L".old");
				if (i > 0) {
					std::wstringstream tmp;
					tmp << i;
					stashed += L"."+tmp.str();
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


		message<<L"The "<<(content == R_MAP? L"map '" : L"mod '")<<source_file.leaf()
		<<L"' has been saved succesfully to '"<<target_dir.branch_path()<<L"'."<<endl
		<<L"Use the reload mods/maps button in the lobby to make Spring find it."<<endl;


	}catch(fs::filesystem_wpath_error& error){

		fs::errno_type error_nr = fs::lookup_errno(error.system_error());
		message<<L"Cannot move file: ";
		switch(error_nr){
			case EPERM:
			case EACCES:{
				message<<L"Permission denied.";
				break;
			}
			case ENOSPC:{
				message<<L"Not enough free disk space.";
				break;
			}
			case ENOENT:{
				message<<L"Invalid path.";
				break;
			}
			case EROFS:{
				message<<L"Target path is read only.";
				break;
			}
			case EEXIST:{
				message<<L"Target folder already contains a file named '"<<error.path1().leaf()<<L"'.";
				break;
			}
			default:{
				message<<strerror(error_nr)<<L".";
			}
		}
		message<<endl<<endl;
		message<<L"Source file: "<<error.path1()<<endl; 
		message<<L"Target folder: "<<error.path2().branch_path()<<endl;

	}catch(fs::filesystem_error& error){
		message<<L"Filesystem error in: "<<error.what()<<endl;
	}
	catch(std::exception& error){
		message<<L"Found an exception with: "<<error.what()<<endl;
	}
	catch(...){
		message<<L"Found an unknown exception."<<endl;
	}

	return 0;
}



//******************************************************************************

static voidpf ZCALLBACK sdz_open_file_unicode(voidpf opaque, const char *filename, int mode){

	FILE* file = NULL;
	const wchar_t* mode_fopen = NULL;
	if ((mode & ZLIB_FILEFUNC_MODE_READWRITEFILTER)==ZLIB_FILEFUNC_MODE_READ){
		mode_fopen = L"rb";
	}else if (mode & ZLIB_FILEFUNC_MODE_EXISTING){
		mode_fopen = L"r+b";
	}else if (mode & ZLIB_FILEFUNC_MODE_CREATE){
		mode_fopen = L"wb";
	}

	if(filename == NULL && mode_fopen != NULL && opaque != NULL){
		file = _wfopen(static_cast<const fs::wpath*>(opaque)->string().c_str(), mode_fopen);
	}
	return file;
}

static archive_type read_archive_content_sdz(const fs::wpath& filename){

	zlib_filefunc_def file_api;
	fill_fopen_filefunc(&file_api);
	file_api.opaque = &const_cast<fs::wpath&>(filename);
	file_api.zopen_file = sdz_open_file_unicode;

	unzFile_ptr sdz = unzOpen2_p(&file_api);
	if (*sdz == NULL) {
		return R_OTHER;
	}

	if (unzGoToFirstFile(*sdz) != UNZ_OK) {
		return R_OTHER;
	}

	const int buffer_size = 1024*1024;
	archive_type content = R_OTHER;
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


static archive_type read_archive_content_sd7(const fs::wpath& filename){

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
	archive_type content = R_OTHER;

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
