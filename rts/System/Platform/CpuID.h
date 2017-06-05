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
		int getTotalNumCores() const { return totalNumCores; }

		/** Total number of physical processor dies in the system. */
		int getTotalNumPackages() const { return totalNumPackages; }

		uint64_t getCoreAffinityMask(int x) const { return affinityMaskOfCores[x & (maxProcessors - 1)]; }
		uint64_t getPackageAffinityMask(int x) { return affinityMaskOfPackages[x & (maxProcessors - 1)]; }

	private:
		void getIdsAmd();
		void getIdsIntel();
		void setDefault();

	private:
		void getIdsIntelEnumerate();

		void getMasksIntelLeaf11Enumerate();
		void getMasksIntelLeaf11();
		void getMasksIntelLeaf1and4();

		uint32_t getApicIdIntel();

	private:
		int numProcessors;
		int totalNumCores;
		int totalNumPackages;

		static constexpr int maxProcessors = 64;

		/** Array of the size coreTotalNumber, containing for each
		    core the affinity mask. */
		uint64_t affinityMaskOfCores[maxProcessors];
		uint64_t affinityMaskOfPackages[maxProcessors];

		////////////////////////
		// Intel specific fields

		uint32_t processorApicIds[maxProcessors];

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
