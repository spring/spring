/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "creg_runtime_tests.h"
#include "creg_cond.h"
#include "System/myMath.h"
#include "System/Util.h"
#include "System/Log/ILog.h"
#include <vector>
#include <map>
#include <set>


/**
 * Tests if all CREG classes are complete
 */
static bool TestCregClasses1()
{
	creg::System::InitializeClasses();
	LOG("CREG: Test1 (Duplicated Members)");

	int fineClasses = 0;
	int brokenClasses = 0;

	const std::vector<creg::Class*>& cregClasses = creg::System::GetClasses();
	for (std::vector<creg::Class*>::const_iterator it = cregClasses.begin(); it != cregClasses.end(); ++it) {
		const creg::Class* c = *it;

		const std::string& className = c->name;

		bool membersBroken = false;
		std::set<std::string> memberNames;

		const creg::Class* c_base = c;
		while (c_base){
			const std::vector<creg::Class::Member*>& classMembers = c_base->members;
			for (std::vector<creg::Class::Member*>::const_iterator jt = classMembers.begin(); jt != classMembers.end(); ++jt) {
				if (std::string((*jt)->name) != "Reserved" && !memberNames.insert((*jt)->name).second) {
					LOG_L(L_WARNING, "  Duplicated member registration %s::%s", className.c_str(), (*jt)->name);
					membersBroken = true;
				}
			}

			c_base = c_base->base;
		}

		if (membersBroken) {
			brokenClasses++;
		} else {
			//LOG( "CREG: Class %s fine, size %u", className.c_str(), classSize);
			fineClasses++;
		}
	}

	if (brokenClasses > 0) {
		LOG_L(L_WARNING, "CREG Results: %i of %i classes are broken", brokenClasses, brokenClasses + fineClasses);
		return false;
	}

	LOG("CREG: Everything fine");
	return true;
}


static bool TestCregClasses2()
{
	creg::System::InitializeClasses();
	LOG("CREG: Test2 (Class' Sizes)");

	int fineClasses = 0;
	int brokenClasses = 0;

	const std::vector<creg::Class*>& cregClasses = creg::System::GetClasses();
	for (std::vector<creg::Class*>::const_iterator it = cregClasses.begin(); it != cregClasses.end(); ++it) {
		const creg::Class* c = *it;

		const std::string& className = c->name;
		const size_t classSize = c->size;

		size_t cregSize = 1; // c++ class is min. 1byte large (part of sizeof definition)

		const creg::Class* c_base = c;
		while (c_base){
			const std::vector<creg::Class::Member*>& classMembers = c_base->members;
			for (std::vector<creg::Class::Member*>::const_iterator jt = classMembers.begin(); jt != classMembers.end(); ++jt) {
				const size_t memberOffset = (*jt)->offset;
				const size_t typeSize = (*jt)->type->GetSize();
				cregSize = std::max(cregSize, memberOffset + typeSize);
			}

			c_base = c_base->base;
		}

		// alignment padding
		const float alignment = c->alignment;
		cregSize = std::ceil(cregSize / alignment) * alignment; //FIXME too simple, gcc's appending rules are ways more complicated

		if (cregSize != classSize) {
			brokenClasses++;
			LOG_L(L_WARNING, "  Missing member(s) in class %s, real size %u, creg size %u", className.c_str(), classSize, cregSize);
			/*for (std::vector<creg::Class::Member*>::const_iterator jt = classMembers.begin(); jt != classMembers.end(); ++jt) {
				const std::string memberName   = (*jt)->name;
				const size_t      memberOffset = (*jt)->offset;
				const std::string typeName = (*jt)->type->GetName();
				const size_t      typeSize = (*jt)->type->GetSize();
				LOG_L(L_WARNING, "  member %20s, type %12s, offset %3u, size %u", memberName.c_str(), typeName.c_str(), memberOffset, typeSize);
			}*/
		} else {
			//LOG( "CREG: Class %s fine, size %u", className.c_str(), classSize);
			fineClasses++;
		}
	}

	if (brokenClasses > 0) {
		LOG_L(L_WARNING, "CREG Results: %i of %i classes are broken", brokenClasses, brokenClasses + fineClasses);
		return false;
	}

	LOG("CREG: Everything fine");
	return true;
}

