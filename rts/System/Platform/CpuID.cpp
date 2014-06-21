/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "CpuID.h"
#include "System/Platform/Linux/Threading.h"
#include "System/Log/ILog.h"
//#include <cstddef>
#ifdef _MSC_VER
	#include <intrin.h>
#endif

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <algorithm>
#include <sched.h>
#include <stdio.h>
#include <set>


namespace springproc {

#if defined(__GNUC__)
	// function inlining breaks this
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
#elif defined(_MSC_VER) && (_MSC_VER >= 1310)
	void ExecCPUID(unsigned int* a, unsigned int* b, unsigned int* c, unsigned int* d)
	{
		int features[4];
		__cpuid(features, *a);
		*a=features[0];
		*b=features[1];
		*c=features[2];
		*d=features[3];
	}
#else
	// no-op on other compilers
	void ExecCPUID(unsigned int* a, unsigned int* b, unsigned int* c, unsigned int* d)
	{
	}
#endif

	CpuId::CpuId(): shiftCore(0), shiftPackage(0),
	  maskVirtual(0),  maskCore(0), maskPackage(0),
	  hasLeaf11(false)
	{
	
		nbProcessors = Threading::NumProcessors();
		affinityMaskOfCores = new uint64_t[nbProcessors];
		affinityMaskOfPackages = new uint64_t[nbProcessors];
		processorApicIds = new uint32_t[nbProcessors];

		unsigned int eax = 0, ebx = 0, ecx = 0, edx = 0;
		eax = 0;
		ExecCPUID(&eax, &ebx, &ecx, &edx);

		// Check if it Intel
		if (ebx == 0x756e6547) {	// "Genu", from "GenuineIntel"
			getIdsIntel();
		} else if (ebx == 0x68747541) {	// "htuA" from "AuthenticAMD"
			// TODO: AMD has also something similar to SMT (called CMT) in Bulldozer
			// microarchitecture (FX processors). 
		}
	}

	// Function based on Figure 1 from Kuo_CpuTopology_rc1.rh1.final.pdf
	void CpuId::getIdsIntel()
	{
		int maxLeaf;
		unsigned int eax = 0, ebx = 0, ecx = 0, edx = 0;
		eax = 0;
		ExecCPUID(&eax, &ebx, &ecx, &edx);

		maxLeaf = eax;
		if (maxLeaf >= 0xB) {
			eax = 11;
			ecx = 0;
			ExecCPUID(&eax, &ebx, &ecx, &edx);

			// Check if cpuid leaf 11 really exists
			if (ebx != 0) {
				hasLeaf11 = true;
				LOG_L(L_DEBUG,"cpuid leaf 11 support\n");
				getMasksIntelLeaf11();
				getIdsIntelEnumerate();
				return;
			}
		}

		if (maxLeaf >= 4) {
			LOG_L(L_DEBUG,"cpuid leaf 4 support\n");
			getMasksIntelLeaf1and4();
			getIdsIntelEnumerate();
			return;
		}
		// Either it is a very old processor, or the cpuid instruction is disabled
		// from BIOS. Print a warning an use processor number
		LOG_L(L_WARNING,"max cpuid leaf is less than 4! Using OS processor number.\n");
		setDefault();
	}

	void CpuId::getMasksIntelLeaf11Enumerate()
	{
		uint32_t currentLevel = 0;
		unsigned int eax = 0, ebx = 0, ecx = 0, edx = 0;
		eax = 1;

		ExecCPUID(&eax, &ebx, &ecx, &edx);

		eax = 11;
		ecx = currentLevel;
		currentLevel++;
		ExecCPUID(&eax, &ebx, &ecx, &edx);

		if ((ebx && 0xFFFF) == 0)
			return;

		if (((ecx >> 8) & 0xFF) == 1) {
			LOG_L(L_DEBUG,"SMT level found\n");
			shiftCore = eax & 0xf;
		} else {
			LOG_L(L_DEBUG,"No SMT level supported\n");
		}

		eax = 11;
		ecx = currentLevel;
		currentLevel++;
		ExecCPUID(&eax, &ebx, &ecx, &edx);

		if (((ecx >> 8) & 0xFF) == 2) {
			LOG_L(L_DEBUG,"Core level found\n");
			    // Practically this is shiftCore + shitVirtual so it is shiftPackage
			    shiftPackage = eax & 0xf;
		} else {
			LOG_L(L_DEBUG,"NO Core level supported\n");
		}
	}

