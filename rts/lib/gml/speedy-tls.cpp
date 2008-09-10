//Copyright (C) 2008 Kevin Hoffman. See LICENSE for use and warranty disclaimer.
//Speedy TLS 1.0. Latest version at http://www.kevinjhoffman.com/
//Contains macros that can be used to very quickly (one instruction) access thread-local memory.

#ifdef USE_GML
#if !defined(WIN32) && !defined(_WIN32) && !defined(__WIN32__)

#include "speedy-tls.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <sys/mman.h>
#include <fcntl.h>

//============================================================================================================== INTEL =====================
#ifdef __intel__

//Useful reading:
//  http://pdos.csail.mit.edu/6.828/2005/readings/i386/s05_01.htm
//  http://www.intel.com/products/processor/manuals/ (especially volume 3A)
//	Descriptor/Selector format for i386
//
//  http://en.wikipedia.org/wiki/X86_assembly_language#Segmented_addressing
//      More info on segmented addressing.
//
//  http://www.ibiblio.org/gferg/ldp/GCC-Inline-Assembly-HOWTO.html#s7
//  http://asm.sourceforge.net/articles/rmiyagi-inline-asm.txt  
//      Intros to inline assembly in GCC
//
//  http://www.microsoft.com/msj/archive/S2CE.aspx
//  http://en.wikipedia.org/wiki/Win32_Thread_Information_Block
//  http://younsi.blogspot.com/2007/05/show-tib-under-hood-from-matt-pietrek.html
//  
//      Might be able to port this to Windows or maybe use compiler TLS storage class and it does it for us (__declspec(thread)).

//NOTE: I don't use arch_prctl(ARCH_SET_GS, base) and instead use 32-bit segment selector because
//if you set a 64-bit segment selector it makes context switches more expensive!

//Generic representation of an x86 segment descriptor (32-bit mode and 64-bit mode w/ 32-bit syscall)
typedef struct i386_descriptor {
	unsigned int limit_0_15:16;
	unsigned int base_0_15:16;
	unsigned int base_16_23:8;
	unsigned int accessed:1;
	unsigned int contents:3;
	unsigned int is_normal:1;
	unsigned int protection:2;
	unsigned int present:1;
	unsigned int limit_16_19:4;
	unsigned int avl:1;
	unsigned int unknown_o:1;
	unsigned int seg_32bit:1;
	unsigned int limit_in_pages:1;
	unsigned int base_24_31:8;
};

#ifdef linux

#include <asm/ldt.h>
#include <asm/unistd.h>

//Define the modify_ldt function that will call this particular syscall.
_syscall3(int, modify_ldt, int, func, void *, ptr, unsigned long, bytecount)

#ifdef __x86_64__
#include <sys/prctl.h>
#include <asm/prctl.h>
extern "C" {
extern int arch_prctl(int code, unsigned long addr);
};
#endif

#else

#include <architecture/i386/table.h>
#include <i386/user_ldt.h>

//We still use this structure.
typedef struct modify_ldt_ldt_s {
	unsigned int  entry_number;
	unsigned long base_addr;
	unsigned int  limit;
	unsigned int  seg_32bit:1;
	unsigned int  contents:2;
	unsigned int  read_exec_only:1;
	unsigned int  limit_in_pages:1;
	unsigned int  seg_not_present:1;
	unsigned int  useable:1;
} modify_ldt_t;

#define MODIFY_LDT_CONTENTS_DATA	0
#define MODIFY_LDT_CONTENTS_STACK	1
#define MODIFY_LDT_CONTENTS_CODE	2

#define LDT_ENTRIES			8192
#define LDT_ENTRY_SIZE			8

