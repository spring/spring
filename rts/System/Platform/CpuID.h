/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CPUID_H
#define CPUID_H

#if defined(__GNUC__)
	#define _noinline __attribute__((__noinline__))
#else
	#define _noinline
#endif

#include <stdint.h>

namespace springproc {
	_noinline void ExecCPUID(unsigned int* a, unsigned int* b, unsigned int* c, unsigned int* d);

	class CpuId {
	 private:
		void getIdsAmd();
		void getIdsIntel();
		void setDefault();

		int nbProcessors;
		int coreTotalNumber;
		int packageTotalNumber;

		uint64_t *affinityMaskOfCores;
		uint64_t *affinityMaskOfPackages;

		////////////////////////
		// Intel specific fields

		uint32_t* processorApicIds;

		void getIdsIntelEnumerate();

		void getMasksIntelLeaf11Enumerate();
		void getMasksIntelLeaf11();
		void getMasksIntelLeaf1and4();

		uint32_t getApicIdIntel();

		uint32_t shiftCore;
		uint32_t shiftPackage;

		uint32_t maskVirtual;
		uint32_t maskCore;
		uint32_t maskPackage;

		bool hasLeaf11;

		////////////////////////
		// AMD specific fields

		////////////////////////
	 public:
		 CpuId();

		int getCoreTotalNumber();
		int getPackageTotalNumber();

		uint64_t getAffinityMaskOfCore(int x);
		uint64_t getAffinityMaskOfPackage(int x);
	};

}

#endif // CPUID_H