static bool TestCregClasses3()
{
	creg::System::InitializeClasses();
	LOG("CREG: Test3 (Missing Class Members)");

	int fineClasses = 0;
	int brokenClasses = 0;

	const std::vector<creg::Class*>& cregClasses = creg::System::GetClasses();
	for (std::vector<creg::Class*>::const_iterator it = cregClasses.begin(); it != cregClasses.end(); ++it) {
		const creg::Class* c = *it;

		const std::string& className = c->name;
		const size_t classSize = c->size;

		creg::Class::Member alignmentFixMember;
		std::vector<creg::Class::Member*> memberMap(classSize, NULL);

		// create `map` of the class
		const creg::Class* c_base = c;
		while (c_base){
			const std::vector<creg::Class::Member*>& classMembers = c_base->members;
			for (std::vector<creg::Class::Member*>::const_iterator jt = classMembers.begin(); jt != classMembers.end(); ++jt) {
				const size_t memberOffset = (*jt)->offset;
				const size_t typeSize = (*jt)->type->GetSize();

				for (int i = 0; i < typeSize; ++i) {
					memberMap[memberOffset + i] = (*jt);
				}
			}

			c_base = c_base->base;
		}

		// class vtable
		if (c->base || !c->derivedClasses.empty() || c->binder->hasVTable) {
			assert(memberMap.size() >= sizeof(void*));
			for (int i = 0; i < sizeof(void*); ++i) {
				if (!memberMap[i])
					memberMap[i] = &alignmentFixMember;
			}
		}

		// class alignment itself
		for (int i = 1; i <= c->alignment; ++i) {
			if (!memberMap[classSize - i])
				memberMap[classSize - i] = &alignmentFixMember;
		}

		// member alignments
		for (int i = 0; i < classSize; ++i) {
			creg::Class::Member* c = memberMap[i];
			if (c && (c != &alignmentFixMember)) {
				for (int j = 1; j <= c->alignment - 1; ++j) {
					if ((i - j) >= 0 && !memberMap[i - j])
						memberMap[i - j] = &alignmentFixMember;
				}
			}
		}

		// find unregistered members
		bool unRegisteredMembers = false;
		creg::Class::Member* prevMember = NULL;
		creg::Class::Member* nextMember = NULL;
		for (int i = 0; i < classSize; ++i) {
			if (!memberMap[i]) {
				unRegisteredMembers = true;

				int holeSize = 0;
				nextMember = NULL;
				for (int j = i + 1; j < classSize; ++j) {
					holeSize++;
					if (memberMap[j] && (memberMap[j] != &alignmentFixMember)) {
						nextMember = memberMap[j];
						i = j;
						break;
					}
				}
				if (!nextMember) {
					i = classSize;
				}

				if (prevMember && nextMember) {
					LOG_L(L_WARNING, "  Missing member(s) in class %s, between %s & %s, ~%i byte(s)", className.c_str(), prevMember->name, nextMember->name, holeSize);
				} else
				if (nextMember) {
					LOG_L(L_WARNING, "  Missing member(s) in class %s, before %s, ~%i byte(s)", className.c_str(), nextMember->name, holeSize);
				} else
				if (prevMember) {
					LOG_L(L_WARNING, "  Missing member(s) in class %s, after %s, ~%i byte(s)", className.c_str(), prevMember->name, holeSize);
				} else
				{
					LOG_L(L_WARNING, "  Missing member(s) in class %s: none member is creg'ed", className.c_str());
				}
			}

			prevMember = memberMap[i];
			if (prevMember == &alignmentFixMember)
				prevMember = NULL;
		}

		if (unRegisteredMembers) {
			/*std::string memberMapStr = className + " ByteMap: ";
			std::string memberMapLegend = "0: unknown\nx: alignment fixes\n";
			int n = 0; creg::Class::Member* lastMember = NULL;
			for (int i = 0; i < classSize; ++i) {
				if (!memberMap[i]) {
					memberMapStr += "0";
				} else
				if (memberMap[i] == &alignmentFixMember) {
					memberMapStr += "x";
				} else
				{
					if (memberMap[i] != lastMember) {
						n %= 9; n++;
						lastMember = memberMap[i];
						memberMapLegend += IntToString(n) + ": " + lastMember->name + " (type: " + lastMember->type->GetName() + " alignment: " + IntToString(lastMember->alignment) + ")\n";
					}
					memberMapStr += IntToString(n);
				}
			}

			memberMapStr += "\n";
			memberMapStr += memberMapLegend;
			LOG_L(L_WARNING, "  %s", memberMapStr.c_str());*/
			brokenClasses++;
		} else {
			//LOG( "CREG: Class %s fine, size %u", className.c_str(), classSize);
			fineClasses++;
		}
	}

	if (brokenClasses > 0) {
		LOG_L(L_WARNING, "CREG Results: %i of %i classes are broken", brokenClasses, brokenClasses + fineClasses);
		return false;
	}

	LOG("CREG: Everything fine");
	return true;
}


