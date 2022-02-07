/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "CpuID.h"
#include "libcpuid/libcpuid/libcpuid.h"
#include "System/MainDefines.h"
#include "System/Platform/Threading.h"
#include "System/Log/ILog.h"

#ifdef _MSC_VER
	#include <intrin.h>
#endif

#include "System/Threading/SpringThreading.h"
#include "System/UnorderedSet.hpp"

#include <cassert>


namespace springproc {
	enum {
		REG_EAX = 0,
		REG_EBX = 1,
		REG_ECX = 2,
		REG_EDX = 3,
		REG_CNT = 4,
	};


#if (__is_x86_arch__ == 1 && defined(__GNUC__))
	// function inlining breaks the asm
	// NOLINTNEXTLINE{readability-non-const-parameter}
	_noinline void ExecCPUID(unsigned int* a, unsigned int* b, unsigned int* c, unsigned int* d)
	{
		#ifndef __APPLE__
		__asm__ __volatile__(
			"cpuid"
			: "=a" (*a), "=b" (*b), "=c" (*c), "=d" (*d)
			: "0" (*a), "2" (*c)
		);

		#else

		#ifdef __x86_64__
			__asm__ __volatile__(
				"pushq %%rbx\n\t"
				"cpuid\n\t"
				"movl %%ebx, %1\n\t"
				"popq %%rbx"
				: "=a" (*a), "=r" (*b), "=c" (*c), "=d" (*d)
				: "0" (*a)
			);
		#else
			__asm__ __volatile__(
				"pushl %%ebx\n\t"
				"cpuid\n\t"
				"movl %%ebx, %1\n\t"
				"popl %%ebx"
				: "=a" (*a), "=r" (*b), "=c" (*c), "=d" (*d)
				: "0" (*a)
			);
		#endif
		#endif
	}

#elif (__is_x86_arch__ == 1 && defined(_MSC_VER) && (_MSC_VER >= 1310))

	void ExecCPUID(unsigned int* a, unsigned int* b, unsigned int* c, unsigned int* d)
	{
		int regs[REG_CNT] = {0};
		__cpuid(regs, *a);
		*a = regs[0];
		*b = regs[1];
		*c = regs[2];
		*d = regs[3];
	}

#else

	// no-op on other compilers / platforms (ARM has no cpuid instruction, etc)
	void ExecCPUID(unsigned int* a, unsigned int* b, unsigned int* c, unsigned int* d) {}
#endif


	CPUID::CPUID()
		: shiftCore(0)
		, shiftPackage(0)

		, maskVirtual(0)
		, maskCore(0)
		, maskPackage(0)

		, hasLeaf11(false)
	{
		uint32_t regs[REG_CNT] = {0, 0, 0, 0};

		SetDefault();

		// check for CPUID presence
		if (!cpuid_present()) {
			LOG_L(L_WARNING, "[CpuId] failed cpuid_present check");
			return;
		}

		struct cpu_raw_data_t raw;
		if (cpuid_get_raw_data(&raw) < 0) {
			LOG_L(L_WARNING, "[CpuId] error: %s", cpuid_error());
			return;
		}

		struct cpu_id_t data;
		// identify the CPU, using the given raw data.
		if (cpu_identify(&raw, &data) < 0) {
			LOG_L(L_WARNING, "[CpuId] error: %s", cpuid_error());
			return;
		}

		ExecCPUID(&regs[REG_EAX], &regs[REG_EBX], &regs[REG_ECX], &regs[REG_EDX]);

		switch (data.vendor) {
			case VENDOR_INTEL: {
				GetIdsIntel();
			} break;
			/*
			// this does nothing smart for now. Makes no sense to call
			case VENDOR_AMD: {
				GetIdsAMD();
			} break;
			*/
			case VENDOR_AMD: [[fallthrough]];
			default: {
				assert(data.num_logical_cpus == numLogicalCores);
				numPhysicalCores = data.num_cores;
			} break;
		}
	}

