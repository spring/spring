
#include "System/FileSystem/FileSystem.h"

#include <string>
#include <cstdio>
#include <cstdlib>

#define BOOST_TEST_MODULE FileSystem
#include <boost/test/unit_test.hpp>


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
				BOOST_FAIL("Failed to create test-file " + filePath);
			}
		}
		std::string GetTempDir() {
			if (testCwd.empty()) {
				char tmpDirTemplate[128];
				strcpy(tmpDirTemplate, "Spring_Tests_FileSystem_XXXXXX");
				char* tmpDir = mkdtemp(tmpDirTemplate);
				if (tmpDir != NULL) {
					testCwd = tmpDir;
				} else {
					BOOST_FAIL("Failed to create temporary test dir");
				}
			}
			return testCwd;
		}
	private:
		std::string oldDir;
		std::string testCwd;
	};
}

BOOST_FIXTURE_TEST_SUITE(everything, PrepareFileSystem)


BOOST_AUTO_TEST_CASE(FileExists)
{
	BOOST_CHECK(FileSystem::FileExists("testFile.txt"));
	BOOST_CHECK(!FileSystem::FileExists("testFile99.txt"));
	BOOST_CHECK(!FileSystem::FileExists("testDir"));
	BOOST_CHECK(!FileSystem::FileExists("testDir99"));
}


BOOST_AUTO_TEST_CASE(GetFileSize)
{
	BOOST_CHECK(FileSystem::GetFileSize("testFile.txt") == 1);
	BOOST_CHECK(FileSystem::GetFileSize("testFile99.txt") == 0);
	BOOST_CHECK(FileSystem::GetFileSize("testDir") > 0);
	BOOST_CHECK(FileSystem::GetFileSize("testDir99") == 0);
}

BOOST_AUTO_TEST_CASE(CreateDirectory)
{
	BOOST_CHECK(FileSystem::CreateDirectory("testDir")); // already exists
	BOOST_CHECK(FileSystem::CreateDirectory("testDir1")); // should be created
	FileSystem::DeleteFile("testDir1");
	BOOST_CHECK(!FileSystem::CreateDirectory("testFile.txt")); // file with this name already exists
}


BOOST_AUTO_TEST_CASE(GetDirectory)
{
#define CHECK_DIR_EXTRACTION(path, dir) \
		BOOST_CHECK(FileSystem::GetDirectory(path) == dir)

	CHECK_DIR_EXTRACTION("testFile.txt", "");
	CHECK_DIR_EXTRACTION("./foo/testFile.txt", "./foo/");

#undef CHECK_DIR_EXTRACTION
}

BOOST_AUTO_TEST_SUITE_END()