static bool TestCregClasses4()
{
	creg::System::InitializeClasses();
	LOG("CREG: Test4 (Incorrect Usage of CR_DECLARE vs. CR_DECLARE_STRUCT)");
	LOG("  Note, CR_DECLARE_STRUCT is for plain structs only without a vTable and/or inheritance.");
	LOG("  While CR_DECLARE should be used for derived classes (both sub & base!).");
	LOG("  It adds a virtual method! And so a vTable if there isn't one already.");
	LOG("  This vTable has the size of a pointer and changes the size of the class.");
	LOG("  AND SO CR_DECLARE IS NOT USABLE FOR `COMPACT` STRUCTS AS FLOAT3/4, RECT, ...!");
	LOG("  Instead use CR_DECLARE_STRUCT for all inherited classes of those and register the members of their baseclass, too.");

	int fineClasses = 0;
	int brokenClasses = 0;

	const std::vector<creg::Class*>& cregClasses = creg::System::GetClasses();
	for (std::vector<creg::Class*>::const_iterator it = cregClasses.begin(); it != cregClasses.end(); ++it) {
		const creg::Class* c = *it;
		const std::string& className = c->name;

		bool incorrectUsage = false;
		if (c->binder->hasVTable) {
			if (!c->base && c->derivedClasses.empty()) {
				incorrectUsage = true;
				LOG_L(L_WARNING, "  Class %s has a vTable but isn't derived (should use CR_DECLARE_STRUCT)", className.c_str());
			}
		} else {
			if (c->base) {
				incorrectUsage = true;
				LOG_L(L_WARNING, "  Class %s hasn't a vTable but is derived from %s (should use CR_DECLARE)", className.c_str(), c->base->name.c_str());
			} else
			if (!c->derivedClasses.empty()) {
				incorrectUsage = true;
				LOG_L(L_WARNING, "  Class %s hasn't a vTable but is derived by %s (should use CR_DECLARE)", className.c_str(), c->derivedClasses[0]->name.c_str());
			}
		}

		if (incorrectUsage) {
			brokenClasses++;
		} else {
			//LOG( "CREG: Class %s fine, size %u", className.c_str(), classSize);
			fineClasses++;
		}
	}

	if (brokenClasses > 0) {
		LOG_L(L_WARNING, "CREG Results: %i of %i classes are broken", brokenClasses, brokenClasses + fineClasses);
		return false;
	}

	LOG("CREG: Everything fine");
	return true;
}


namespace creg {
	bool RuntimeTest()
	{
		bool res = true;
		res &= TestCregClasses1();
		res &= TestCregClasses2();
		res &= TestCregClasses3();
		res &= TestCregClasses4();
		return res;
	}
};
