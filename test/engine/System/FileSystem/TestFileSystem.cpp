#include <string>
#include <cstdio>
#include <sys/stat.h>
#include "System/Log/ILog.h"

#define CATCH_CONFIG_MAIN
#include "lib/catch.hpp"

// needs to be included after catch
#include "System/FileSystem/FileSystem.h"

namespace {
	struct PrepareFileSystem {
		PrepareFileSystem() {
			oldDir = FileSystem::GetCwd();
			testCwd = GetTempDir();
			if (!testCwd.empty()) {
				FileSystem::ChDir(testCwd);

				// create files and dirs to test
				FileSystem::CreateDirectory("testDir");
				WriteFile("testFile.txt", "a");
			}
		}
		~PrepareFileSystem() {
			if (!testCwd.empty()) {
				// delete files and dirs created in the ctor
				FileSystem::DeleteFile("testFile.txt");
				FileSystem::DeleteFile("testDir");
			}

			FileSystem::ChDir(oldDir);
			if (!testCwd.empty()) {
				FileSystem::DeleteFile(testCwd);
			}
		}
		static void WriteFile(const std::string& filePath, const std::string& content) {
			FILE* testFile = fopen(filePath.c_str(), "w");
			if (testFile != NULL) {
				fprintf(testFile, "%s", content.c_str());
				fclose(testFile);
				testFile = NULL;
			} else {
				FAIL("Failed to create test-file " + filePath);
			}
		}
		std::string GetTempDir() {
			if (testCwd.empty()) {
				char* tmpDir = tmpnam(NULL);
				if (tmpDir != NULL) {
					testCwd = oldDir + tmpDir;
					FileSystem::CreateDirectory(testCwd);
					if (!FileSystem::DirIsWritable(testCwd)) {
						FAIL("Failed to create temporary test dir");
					}
				} else {
					FAIL("Failed to get temporary file name");
				}
			}
			return testCwd;
		}

		std::string testCwd;
	private:
		std::string oldDir;
	};
}

PrepareFileSystem pfs;

TEST_CASE("FileExists")
{
	CHECK(FileSystem::FileExists("testFile.txt"));
	CHECK_FALSE(FileSystem::FileExists("testFile99.txt"));
	CHECK_FALSE(FileSystem::FileExists("testDir"));
	CHECK_FALSE(FileSystem::FileExists("testDir99"));
}


TEST_CASE("GetFileSize")
{
	CHECK(FileSystem::GetFileSize("testFile.txt") == 1);
	CHECK(FileSystem::GetFileSize("testFile99.txt") == -1);
	CHECK(FileSystem::GetFileSize("testDir") == -1);
	CHECK(FileSystem::GetFileSize("testDir99") == -1);
}


TEST_CASE("GetFileModificationDate")
{
	CHECK(FileSystem::GetFileModificationDate("testDir") != "");
	CHECK(FileSystem::GetFileModificationDate("testFile.txt") != "");
	CHECK(FileSystem::GetFileModificationDate("not_there") == "");
}


TEST_CASE("CreateDirectory")
{
	// create & exists
	CHECK(FileSystem::DirIsWritable("./"));
	CHECK(FileSystem::DirExists("testDir"));
	CHECK(FileSystem::DirExists("testDir///"));
	CHECK(FileSystem::DirExists("testDir////./"));
	CHECK(FileSystem::ComparePaths("testDir", "testDir////./"));
	CHECK_FALSE(FileSystem::ComparePaths("testDir", "test Dir2"));
	CHECK(FileSystem::CreateDirectory("testDir")); // already exists
	CHECK(FileSystem::CreateDirectory("testDir1")); // should be created
	CHECK(FileSystem::CreateDirectory("test Dir2")); // should be created

	// check if exists & no overwrite
	CHECK(FileSystem::CreateDirectory("test Dir2")); // already exists
	CHECK(FileSystem::DirIsWritable("test Dir2"));
	CHECK_FALSE(FileSystem::CreateDirectory("testFile.txt")); // file with this name already exists

	// delete temporaries
	CHECK(FileSystem::DeleteFile("testDir1"));
	CHECK(FileSystem::DeleteFile("test Dir2"));

	// check if really deleted
	CHECK_FALSE(FileSystem::DirExists("testDir1"));
	CHECK_FALSE(FileSystem::DirExists("test Dir2"));
}


TEST_CASE("GetDirectory")
{
#define CHECK_DIR_EXTRACTION(path, dir) \
		CHECK(FileSystem::GetDirectory(path) == dir)

	CHECK_DIR_EXTRACTION("testFile.txt", "");
	CHECK_DIR_EXTRACTION("./foo/testFile.txt", "./foo/");

#undef CHECK_DIR_EXTRACTION
}


TEST_CASE("GetNormalizedPath")
{
#define CHECK_NORM_PATH(path, normPath) \
		CHECK(FileSystem::GetNormalizedPath(path) == normPath)

	CHECK_NORM_PATH("/home/userX/.spring/foo/bar///./../test.log", "/home/userX/.spring/foo/test.log");
	CHECK_NORM_PATH("./symLinkToHome/foo/bar///./../test.log", "./symLinkToHome/foo/test.log");
	CHECK_NORM_PATH("C:\\foo\\bar\\\\\\.\\..\\test.log", "C:/foo/test.log");

#undef CHECK_NORM_PATH
}