	// Function based on Figure 1 from Kuo_CpuTopology_rc1.rh1.final.pdf
	void CPUID::GetIdsIntel()
	{
		int maxLeaf = 0;
		uint32_t regs[REG_CNT] = {0, 0, 0, 0};

		ExecCPUID(&regs[REG_EAX], &regs[REG_EBX], &regs[REG_ECX], &regs[REG_EDX]);

		if ((maxLeaf = regs[REG_EAX]) >= 0xB) {
			regs[REG_EAX] = 11;
			regs[REG_ECX] = 0;
			ExecCPUID(&regs[REG_EAX], &regs[REG_EBX], &regs[REG_ECX], &regs[REG_EDX]);

			// Check if cpuid leaf 11 really exists
			if (regs[REG_EBX] != 0) {
				hasLeaf11 = true;
				LOG_L(L_DEBUG,"[CpuId] leaf 11 support");
				GetMasksIntelLeaf11();
				GetIdsIntelEnumerate();
				return;
			}
		}

		if (maxLeaf >= 4) {
			LOG_L(L_DEBUG,"[CpuId] leaf 4 support");
			GetMasksIntelLeaf1and4();
			GetIdsIntelEnumerate();
			return;
		}

		// Either it is a very old processor, or the cpuid instruction is disabled
		// from BIOS. Print a warning and use default processor number
		LOG_L(L_WARNING,"[CpuId] max cpuid leaf is less than 4! Using OS processor number.");
		SetDefault();
	}

	void CPUID::GetMasksIntelLeaf11Enumerate()
	{
		uint32_t currentLevel = 0;
		uint32_t regs[REG_CNT] = {1, 0, 0, 0};

		ExecCPUID(&regs[REG_EAX], &regs[REG_EBX], &regs[REG_ECX], &regs[REG_EDX]);

		regs[REG_EAX] = 11;
		regs[REG_ECX] = currentLevel++;

		ExecCPUID(&regs[REG_EAX], &regs[REG_EBX], &regs[REG_ECX], &regs[REG_EDX]);

		if ((regs[REG_EBX] & 0xFFFF) == 0)
			return;

		if (((regs[REG_ECX] >> 8) & 0xFF) == 1) {
			LOG_L(L_DEBUG,"[CpuId] SMT level found");
			shiftCore = regs[REG_EAX] & 0xf;
		} else {
			LOG_L(L_DEBUG,"[CpuId] No SMT level supported");
		}

		regs[REG_EAX] = 11;
		regs[REG_ECX] = currentLevel++;

		ExecCPUID(&regs[REG_EAX], &regs[REG_EBX], &regs[REG_ECX], &regs[REG_EDX]);

		if (((regs[REG_ECX] >> 8) & 0xFF) == 2) {
			LOG_L(L_DEBUG,"[CpuId] Core level found");
		    // Practically this is shiftCore + shiftVirtual so it is shiftPackage
		    shiftPackage = regs[REG_EAX] & 0xf;
		} else {
			LOG_L(L_DEBUG,"[CpuId] NO Core level supported");
		}
	}

	// Implementing "Sub ID Extraction Parameters for x2APIC ID" from
	// Kuo_CpuTopology_rc1.rh1.final.pdf

	void CPUID::GetMasksIntelLeaf11()
	{
		GetMasksIntelLeaf11Enumerate();

		// determined the shifts, now compute the masks
		maskVirtual =  ~((-1u) << shiftCore    );
		maskCore    = (~((-1u) << shiftPackage)) ^ maskVirtual;
		maskPackage =   ((-1u) << shiftPackage );
	}

