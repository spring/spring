/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CPUID_H
#define CPUID_H

#if defined(__GNUC__)
	#define _noinline __attribute__((__noinline__))
#else
	#define _noinline
#endif

#include <cstdint>

namespace springproc {
	_noinline void ExecCPUID(unsigned int* a, unsigned int* b, unsigned int* c, unsigned int* d);

	/** Class to detect the processor topology, more specifically,
	    for now it can detect the number of real (not hyper threaded
	    core.

	    It uses 'cpuid' instructions to query the information. It
	    implements both the new (i7 and above) and legacy (from P4 on
	    methods).

	    The implementation is done only for Intel processor for now, as at
	    the time of the writing it was not clear how to achieve a similar
	    result for AMD CMT multithreading.

	    This file is based on the following documentations from Intel:
	    - "Intel® 64 Architecture Processor Topology Enumeration"
	      (Kuo_CpuTopology_rc1.rh1.final.pdf)
	    - "Intel® 64 and IA-32 Architectures Software Developer’s Manual
	     Volume 3A: System Programming Guide, Part 1"
	      (64-ia-32-architectures-software-developer-vol-3a-part-1-manual.pdf)
	    - "Intel® 64 and IA-32 Architectures Software Developer’s Manual
	     Volume 2A: Instruction Set Reference, A-M"
	      (64-ia-32-architectures-software-developer-vol-2a-manual.pdf) */

	class CPUID {
	public:
		CPUID();

		/** Total number of cores in the system. This excludes SMT/HT
		    cores. */
		int GetNumPhysicalCores() const { return numPhysicalCores; }

		/** Total number of physical processor dies in the system. */
		int GetTotalNumPackages() const { return totalNumPackages; }

		uint64_t GetCoreAffinityMask(int x) const { return affinityMaskOfCores[x & (MAX_PROCESSORS - 1)]; }
		uint64_t GetPackageAffinityMask(int x) { return affinityMaskOfPackages[x & (MAX_PROCESSORS - 1)]; }

	private:
		void GetIdsAMD();
		void GetIdsIntel();
		void SetDefault();

	private:
		void GetIdsIntelEnumerate();

		void GetMasksIntelLeaf11Enumerate();
		void GetMasksIntelLeaf11();
		void GetMasksIntelLeaf1and4();

		uint32_t GetApicIdIntel();

	private:
		int numLogicalCores;
		int numPhysicalCores;
		int totalNumPackages;

		static constexpr int MAX_PROCESSORS = 64;

		/** Array of the size coreTotalNumber, containing for each
		    core the affinity mask. */
		uint64_t affinityMaskOfCores[MAX_PROCESSORS];
		uint64_t affinityMaskOfPackages[MAX_PROCESSORS];

		////////////////////////
		// Intel specific fields

		uint32_t processorApicIds[MAX_PROCESSORS];

		uint32_t shiftCore;
		uint32_t shiftPackage;

		uint32_t maskVirtual;
		uint32_t maskCore;
		uint32_t maskPackage;

		bool hasLeaf11;

		////////////////////////
		// AMD specific fields
	};

}

#endif // CPUID_H
