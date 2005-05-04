/*
  HPIUtil - HPI file utilities
*/

#define NO_COMPRESSION 0
#define LZ77_COMPRESSION 1
#define ZLIB_COMPRESSION 2

/*
 GetTADirectory - Get the directory that TA is installed in
 Arguments:
   TADir - Pointer to buffer of at least MAX_PATH bytes to hold the returned directory.
*/
void WINAPI GetTADirectory(LPSTR TADir);
 
/*
 HPIOpen - Opens an HPI archive
 Arguments:
   FileName - The name of the HPI file
 Returns:
   If not successful, returns NULL
      Either the file does not exist, or it's not a valid
      HPI file
   Otherwise, return a handle to the HPI file that is
   used in the other functions

 When finished, use HPIClose to close the archive
*/
LPVOID WINAPI HPIOpen(LPSTR FileName);
 

/*
 HPIGetFiles - Enumerate all of the files in the HPI archive
 Arguments:
   HPI - handle to open HPI file returned by HPIOpen
   NextEntry - Set to 0 if this is the start of the search.
               Oterwise, set to the return value of the last
               HPIGetFiles call
   Name - buffer of at least MAX_PATH bytes to hold the returned file name
          The name includes the complete path of the file in
          the HPI archive.
   FileType - Pointer to long variable to hold the file type
              0 = File, 1 = Directory
   Size - Pointer to long variable to hold the file size
          If the file is a directory, this is the number
          of files in the directory.
          Otherwise, it is the uncompressed size of the file.
 Returns:
   Returns 0 if there are no more files to enumerate.
   Otherwise, returns a value to use as the 'NextEntry' argument
   for the next call to HPIGetFiles.
*/
LRESULT WINAPI HPIGetFiles(void *hpi, int Next, LPSTR Name, LPINT Type, LPINT Size);

/*
 HPIDir - Enumerate all of the files in a subdirectory in the HPI archive
 Arguments:
   HPI - handle to open HPI file returned by HPIOpen
   NextEntry - Set to 0 if this is the start of the search.
              Otherwise, set to the return value of the last
              HPIDir call
   DirName - The directory name whose files you want to list
   Name - buffer of at least MAX_PATH bytes to hold the returned file name
          The name does not include any path information.
   FileType - Pointer to long variable to hold the file type
              0 = File, 1 = Directory
   Size - Pointer to long variable to hold the file size
          If the file is a directory, this is the number
          of files in the directory.
          Otherwise, it is the uncompressed size of the file.
 Returns:
   Returns 0 if there are no more files to enumerate.
   Otherwise, returns a value to use as the 'NextEntry' argument
   for the next call to HPIDir.
*/ 
LRESULT WINAPI HPIDir(void *hpi, int Next, LPSTR DirName, LPSTR Name, LPINT Type, LPINT Size);
 
/*
 HPIClose - Close an open HPI archive
 Arguments:
   HPI - handle to open HPI file returned by HPIOpen
 Returns:
   Always returns 0

 Always close the archive with HPIClose.
*/
LRESULT WINAPI HPIClose(void *hpi);

/*
 HPIOpenFile - Decode a file inside an HPI archive to memory
 Arguments:
   HPI - handle to open HPI file returned by HPIOpen
   FileName - Fully qualified file name to open
 Returns:
   NULL if not successful
   else returns pointer to memory block holding entire file.
   Note: This can be a big chunk of memory, especially for large maps.
         When finished, use HPICloseFile to deallocate the memory
*/ 
LPSTR WINAPI HPIOpenFile(void *hpi, LPSTR FileName);

/*
  HPIGet - Extract data from file opened by HPIOpenFile
  Arguments:
    Dest - Destination address
    FileHandle - Pointer returned by HPIOpenFile
    offset - offset within FileHandle
    bytecount - number of bytes to copy
    
*/
void WINAPI HPIGet(void *Dest, void *FileHandle, int offset, int bytecount);
 
/*
 HPICloseFile - Closes a file opened with HPIOpenFile
 Arguments:
   FileHandle - pointer previously returned by HPIOpenFile
 Returns:
   Always returns 0
*/
LRESULT WINAPI HPICloseFile(LPSTR FileHandle);
 