//Fake the Linux syscall by implementing use BSD-syscall
int modify_ldt(int func, void *ptr, unsigned long bytecount)
{
	if (0 == func){
		//reading LDT
		int ret = i386_get_ldt(0, (union ldt_entry*)ptr, bytecount / LDT_ENTRY_SIZE);
		if (ret < 0){ return -1; }
		int realRet = (bytecount/LDT_ENTRY_SIZE)*LDT_ENTRY_SIZE;
		if (ret*LDT_ENTRY_SIZE < realRet){ realRet = ret*LDT_ENTRY_SIZE; }
		return realRet;
	} else {
		//writing one LDT
		//have to form actual binary representation of LDT
		modify_ldt_ldt_s* pLDT = (modify_ldt_ldt_s*)ptr;
		union ldt_entry newLDT;
		memset(&newLDT, 0, sizeof(newLDT));

		int ret=0;
		if (pLDT->seg_not_present){
			//Must use this other syntax to free the LDT entry.
			ret = i386_set_ldt(pLDT->entry_number, NULL, 1);
		} else {
			newLDT.data.limit00 = pLDT->limit & 0xFFFF;
			newLDT.data.base00  = pLDT->base_addr & 0xFFFF;
			newLDT.data.base16  = (pLDT->base_addr & 0xFF0000) >> 16;
			newLDT.data.type    = (pLDT->read_exec_only ?  DESC_DATA_RONLY : DESC_DATA_WRITE);
			newLDT.data.dpl     = 0x3;
			newLDT.data.present = (pLDT->seg_not_present ? 0 : 1);
			newLDT.data.limit16 = (pLDT->limit & 0x0F0000) >> 16;
			newLDT.data.stksz   = (pLDT->seg_32bit ? DESC_DATA_32B : DESC_DATA_16B);
			newLDT.data.granular= (pLDT->limit_in_pages ? 1 : 0);
			newLDT.data.base24  = (pLDT->base_addr & 0xFF000000) >> 24;
			//now do syscall
			ret = i386_set_ldt(pLDT->entry_number, &newLDT, 1);
		}
		if (ret < 0){ return -1; }
		if (pLDT->entry_number == LDT_AUTO_ALLOC){
			pLDT->entry_number = ret;
		}
		//printf("ALLOCATED LDT NUMBER %d\n", pLDT->entry_number);
		return 0;
	}
}

#endif //linux

#ifndef MAP_ANON
#define MAP_ANON MAP_ANONYMOUS
#endif //MAP_ANON

//-------------------------------------------------------------------------------

//Worker function to search for the next available ldt
int speedy_tls_get_next_avail_ldt(){
#ifdef linux
	char temp[LDT_ENTRIES*LDT_ENTRY_SIZE];
	int ret = modify_ldt(0, temp, sizeof(temp));
	if (ret < 0){
		perror("failed to read ldt to find next free spot");
		exit(-1);
	}
	int num=1;
	struct i386_descriptor* entries = (i386_descriptor*)temp;
	for (; entries[num].present; num++){}
	return num;
#else
	//Let the kernel choose the next free spot (may be more efficient)
	return LDT_AUTO_ALLOC;
#endif
}

//Determine how many LDT entries are still free.
int speedy_tls_get_number_ldt_entries(){  
	char temp[LDT_ENTRIES*LDT_ENTRY_SIZE];
	int ret = modify_ldt(0, temp, sizeof(temp));
	if (ret < 0){
		perror("failed to read ldt to get # of entries");
		exit(-1);
	}
	int maxToScan=ret / LDT_ENTRY_SIZE;
	int num=0;
	struct i386_descriptor* entries = (i386_descriptor*)temp;
	for (int i=0; i<maxToScan; i++){
		if (entries[i].present){ num++; }
	}
	return num;
}

void speedy_tls_onthread_ending(void* arg);
//Used to install a global hook when a pthread ends.
class speedy_tls_threadhook {
public:
	pthread_key_t m_ldt;
	pthread_key_t m_tlsbase;
	pthread_key_t m_tlslength;
	pthread_mutex_t m_lock;

	//We need to make sure we free the thread's LDT when the thread is done.
	static void onthread_ending(void* arg){
		speedy_tls_onthread_ending(arg);
	}

	speedy_tls_threadhook(){
		if (pthread_mutex_init(&m_lock, NULL) < 0){
			perror("speedy_tls failed to create global thread speedy_tls mutex");
			exit(-1);
		}
		if (pthread_key_create(&m_ldt, onthread_ending) < 0){
			perror("speedy_tls failed to create global thread termination hook");
			exit(-1);
		}
		if (pthread_key_create(&m_tlsbase, NULL) < 0){
			perror("speedy_tls failed to create TLS slot for private-TLS base address");
			exit(-1);
		}
		if (pthread_key_create(&m_tlslength, NULL) < 0){
			perror("speedy_tls failed to create TLS slot for private-TLS base address");
			exit(-1);
		}
	}
};
static speedy_tls_threadhook __speedy_tls_threadhook;

