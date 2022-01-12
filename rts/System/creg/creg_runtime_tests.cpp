/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "creg_runtime_tests.h"
#include "creg_cond.h"
#include "System/SpringMath.h"
#include "System/StringUtil.h"
#include "System/Log/ILog.h"
#include <vector>
#include <map>
#include <set>
#include <cmath>


static void PreCregTest(const char* logmsg)
{
	LOG("%s", logmsg);
}

static bool PostCregTest(int fineClasses, int brokenClasses, int ignore = 0)
{
	if (brokenClasses > 0) {
		LOG_L(L_WARNING, "CREG Results: %i of %i classes are broken", brokenClasses, brokenClasses + fineClasses);
		if (ignore>0) { //FIXME: remove this
			LOG_L(L_ERROR, "%d broken classes of CREG ignored", ignore);
		}
		return (brokenClasses - ignore) <= 0;
	}

	LOG("CREG: Everything fine");
	return true;
}

/**
 * Tests if all CREG classes are complete
 */
static bool TestCregClasses1()
{
	PreCregTest("CREG: Test1 (Duplicated Members)");

	int fineClasses = 0;
	int brokenClasses = 0;

	for (const creg::Class* c: creg::System::GetClasses()) {
		const char* className = c->name;

		bool membersBroken = false;
		std::set<std::string> memberNames;

		for (const creg::Class* c_base = c; c_base; c_base = c_base->base()) {
			for (const creg::Class::Member& m: c_base->members) {
				if (std::string(m.name) != "Reserved" && !memberNames.insert(m.name).second) {
					LOG_L(L_WARNING, "  Duplicated member registration %s::%s %s", className, m.name, c_base->name);
					membersBroken = true;
				}
			}
		}

		if (membersBroken) {
			brokenClasses++;
		} else {
			//LOG( "CREG: Class %s fine, size %u", className, classSize);
			fineClasses++;
		}
	}

	return PostCregTest(fineClasses, brokenClasses);
}


static bool TestCregClasses2()
{
	PreCregTest("CREG: Test2 (Class' Sizes)");

	int fineClasses = 0;
	int brokenClasses = 0;

	for (const creg::Class* c: creg::System::GetClasses()) {
		const char* className = c->name;
		const size_t classSize = c->size;

		size_t cregSize = 1; // c++ class is min. 1byte large (part of sizeof definition)

		for (const creg::Class* c_base = c; c_base; c_base = c_base->base()) {
			for (const creg::Class::Member& m: c_base->members) {
				const size_t memberOffset = m.offset;
				const size_t typeSize = m.type->GetSize();
				cregSize = std::max(cregSize, memberOffset + typeSize);
			}
		}

		// alignment padding
		const float alignment = c->alignment;
		cregSize = std::ceil(cregSize / alignment) * alignment; //FIXME too simple, gcc's appending rules are ways more complicated

		if (cregSize != classSize) {
			brokenClasses++;
			LOG_L(L_WARNING, "  Missing member(s) in class %s, real size %i, creg size %i", className, int(classSize), int(cregSize));
			/*for (auto jt = classMembers.cbegin(); jt != classMembers.cend(); ++jt) {
				const std::string memberName   = (*jt)->name;
				const size_t      memberOffset = (*jt)->offset;
				const std::string typeName = (*jt)->type->GetName();
				const size_t      typeSize = (*jt)->type->GetSize();
				LOG_L(L_WARNING, "  member %20s, type %12s, offset %3u, size %u", memberName.c_str(), typeName.c_str(), memberOffset, typeSize);
			}*/
		} else {
			//LOG( "CREG: Class %s fine, size %u", className, classSize);
			fineClasses++;
		}
	}
	return PostCregTest(fineClasses, brokenClasses);
}