/*
 HPIExtractFile - Extract a file from an HPI file to disk
 Arguments:
   HPI - handle to open HPI file returned by HPIOpen
   FileName - Fully qualified file name to extract
   ExtractName - File name to extract it to
 Returns:
   0 if not successful
   non-zero if successful
*/
LRESULT WINAPI HPIExtractFile(void *hpi, LPSTR FileName, LPSTR ExtractName);

/*
 HPICreate - Create an HPI archive
 Arguments:
   FileName - The name of the HPI file to create
   Callback - The address of a callback function (see notes)
              If there is no callback function, use NULL
 Returns:
   0 if not successful
   else returns handle to be used in
   HPICreateDirectory, HPIAddFile, and/or HPIPackFile
 Note:
   Any existing HPI archive with this name will be overwritten.

   This function prepares a new HPI archive.  After this, call
   HPIAddFile and/or HPICreateDirectory for each file you want to add.
   When all files have been added, call HPIPackFile to actually create
   the HPI archive.

   The callback function is called periodically during the pack process
   so that the program can display progress information.
   It's defined thusly:
*/
typedef int (__stdcall *HPICALLBACK)(LPSTR FileName, LPSTR HPIName, 
                           int FileCount, int FileCountTotal,
                           int FileBytes, int FileBytesTotal,
                           int TotalBytes, int TotalBytesTotal);
/*
   Arguments:
     FileName - the file name being packed.
     HPIName - the name in the HPI file that it's being packed into
     FileCount - the number of the file being packed
     FileCountTotal - the total number of files to pack
        (use these to display 'File 1 of 20' type messages)
     FileBytes - The amount of the file that has been packed
     FileBytesTotal - The file size to be packed
     TotalBytes - The total amount of data that's been packed
     TotalBytesTotal - The total amount of data to be packed
   Return value:
     Return non-zero to stop the pack process
     return 0 to continue the pack process

   If you don't want to use a callback function, pass NULL as
   the Callback argument.
*/
LPVOID WINAPI HPICreate(LPSTR FileName, HPICALLBACK Callback);

/*
 HPICreateDirectory - Create a directory in an HPI archive
 Arguments:
   Pack - the handle returned by HPICreate
   DirName - the complete directory name to create
 Returns:
   0 if not successful
   non-zero if successful
 Note:
   This function is the only way to create an empty supdirectory in the
   HPI archive.
*/
LRESULT WINAPI HPICreateDirectory(void *Pack, LPSTR DirName);

/*
 HPIAddFile - Add a file to an HPI archive
 Arguments:
   Pack - the handle returned by HPICreate
   HPIName - the complete path and file name to create in the HPI archive
   FileName - the file name on disk to add
 Returns:
   0 if not successful
   non-zero if successful
 Note:
   Directories in the HPI archive will be created as necessary.
*/
LRESULT WINAPI HPIAddFile(void *Pack, LPSTR HPIName, LPSTR FileName);

/*
 HPIAddFileFromMemory - Add a file in memory to an HPI archive
 Arguments:
   Pack - the handle returned by HPICreate
   HPIName - the complete path and file name to create in the HPI archive
   FileBlock - pointer to file in memory to add
	 fsize - length of FileBlock
 Returns:
   0 if not successful
   non-zero if successful
 Note:
   Directories in the HPI archive will be created as necessary.
	 The 'FileBlock' can be obtained from HPIOpenFile
*/
LRESULT WINAPI HPIAddFileFromMemory(void *Pack, LPSTR HPIName, LPSTR FileBlock, int fsize);

/*
 HPIPackArchive - Pack the HPI archive
 Arguments:
   Pack - the handle returned by HPICreate
   CMethod - NO_COMPRESSION - No compression (duh)
	         - LZ77_COMPRESSION - use LZ77 compression
           - ZLIB_COMPRESSION - use ZLib compression
 Returns:
   0 if not successful
   non-zero if successful
 Note:
   The 'Pack' handle is no longer valid after this call.
   The Callback function specified in the HPICreate function is called
     periodically during the pack process.
   This function will not return until the packing is complete, which can
   take awhile for large amounts of data.
*/
LRESULT WINAPI HPIPackArchive(void *Pack, int CMethod);

/*
This function is included so that older programs that use this call
will not break.  Use HPIPackArchive instead.
This call does the equivalent of:
  HPIPackArchive(Pack, LZ77_COMPRESSION);
*/
LRESULT WINAPI HPIPackFile(void *Pack);