void speedy_tls_onthread_ending(void* arg){
	int ldt = speedy_tls_ptr_to_int32(arg);
	struct modify_ldt_ldt_s freeLDT;
	memset(&freeLDT, 0, sizeof(freeLDT));

	//Make sure we get base pointer to unmap memory as well.
	void* tlsBase = pthread_getspecific(__speedy_tls_threadhook.m_tlsbase);
	size_t tlsLength = (size_t)pthread_getspecific(__speedy_tls_threadhook.m_tlslength);

#ifdef __x86_64__
#ifdef __linux__
	if (ldt == -1){
		//We are using a 64-bit base, so reset it.
		arch_prctl(ARCH_SET_GS, (unsigned long)0);
	} else {
#endif
#endif
	//Make sure that we clear the segment register before freeing the LDT entry!
	__asm__ __volatile__
	(
		"movl %0,%%eax\n\t"
		"movw %%ax, " __speedy_tls_reg__ "\n\t"
		  : : "r"(0) : "%eax"
	);
#ifdef __x86_64__
#ifdef __linux__
	}
#endif
#endif

	if (ldt != -1){
		freeLDT.entry_number=ldt;
		freeLDT.base_addr = 0;
		freeLDT.limit = 0;
		freeLDT.seg_32bit = 1;
		freeLDT.contents = MODIFY_LDT_CONTENTS_DATA;
		freeLDT.read_exec_only=0;
		freeLDT.limit_in_pages=0;
		freeLDT.seg_not_present=1;
		freeLDT.useable = 0;
		int ret = modify_ldt(1, &freeLDT, sizeof(freeLDT));
		if (ret < 0){
			perror("WARNING: speedy_tls_cleanup_ldt failed to free ldt");
		}
	}

	//Finally, free the memory we used for the private TLS area.
	if (NULL != tlsBase){
		munmap(tlsBase, tlsLength);
	}
}

//-------------------------------------------------------------------------------