	void CPUID::GetMasksIntelLeaf1and4()
	{
		uint32_t regs[REG_CNT] = {1, 0, 0, 0};

		unsigned maxAddressableLogical;
		unsigned maxAddressableCores;

		ExecCPUID(&regs[REG_EAX], &regs[REG_EBX], &regs[REG_ECX], &regs[REG_EDX]);

		maxAddressableLogical = (regs[REG_EBX] >> 18) & 0xff;

		regs[REG_EAX] = 4;
		regs[REG_ECX] = 0;
		ExecCPUID(&regs[REG_EAX], &regs[REG_EBX], &regs[REG_ECX], &regs[REG_EDX]);
		maxAddressableCores = ((regs[REG_EAX] >> 26) & 0x3f) + 1;
		shiftCore = (maxAddressableLogical / maxAddressableCores);

		{
			// compute the next power of 2 that is larger than shiftPackage
			int i = 0;
			while ((1u << i) < shiftCore)
				i++;
			shiftCore = i;
		}
		{
			int i = 0;
			while ((1u << i) < maxAddressableCores)
				i++;
			shiftPackage = i;
		}

		// determined the shifts, now compute the masks
		maskVirtual =  ~((-1u) << shiftCore    );
		maskCore    = (~((-1u) << shiftPackage)) ^ maskVirtual;
		maskPackage =   ((-1u) << shiftPackage );
	}

	void CPUID::GetIdsIntelEnumerate()
	{
		const auto oldAffinity = Threading::GetAffinity();

		for (int processor = 0; processor < numLogicalCores; processor++) {
			Threading::SetAffinity(1u << processor, true);
			spring::this_thread::yield();
			processorApicIds[processor] = GetApicIdIntel();
		}

		spring::unordered_set<uint32_t> cores;
		spring::unordered_set<uint32_t> packages;

		// determine the total number of cores
		for (int processor = 0; processor < numLogicalCores; processor++) {
			auto ret = cores.insert(processorApicIds[processor] >> shiftCore);
			if (!ret.second)
				continue;

			affinityMaskOfCores[cores.size() - 1] = (1lu << processor);
		}
		numPhysicalCores = cores.size();

		// determine the total number of packages
		for (int processor = 0; processor < numLogicalCores; processor++) {
			auto ret = packages.insert(processorApicIds[processor] >> shiftPackage);
			if (!ret.second)
				continue;

			affinityMaskOfPackages[packages.size() - 1] |= (1lu << processor);
		}

		totalNumPackages = packages.size();

		Threading::SetAffinity(oldAffinity);
	}

	uint32_t CPUID::GetApicIdIntel()
	{
		uint32_t regs[REG_CNT] = {0, 0, 0, 0};

		if (hasLeaf11) {
			regs[REG_EAX] = 11;
			ExecCPUID(&regs[REG_EAX], &regs[REG_EBX], &regs[REG_ECX], &regs[REG_EDX]);
			return regs[REG_EDX];
		}

		regs[REG_EAX] = 1;
		ExecCPUID(&regs[REG_EAX], &regs[REG_EBX], &regs[REG_ECX], &regs[REG_EDX]);
		return (regs[REG_EBX] >> 24);
	}

	void CPUID::GetIdsAMD()
	{
		#pragma message ("TODO")
	}

	void CPUID::SetDefault()
	{
		numLogicalCores = Threading::GetLogicalCpuCores();
		numPhysicalCores = numLogicalCores >> 1; //In 2022 HyperThreading is likely more common rather than not
		totalNumPackages = 1;

		// affinity mask is a uint64_t, but spring uses uint32_t
		assert(numLogicalCores <= MAX_PROCESSORS);

		static_assert(sizeof(affinityMaskOfCores   ) == (MAX_PROCESSORS * sizeof(affinityMaskOfCores   [0])), "");
		static_assert(sizeof(affinityMaskOfPackages) == (MAX_PROCESSORS * sizeof(affinityMaskOfPackages[0])), "");

		memset(affinityMaskOfCores   , 0, sizeof(affinityMaskOfCores   ));
		memset(affinityMaskOfPackages, 0, sizeof(affinityMaskOfPackages));
		memset(processorApicIds      , 0, sizeof(processorApicIds      ));

		// failed to determine CPU anatomy, just set affinity mask to (-1)
		for (int i = 0; i < numLogicalCores; i++) {
			affinityMaskOfCores[i] = affinityMaskOfPackages[i] = -1;
		}
	}

}