static bool TestCregClasses3()
{
	PreCregTest("CREG: Test3 (Missing Class Members)");

	int fineClasses = 0;
	int brokenClasses = 0;

	for (const creg::Class* c: creg::System::GetClasses()) {
		const char* className = c->name;
		const size_t classSize = c->size;

		creg::Class::Member alignmentFixMember;
		std::vector<const creg::Class::Member*> memberMap(classSize, nullptr);

		// create `map` of the class
		for (const creg::Class* c_base = c; c_base; c_base = c_base->base()) {
			for (const creg::Class::Member& m: c_base->members) {
				const size_t memberOffset = m.offset;
				const size_t typeSize = m.type->GetSize();

				if ((memberOffset + typeSize) > classSize) {
					LOG_L(L_WARNING, "  Member %s of class %s (typeSize=%lu) has offset=%lu greater than classSize=%lu", m.name, className, (unsigned long) typeSize, (unsigned long) memberOffset, (unsigned long) classSize);
				} else {
					for (int i = 0; i < typeSize; ++i) {
						memberMap[memberOffset + i] = &m;
					}
				}
			}
		}

		// class vtable
		if (c->base() || !c->GetDerivedClasses().empty() || c->hasVTable) {
			assert(memberMap.size() >= sizeof(void*));
			for (int i = 0; i < sizeof(void*); ++i) {
				if (memberMap[i] == nullptr)
					memberMap[i] = &alignmentFixMember;
			}
		}

		// class alignment itself
		for (int i = 1; i <= c->alignment; ++i) {
			if (memberMap[classSize - i] == nullptr)
				memberMap[classSize - i] = &alignmentFixMember;
		}

		// member alignments
		for (int i = 0; i < classSize; ++i) {
			const creg::Class::Member* m = memberMap[i];
			if (m != nullptr && (m != &alignmentFixMember)) {
				for (int j = 1; j <= m->alignment - 1; ++j) {
					if ((i - j) >= 0 && memberMap[i - j] == nullptr)
						memberMap[i - j] = &alignmentFixMember;
				}
			}
		}

		// find unregistered members
		bool unRegisteredMembers = false;
		const creg::Class::Member* prevMember = nullptr;
		const creg::Class::Member* nextMember = nullptr;
		for (int i = 0; i < classSize; ++i) {
			if (memberMap[i] == nullptr) {
				unRegisteredMembers = true;

				int holeSize = 0;
				nextMember = nullptr;
				for (int j = i + 1; j < classSize; ++j) {
					holeSize++;
					if (memberMap[j] != nullptr && (memberMap[j] != &alignmentFixMember)) {
						nextMember = memberMap[j];
						i = j;
						break;
					}
				}
				if (nextMember == nullptr)
					i = classSize - 1;

				if (prevMember && nextMember) {
					LOG_L(L_WARNING, "  Missing member(s) in class %s, between %s & %s, ~%i byte(s)", className, prevMember->name, nextMember->name, holeSize);
				} else
				if (nextMember) {
					LOG_L(L_WARNING, "  Missing member(s) in class %s, before %s, ~%i byte(s)", className, nextMember->name, holeSize);
				} else
				if (prevMember) {
					LOG_L(L_WARNING, "  Missing member(s) in class %s, after %s, ~%i byte(s)", className, prevMember->name, holeSize);
				} else
				{
					LOG_L(L_WARNING, "  Missing member(s) in class %s: none member is creg'ed", className);
				}
			}

			prevMember = memberMap[i];
			if (prevMember == &alignmentFixMember)
				prevMember = nullptr;
		}


		if (unRegisteredMembers) {
			brokenClasses++;

			/*std::string memberMapStr = className + " ByteMap: ";
			std::string memberMapLegend = "0: unknown\nx: alignment fixes\n";
			int n = 0;
			const creg::Class::Member* lastMember = NULL;
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
		} else {
			//LOG( "CREG: Class %s fine, size %u", className, classSize);
			fineClasses++;
		}
	}

	return PostCregTest(fineClasses, brokenClasses);
}


static bool TestCregClasses4()
{
	PreCregTest("CREG: Test4 (Incorrect Usage of CR_DECLARE vs. CR_DECLARE_STRUCT)");
	LOG("  CR_DECLARE_STRUCT is for plain structs without a vTable while");
	LOG("  CR_DECLARE should be used for classes that have virtual members.");
	LOG("  The latter adds a virtual method and a vTable if there isn't one already.");
	LOG("  This vTable has the size of a pointer and changes the size of the class.");
	LOG("  CR_DECLARE IS NOT USABLE FOR `COMPACT` STRUCTS AS FLOAT3/4, RECT, ...!");
	LOG("  Instead use CR_DECLARE_STRUCT for all inherited classes of those and");
	LOG("  register the members of their baseclass, too.");

	int fineClasses = 0;
	int brokenClasses = 0;

	for (const creg::Class* c: creg::System::GetClasses()) {
		const char* className = c->name;

		bool incorrectUsage = false;
		if (!c->isCregStruct) {
			if (!c->base() && c->GetDerivedClasses().empty()) {
				incorrectUsage = true;
				LOG_L(L_WARNING, "  Class %s has a vTable but isn't derived (should use CR_DECLARE_STRUCT)", className);
			}
		} else {
			if (c->base()) {
				incorrectUsage = true;
				LOG_L(L_WARNING, "  Class %s hasn't a vTable but is derived from %s (should use CR_DECLARE)", className, c->base()->name);
			} else
			if (!c->GetDerivedClasses().empty()) {
				incorrectUsage = true;
				LOG_L(L_WARNING, "  Class %s hasn't a vTable but is derived by %s (should use CR_DECLARE)", className, c->GetDerivedClasses()[0]->name);
			}
		}

		if (incorrectUsage) {
			brokenClasses++;
		} else {
			//LOG( "CREG: Class %s fine, size %u", className, classSize);
			fineClasses++;
		}
	}
	return PostCregTest(fineClasses, brokenClasses);
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
}