//Allocates specified amount of memory (rounded up to nearest page).
//Results are undefined if you call this more than once on a thread.
//Returns system error or 0 on success.
int speedy_tls_init(int numBytes)
{
	//Round up to nearest page size if neccessary.
	int pageSize = getpagesize();
	int realNumBytes = (numBytes + (pageSize-1)) & (~(pageSize-1)); 

#ifdef __x86_64__
  #ifdef __linux__
	void* mmapBase=NULL;
	int otherFlags = MAP_32BIT;
  #else
	//Have to try to allocate from lower 32-bits or TLS won't work.
	void* mmapBase=(void*)0x100000;
	int otherFlags = 0;
  #endif
#else
	void* mmapBase=NULL;
	int otherFlags = 0;
#endif
	void* tls=mmap(mmapBase, numBytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON | otherFlags, -1, 0);
#ifdef __x86_64__
	//Retry malloc obtaining non-32bit address if we have to go that route (results in slower context switches)
	if(tls==MAP_FAILED){
		tls=mmap(NULL, numBytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
	}
#endif
	if (tls==MAP_FAILED){
		int err = errno;
		perror("ERROR: fast-tls couldn't allocate memory for private-TLS segment");
		printf("       fast-tls failed to allocate %d bytes with mmap (errno=%d)\n", numBytes, err);
		return -1;
	}

	return speedy_tls_init_foraddr(tls, realNumBytes);
}

//Initializes the thread-local storage area to the memory region indicated. Size must be a multiple of the page size.
//Results are undefined if you call this more than once on a thread.
//Returns system error or 0 on success.
int speedy_tls_init_foraddr(void* addr, int numBytes)
{
	unsigned char* base = (unsigned char*)addr;
	struct modify_ldt_ldt_s newLDT;
	memset(&newLDT, 0, sizeof(newLDT));

#ifdef __x86_64__
	//if base address is not 32-bit, then have to use arch_prctl instead.
	if (addr > (void*)0xFFFFFFFF){
  #ifdef __linux__
		int ret = arch_prctl(ARCH_SET_GS, (unsigned long)addr);
		if (0 != ret){
			perror("speedy_tls_init_foraddr: failed to set 64-bit base address in GS register!");
			return -1;
		}
		pthread_setspecific(__speedy_tls_threadhook.m_ldt, speedy_tls_int32_to_ptr(-1));
		pthread_setspecific(__speedy_tls_threadhook.m_tlsbase, addr);
		pthread_setspecific(__speedy_tls_threadhook.m_tlslength, speedy_tls_int32_to_ptr(numBytes));
		return 0;
  #else
		printf("speedy_tls_init_foraddr: base address is 64-bit. Non-Linux OSes do not support this. Out of 32-bit TLS memory. Must fail!!!\n");
		errno = EFAULT;
		return -1;
  #endif
	}
#endif

	pthread_mutex_lock(&__speedy_tls_threadhook.m_lock);

	int newEntryNumber = speedy_tls_get_next_avail_ldt();
	newLDT.entry_number=newEntryNumber;
	newLDT.base_addr = (speedy_tls_ptr_to_int32(base) + (getpagesize()-1)) & (~(getpagesize()-1));
	newLDT.limit = numBytes / getpagesize();
	newLDT.seg_32bit = 1;
	newLDT.contents = MODIFY_LDT_CONTENTS_DATA;
	newLDT.read_exec_only=0;
	newLDT.limit_in_pages=1;
	newLDT.seg_not_present=0;
	newLDT.useable = 1;
	int ret = modify_ldt(1, &newLDT, sizeof(newLDT));
	pthread_mutex_unlock(&__speedy_tls_threadhook.m_lock);
	if (ret < 0){
		perror("speedy_tls_init_foraddr failed to set ldt");
		return -1;
	}

	__asm__ __volatile__
	(
		"movl %0,%%eax\n\t"
		"movw %%ax, " __speedy_tls_reg__ "\n\t"
		  : : "r"((newLDT.entry_number<<3) | (1 << 2) | 0x3) : "%eax"
	);

	//Make sure that we will deallocate this LDT when the thread ends
	pthread_setspecific(__speedy_tls_threadhook.m_ldt, speedy_tls_int32_to_ptr(newLDT.entry_number));
	//Remember the base address for our private TLS for this thread.
	pthread_setspecific(__speedy_tls_threadhook.m_tlsbase, speedy_tls_int32_to_ptr(newLDT.base_addr));
	pthread_setspecific(__speedy_tls_threadhook.m_tlslength, speedy_tls_int32_to_ptr(numBytes));

	return 0;
}

//Returns the base address of the thread-local storage area or NULL if not initialized.
void* speedy_tls_get_base()
{
	void* baseAddr=NULL;
	baseAddr = pthread_getspecific(__speedy_tls_threadhook.m_tlsbase);
	return baseAddr;
/**************************************** the old way was to get the selector, but doesn't work if we have a 64-bit base.
	//Get the current selector.
	unsigned int curSelector=0;
	__asm__ __volatile__
	(
		"xor %%eax,%%eax\n\t"
		"movw " __speedy_tls_reg__ ", %%ax\n\t"
		"movl %%eax, %0\n\t"
		  : "=r"(curSelector) : : "%eax"
	);
	//Make sure it's not into the GDT instead of the LDT.
	//(if it is, that means we didn't set it, so don't use it)
	if (!(curSelector & (1<<2))){
		return NULL;
	}
	//Get the index into the LDT.
	int idx = (curSelector >> 3);
	if (idx < 0 || idx >= LDT_ENTRIES){ return NULL; }
	
	char temp[LDT_ENTRIES*LDT_ENTRY_SIZE];
	int ret = modify_ldt(0, temp, sizeof(temp));
	if (ret < 0){
		perror("speedy_tls_get_base: failed to read ldt");
		return NULL;
	}
	struct i386_descriptor* entries = (i386_descriptor*)temp;
	if (!entries[idx].present){ return NULL; }
	unsigned int realBase = entries[idx].base_0_15 | (entries[idx].base_16_23 << 16) | (entries[idx].base_24_31 << 24);
	return (void*)realBase;	
*****************************************/
}

#else //__intel__

#error Implementation is not defined for non-Intel architecture right now.

#endif

#endif
#endif
