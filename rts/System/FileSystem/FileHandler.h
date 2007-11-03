#ifndef __FILE_HANDLER_H__
#define __FILE_HANDLER_H__

#include <set>
#include <vector>
#include <string>


#define SPRING_VFS_RAW  "r"
#define SPRING_VFS_MOD  "M"
#define SPRING_VFS_MAP  "m"
#define SPRING_VFS_BASE "b"
#define SPRING_VFS_NONE " "
#define SPRING_VFS_MOD_BASE   SPRING_VFS_MOD  SPRING_VFS_BASE
#define SPRING_VFS_MAP_BASE   SPRING_VFS_MAP  SPRING_VFS_BASE
#define SPRING_VFS_ZIP        SPRING_VFS_MOD  SPRING_VFS_MAP  SPRING_VFS_BASE
#define SPRING_VFS_RAW_FIRST  SPRING_VFS_RAW  SPRING_VFS_ZIP
#define SPRING_VFS_ZIP_FIRST  SPRING_VFS_ZIP  SPRING_VFS_RAW
#define SPRING_VFS_ALL        SPRING_VFS_RAW_FIRST



class CFileHandler {
	public:
		CFileHandler(const char* filename,
		             const char* modes = SPRING_VFS_RAW_FIRST);
		CFileHandler(const std::string& filename,
		             const std::string& modes = SPRING_VFS_RAW_FIRST);
		~CFileHandler(void);
		
		int Read(void* buf,int length);
		void Seek(int pos);

		bool FileExists() const;
		bool Eof() const;
		int Peek() const;
		int GetPos() const;
		int FileSize() const;
		
		bool LoadStringData(std::string& data);

	public:
		static std::vector<std::string> FindFiles(const std::string& path, const std::string& pattern);

		static std::vector<std::string> DirList(const std::string& path,
																						const std::string& pattern,
																						const std::string& modes);

		static std::string AllowModes(const std::string& modes,
		                              const std::string& allowed);
		static std::string ForbidModes(const std::string& modes,
		                               const std::string& forbidden);

	private:
		void Init(const std::string& filename, const std::string& modes);

		bool TryRawFS(const std::string& filename);
		bool TryModFS(const std::string& filename);
		bool TryMapFS(const std::string& filename);
		bool TryBaseFS(const std::string& filename);

	private:
		static bool InsertRawFiles(std::set<std::string>& fileSet,
															 const std::string& path,
															 const std::string& pattern);
		static bool InsertModFiles(std::set<std::string>& fileSet,
															 const std::string& path,
															 const std::string& pattern);
		static bool InsertMapFiles(std::set<std::string>& fileSet,
															 const std::string& path,
															 const std::string& pattern);
		static bool InsertBaseFiles(std::set<std::string>& fileSet,
																const std::string& path,
																const std::string& pattern);

	private:
		std::ifstream* ifs;
		unsigned char* hpiFileBuffer;
		int hpiLength;
		int hpiOffset;
		int filesize;
};

#endif // __FILE_HANDLER_H__
