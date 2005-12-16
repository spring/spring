#ifndef __ARCHIVE_FACTORY_H
#define __ARCHIVE_FACTORY_H

#include <string>
#include "ArchiveBase.h"

using namespace std;

class CArchiveFactory
{
public:
	static bool IsArchive(const string& fileName);
	static CArchiveBase* OpenArchive(const string& fileName);
};

#endif
