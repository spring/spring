#ifndef __FILE_HANDLER_H__
#define __FILE_HANDLER_H__

#include <vector>
#include <string>

class CFileHandler
{
public:
	enum VFSmode {
		AnyFS,
		OnlyRawFS,
		OnlyArchiveFS
	};
	CFileHandler(const char* filename, VFSmode vfsMode = AnyFS);
	CFileHandler(std::string filename, VFSmode vfsMode = AnyFS);
	~CFileHandler(void);
	
	bool FileExists();

	void Read(void* buf,int length);
	void Seek(int pos);
	int Peek();
	bool Eof();
	int FileSize();
	int GetPos();
	
	bool LoadStringData(std::string& data);

	static std::vector<std::string> FindFiles(const std::string& path, const std::string& pattern);
private:
	void Init(const char* filename, VFSmode vfsMode);

	std::ifstream* ifs;
	unsigned char* hpiFileBuffer;
	int hpiLength;
	int hpiOffset;
	int filesize;
};

#endif // __FILE_HANDLER_H__
