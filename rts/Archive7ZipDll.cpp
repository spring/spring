#include "StdAfx.h"
#include "Archive7ZipDll.h"
#include "7zip/Windows/PropVariant.h"
#include "7zip/Windows/PropVariantConversions.h"
#include <algorithm>
//#include "mmgr.h"

/*
 * Most of this code comes from the Client7z-example in the 7zip source distribution
 *
 * 7zip also includes a ansi c library for accessing 7zip files which seems to be much 
 * nicer to work with, but unfortunately it doesn't support folders inside archives
 * yet. But it should apparently be added soon. When it is, this class should probably
 * be rewritten to use that other library instead. I had written most of it before
 * I realised it didn't support directories anyway, so it should be quick. :)
 */

// {23170F69-40C1-278A-1000-000110050000}
DEFINE_GUID(CLSID_CFormat7z, 
  0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x05, 0x00, 0x00);

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

class CTestOutFileStream: 
	public ISequentialOutStream,
	public CMyUnknownImp
{
public:
	MY_UNKNOWN_IMP

	virtual ~CTestOutFileStream() {}
	STDMETHOD(Write)(const void *data, UInt32 size, UInt32 *processedSize);
	STDMETHOD(WritePart)(const void *data, UInt32 size, UInt32 *processedSize);
	void SetBuf(ABOpenFile_t* buf);
private:
	ABOpenFile_t* buf;
	char* curbuf;
};

void CTestOutFileStream::SetBuf(ABOpenFile_t* buf)
{
	this->buf = buf;
	curbuf = buf->data;
}

HRESULT CTestOutFileStream::Write(const void *data, UInt32 size, UInt32 *processedSize)
{
	//printf("write %d\n", size);
	memcpy(curbuf, data, size);
	*processedSize = size;

	curbuf += size;

	return S_OK;
}

HRESULT CTestOutFileStream::WritePart(const void *data, UInt32 size, UInt32 *processedSize)
{
	//printf("writepart %d\n", size);
	return Write(data, size, processedSize);
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

class CExtractCallbackImp: 
	public IArchiveExtractCallback,
	public CMyUnknownImp
{
public:

	MY_UNKNOWN_IMP

	// IProgress
	STDMETHOD(SetTotal)(UInt64 size);
	STDMETHOD(SetCompleted)(const UInt64 *completeValue);

	// IExtractCallback
	STDMETHOD(GetStream)(UInt32 index, ISequentialOutStream **outStream,
      Int32 askExtractMode);
	STDMETHOD(PrepareOperation)(Int32 askExtractMode);
	STDMETHOD(SetOperationResult)(Int32 resultEOperationResult);

	void SetBuf(ABOpenFile_t* buf);
private:
	ABOpenFile_t* buf;
};

void CExtractCallbackImp::SetBuf(ABOpenFile_t* buf)
{
	//printf("got buf\n");
	this->buf = buf;
}

HRESULT CExtractCallbackImp::SetTotal(UInt64 size)
{
	//printf("Total: %ld\n", size);
	return S_OK;
}

HRESULT CExtractCallbackImp::SetOperationResult(Int32 resultEOperationResult)
{
	//printf("boo %d\n", resultEOperationResult);
	return S_OK;
}

HRESULT CExtractCallbackImp::PrepareOperation(Int32 askExtractMode)
{
	//printf("hoo %d\n", askExtractMode);
	return 0;
}

HRESULT CExtractCallbackImp::GetStream(UInt32 index, ISequentialOutStream **outStream, Int32 askExtractMode)
{
	//printf("lol %d\n", index);

	CTestOutFileStream *fileSpec = new CTestOutFileStream;
	fileSpec->SetBuf(buf);
	CMyComPtr<ISequentialOutStream> file = fileSpec;

	*outStream = file.Detach();

	return S_OK;
}

HRESULT CExtractCallbackImp::SetCompleted(const UInt64 *completeValue)
{
	//printf("Set completed: %ld\n", *completeValue);
	return S_OK;
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

CArchive7ZipDll::CArchive7ZipDll(const string& name) :
	CArchiveBuffered(name),
	archive(NULL),
	curSearchHandle(1)
{
	CInFileStream *fileSpec = new CInFileStream;
	CMyComPtr<IInStream> file = fileSpec;

	if (!fileSpec->Open(name.c_str()))
	{
		//printf("Can not open");
		return;
	}
	if (archive->Open(file, 0, 0) != S_OK)
	    return;

	UInt32 numItems = 0;
	archive->GetNumberOfItems(&numItems);  
	for (UInt32 i = 0; i < numItems; i++)
	{
		NWindows::NCOM::CPropVariant propVariant;
		archive->GetProperty(i, kpidPath, &propVariant);
		UString s = ConvertPropVariantToString(propVariant);

		archive->GetProperty(i, kpidSize, &propVariant);
		int size = (int)ConvertPropVariantToUInt64(propVariant);
		
		if (size > 0) {
			string fileName = (LPCSTR)GetOemString(s);
			transform(fileName.begin(), fileName.end(), fileName.begin(), (int (*)(int))tolower);

		//	printf("%d %d %s\n", i, size, (LPCSTR)GetOemString(s));
			FileData fd;
			fd.index = i;
			fd.size = size;
			fd.origName = (LPCSTR)GetOemString(s);

			fileData[fileName] = fd;
		}
	}
}

CArchive7ZipDll::~CArchive7ZipDll(void)
{
	//if (archive)
		//delete archive;	//crash?
}


ABOpenFile_t* CArchive7ZipDll::GetEntireFile(const string& fName)
{
	string fileName = fName;
	transform(fileName.begin(), fileName.end(), fileName.begin(), (int (*)(int))tolower);	

	if (fileData.find(fileName) == fileData.end())
		return NULL;

	FileData fd = fileData[fileName];

	ABOpenFile_t* of = new ABOpenFile_t;
	of->pos = 0;
	of->size = fd.size;
	of->data = (char*)malloc(of->size); 

	CExtractCallbackImp *ExtractCallbackSpec;
	CMyComPtr<IArchiveExtractCallback> ExtractCallback;

	ExtractCallbackSpec = new CExtractCallbackImp;
	ExtractCallbackSpec->SetBuf(of);
	ExtractCallback = ExtractCallbackSpec;

	UInt32 files[1];
	files[0] = fd.index;

	archive->Extract(files, 1, 0, ExtractCallback);

	return of;
}

bool CArchive7ZipDll::IsOpen()
{
	return (archive != NULL);
}

int CArchive7ZipDll::FindFiles(int cur, string* name, int* size)
{
	if (cur == 0) {
		curSearchHandle++;
		cur = curSearchHandle;
		searchHandles[cur] = fileData.begin();
	}

	if (searchHandles[cur] == fileData.end()) {
		searchHandles.erase(cur);
		return 0;
	}

	*name = searchHandles[cur]->second.origName;
	*size = searchHandles[cur]->second.size;

	searchHandles[cur]++;
	return cur;
}
