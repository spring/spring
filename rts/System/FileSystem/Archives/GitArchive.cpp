/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "GitArchive.h"

#include <assert.h>
#include <fstream>

#include "System/FileSystem/DataDirsAccess.h"
#include "System/FileSystem/FileSystem.h"
#include "System/FileSystem/FileQueryFlags.h"
#include "System/StringUtil.h"
#include "System/Log/ILog.h"


static void checkRet(int Error, char const * Message, std::string const & Extra = "")
{
	if (!Error) return;
	git_error const * LogToError;
	char const * LogToErrorMessage = "";
	if ((LogToError = giterr_last()) != nullptr && LogToError->message != nullptr) {
		LogToErrorMessage = LogToError->message;
	}
	LOG_L(L_ERROR, "%s %s", Extra.c_str(), LogToErrorMessage);
	throw std::runtime_error{Extra};
}


CGitArchiveFactory::CGitArchiveFactory()
	: IArchiveFactory("git")
{
}

IArchive* CGitArchiveFactory::DoCreateArchive(const std::string& filePath) const
{
	return new CGitArchive(filePath);
}

CGitArchive::CGitArchive(const std::string& archiveName)
	: IArchive(archiveName)
	, dirName(archiveName + '/')
{
	int major, minor, rev;
	git_libgit2_init();
	git_libgit2_version(&major, &minor, &rev);
	LOG_L(L_INFO, "libgit version %d.%d.%d loading %s", major, minor, rev, archiveName.c_str());
	checkRet(git_repository_open_ext(&Repo, archiveName.c_str(), 0, nullptr), "git_repository_open_ext");

	/*
	// FIXME: extend like rapidhandler so all git versions are shown (+ find a way how to show +10k versions to users)
	const std::string GitHash = "HEAD";
	git_oid oid;
        checkRet(git_reference_name_to_id(&oid, Repo, GitHash.c_str()), "git_reference_name_to_id");
	*/

	git_reference * Reference = NULL;
	checkRet(git_repository_head(&Reference, Repo), "git_repository_head");

	checkRet(git_reference_peel((git_object **) &tree_root, Reference, GIT_OBJ_TREE), "git_reference_peel");

	LoadFilenames("", tree_root);

	for (size_t i=0; i < searchFiles.size(); i++) {
		lcNameIndex[StringToLower(searchFiles[i].filename)] = i;
	}
}

void CGitArchive::LoadFilenames(const std::string dirname, git_tree *tree)
{
	const size_t count = git_tree_entrycount(tree);
	for (size_t i = 0; i < count; i++) {
		const git_tree_entry *entry = git_tree_entry_byindex(tree, i);
		const char *name = git_tree_entry_name(entry);
		if (git_tree_entry_type(entry) == GIT_OBJECT_TREE) {
			git_tree *subtree = NULL;
			checkRet(git_tree_lookup(&subtree, Repo, git_tree_entry_id(entry)), "git_tree_lookup2");
			LoadFilenames(dirname + name + "/", subtree);
		} else {
			searchFiles.push_back({dirname + name, 0, nullptr});
		}
	}
}


static git_blob * GetBlob(searchfile& file, git_repository * Repo, const git_tree * tree_root)
{
	if (file.blob)
		return file.blob;
	//LOG_L(L_INFO, "loading blob");
	git_tree_entry * TreeEntry;
	checkRet(git_tree_entry_bypath(&TreeEntry, tree_root, file.filename.c_str()), "git_tree_entry_bypath");
	checkRet(git_blob_lookup(&file.blob, Repo, git_tree_entry_id(TreeEntry)), "git_blob_lookup");
	return file.blob;
}

static void FreeBlob(searchfile& file)
{
	if (!file.blob)
		return;
	git_blob_free(file.blob);
	file.blob = nullptr;
}

bool CGitArchive::GetFile(unsigned int fid, std::vector<std::uint8_t>& buffer)
{
	assert(IsFileId(fid));
	const std::string filename = searchFiles[fid].filename;
	//LOG_L(L_INFO, "GetFile %s", filename.c_str());

	if (filename == "modinfo.lua") {
		// FIXME:
		// Rapid::replaceVersion()
		// first replace by git tag when exists, else parse from commit message
	}

	git_blob * Blob = GetBlob(searchFiles[fid], Repo, tree_root);
	const size_t Size = git_blob_rawsize(Blob);
	buffer.resize(Size);
	const void * BlobBuf = git_blob_rawcontent(Blob);
	memcpy(buffer.data(), BlobBuf, Size);
	FreeBlob(searchFiles[fid]);
	return true;
}

void CGitArchive::FileInfo(unsigned int fid, std::string& name, int& size) const
{
	assert(IsFileId(fid));
	// LOG_L(L_INFO, "FileInfo %s", searchFiles[fid].filename.c_str());
	git_blob * Blob = GetBlob(searchFiles[fid], Repo, tree_root);
	size = git_blob_rawsize(Blob);
	name = searchFiles[fid].filename;
}

CGitArchive::~CGitArchive()
{
	for (size_t i = 0; i < searchFiles.size(); i++) {
		//TODO: GetBlob seems to load the data, find a way to avoid loading in FileInfo / duplicate load
		FreeBlob(searchFiles[i]);
	}
	git_libgit2_shutdown();
}