	// Implementing "Sub ID Extraction Parameters for x2APIC ID" from 
	// Kuo_CpuTopology_rc1.rh1.final.pdf

	void CpuId::getMasksIntelLeaf11()
	{
		getMasksIntelLeaf11Enumerate();

		// We determined the shifts now compute the masks
		maskVirtual = ~((-1) << shiftCore);
		maskCore = (~((-1) << shiftPackage)) ^ maskVirtual;
		maskPackage = (-1) << shiftPackage;
	}

	void CpuId::getMasksIntelLeaf1and4()
	{
		int i;

		unsigned int eax = 0, ebx = 0, ecx = 0, edx = 0;

		unsigned maxAddressableLogical;
		unsigned maxAddressableCores;

		eax = 1;
		ExecCPUID(&eax, &ebx, &ecx, &edx);

		maxAddressableLogical = (ebx >> 18) & 0xff;

		eax = 4;
		ecx = 0;
		ExecCPUID(&eax, &ebx, &ecx, &edx);
		maxAddressableCores = ((eax >> 26) & 0x3f) + 1;

		shiftCore = (maxAddressableLogical / maxAddressableCores);
		// We compute the next power of 2 that is larger than shiftPackage
		i = 0;
		while ((1 << i) < shiftCore)
			i++;
		shiftCore = i;

		i = 0;
		while ((1 << i) < maxAddressableCores)
			i++;
		shiftPackage = i;

		// We determined the shifts now compute the masks
		maskVirtual = ~((-1) << shiftCore);
		maskCore = (~((-1) << shiftPackage)) ^ maskVirtual;
		maskPackage = (-1) << shiftPackage;
	}

	void CpuId::getIdsIntelEnumerate()
	{
		int processorNumber = Threading::NumProcessors();
		assert(processorNumber <= 64);	// as the affinity mask is a uint64_t
		for (size_t processor = 0; processor < processorNumber; processor++) {
			Threading::SetThreadAffinityMask(((uint64_t) 1) << processor);
			processorApicIds[processor] = getApicIdIntel();
		}

		std::pair < std::set < uint32_t >::iterator, bool > ret;
		// We determine the total numbers of cores cores
		std::set < uint32_t > cores;
		for (size_t processor = 0; processor < processorNumber; processor++) {
			ret = cores.insert(processorApicIds[processor] >> shiftCore);
			if (ret.second) {
				affinityMaskOfCores[cores.size() - 1] =
				    ((uint64_t) 1) << processor;
			}
		}
		coreTotalNumber = cores.size();

		// We determine the total numbers of packages cores
		std::set < uint32_t > packages;
		for (size_t processor = 0; processor < processorNumber; processor++) {
			ret =
			    packages.
			    insert(processorApicIds[processor] >> shiftPackage);
			if (ret.second) {
				affinityMaskOfPackages[packages.size() - 1] =
				    ((uint64_t) 1) << processor;
			}
		}

		packageTotalNumber = packages.size();
	}

	uint32_t CpuId::getApicIdIntel()
	{
		unsigned int eax = 0, ebx = 0, ecx = 0, edx = 0;

		if (hasLeaf11) {
			eax = 11;
			ExecCPUID(&eax, &ebx, &ecx, &edx);
			return edx;
		} else {
			eax = 1;
			ExecCPUID(&eax, &ebx, &ecx, &edx);
			return (unsigned char)(ebx >> 24);
		}
	}

	void CpuId::getIdsAmd()
	{
		LOG_L(L_WARNING,"ht/smt/cmt detection for AMD is not implemented! Using OS processor number.\n");
		setDefault();
	}

	int CpuId::getCoreTotalNumber()
	{
		return coreTotalNumber;
	}

	int CpuId::getPackageTotalNumber()
	{
		return packageTotalNumber;
	}

	uint64_t CpuId::getAffinityMaskOfCore(int x)
	{
		return affinityMaskOfCores[x];
	}

	uint64_t CpuId::getAffinityMaskOfPackage(int x)
	{
		return affinityMaskOfPackages[x];
	}

	void CpuId::setDefault()
	{
		coreTotalNumber = Threading::NumProcessors();
		packageTotalNumber = Threading::NumProcessors();

		// As we could not determine anything just set affinity mask to (-1)
		for (int i = 0; i < Threading::NumProcessors(); i++) {
			affinityMaskOfCores[i] = affinityMaskOfPackages[i] = -1;
		}
	}

}
