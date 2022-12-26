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
	checkRet(git_repository_open_bare(&Repo, archiveName.c_str()), "git_repository_open_bare");

	/*
	// FIXME: extend like rapidhandler so all git versions are shown (+ find a way how to show +10k versions to users)
	const std::string GitHash = "HEAD";
	git_oid oid;
        checkRet(git_reference_name_to_id(&oid, Repo, GitHash.c_str()), "git_reference_name_to_id");
	*/

	checkRet(git_repository_head(&reference_root, Repo), "git_repository_head");

	checkRet(git_reference_peel((git_object **) &tree_root, reference_root, GIT_OBJ_TREE), "git_reference_peel");

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
			git_tree *subtree = nullptr;
			checkRet(git_tree_lookup(&subtree, Repo, git_tree_entry_id(entry)), "git_tree_lookup2");
			LoadFilenames(dirname + name + "/", subtree);
			git_tree_free(subtree);
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
	git_tree_entry * TreeEntry = nullptr;
	checkRet(git_tree_entry_bypath(&TreeEntry, tree_root, file.filename.c_str()), "git_tree_entry_bypath");
	checkRet(git_blob_lookup(&file.blob, Repo, git_tree_entry_id(TreeEntry)), "git_blob_lookup");
	git_tree_entry_free(TreeEntry);
	return file.blob;
}

static void FreeBlob(searchfile& file)
{
	if (!file.blob)
		return;
	git_blob_free(file.blob);
	file.blob = nullptr;
}

struct CommitInfoT
{
	std::string Version;
	bool release;
};

CommitInfoT extractVersion(std::string const & Log, std::string const & RevisionString)
{
	std::string const StableString{"STABLE"};
	std::string const VersionString{"VERSION{"};

	if ( Log.size() >= StableString.size() && std::equal(StableString.begin(), StableString.end(), Log.begin())) {
		return {std::string("stable-") + RevisionString, true};
	} else if ( Log.size() >= VersionString.size() && std::equal(VersionString.begin(), VersionString.end(), Log.begin())) {
		auto First = Log.begin() + VersionString.size();
		auto Last = Log.end();
		auto EndPos = std::find(First, Last, '}');
		if (EndPos != Last) return {{First, EndPos}, true};
	}
	return {std::string("test-") + RevisionString, false};
}

static std::string GetVersion(git_repository* Repo, const git_reference* reference)
{
	// Find the commit count
	const git_oid* DestOid = git_reference_target(reference);
	git_revwalk * Walker;
	checkRet(git_revwalk_new(&Walker, Repo), "git_revwalk_new");
	checkRet(git_revwalk_push(Walker, DestOid), "git_revwalk_push");

	std::size_t CommitCount = 0;
	while (true) {
		git_oid WalkerOid;
		int Ret = git_revwalk_next(&WalkerOid, Walker);
		if (Ret == GIT_ITEROVER) break;
		checkRet(Ret, "git_revwalk_next");
		++CommitCount;
	}
	git_revwalk_free(Walker);

	const char* cbranch = git_reference_shorthand(reference);
	const std::string branch(cbranch);

	// Extract the commit type from the commit message
	git_commit * Commit;
	checkRet(git_commit_lookup(&Commit, Repo, DestOid), "git_commit_lookup");
	//std::size_t AncestorCount = git_commit_parentcount(Commit);
	std::string TestVersion = std::to_string(CommitCount) +  '-' + git_oid_tostr_s(DestOid);
	auto CommitInfo = extractVersion(git_commit_message_raw(Commit), TestVersion);
	git_commit_free(Commit);
	return branch + "-" + IntToString(CommitCount) +  "-" + CommitInfo.Version.substr(CommitInfo.Version.length() - 8);
}


bool CGitArchive::GetFile(unsigned int fid, std::vector<std::uint8_t>& buffer)
{
	assert(IsFileId(fid));
	const std::string filename = searchFiles[fid].filename;
	//LOG_L(L_INFO, "GetFile %s", filename.c_str());

	git_blob * Blob = GetBlob(searchFiles[fid], Repo, tree_root);
	const size_t Size = git_blob_rawsize(Blob);
	if (Size == 0)
		return true;
	const void * BlobBuf = git_blob_rawcontent(Blob);

	if (filename == "modinfo.lua") {
		// FIXME: adjust return info of FileInfo, too
		const std::string tmp((const char*)BlobBuf, Size);
		git_commit* commit=nullptr;
		checkRet(git_reference_peel((git_object **) &commit, reference_root, GIT_OBJ_COMMIT), "git_reference_peel2");
		const std::string version = GetVersion(Repo, reference_root);
		const std::string out = StringReplace(tmp, "$VERSION", version);
		buffer.assign(out.begin(), out.end());
		// first replace by git tag when exists, else parse from commit message

	} else {
		buffer.resize(Size);
		memcpy(buffer.data(), BlobBuf, Size);
	}
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
	git_tree_free(tree_root);
	git_reference_free(reference_root);
	git_libgit2_shutdown();
}
