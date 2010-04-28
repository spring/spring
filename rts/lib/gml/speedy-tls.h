//Copyright (C) 2008 Kevin Hoffman. See LICENSE for use and warranty disclaimer.
//Speedy TLS 1.0. Latest version at http://www.kevinjhoffman.com/
//Contains macros that can be used to very quickly (one instruction) access thread-local memory.

#ifndef __SPEEDY_TLS_H__
#define __SPEEDY_TLS_H__

//Allocates specified amount of memory (rounded up to nearest page).
//Results are undefined if you call this more than once on a thread.
//Returns system error or 0 on success.
int speedy_tls_init(int numBytes);

//Initializes the thread-local storage area to the memory region indicated. Size must be a multiple of the page size.
//Results are undefined if you call this more than once on a thread.
//Returns system error or 0 on success.
int speedy_tls_init_foraddr(void* addr, int numBytes);

//Returns the base address of the thread-local storage area or NULL if not initialized.
void* speedy_tls_get_base();

#if defined(__i386__)

#define __intel__
#define __speedy_tls_reg__ "%%fs"

#define speedy_tls_ptr_to_int32(x) ((int)(x))
#define speedy_tls_int32_to_ptr(x) ((void*)(x))

#elif defined(__x86_64__)

#define __intel__
#define __speedy_tls_reg__ "%%gs"

#define speedy_tls_ptr_to_int32(x) ((int)(long long int)(x))
#define speedy_tls_int32_to_ptr(x) ((void*)(long long int)(x))

#else
#error Fast TLS operations have not yet been implemented for this architecture. Please contribute.
#endif

//We can have a common assembly implementation for x86 architecture (uses fs on x86 and gs on x64).
#ifdef __intel__

//Use for information only on Intel only.
int speedy_tls_get_number_ldt_entries();

//-------------------------------------------------------------------------------------------------------------------------------------
//MACROS TO GET AND SET TLS VALUES (SAFE FOR SMP)
//-------------------------------------------------------------------------------------------------------------------------------------

#define speedy_tls_get_int8(base, index, scale, output) \
	__asm__ __volatile__ ( "movb " __speedy_tls_reg__ ":(%1,%2," #scale "), %0" : "=r"(output) : "r"(base), "r"(index) );
#define speedy_tls_get_int16(base, index, scale, output) \
	__asm__ __volatile__ ( "movw " __speedy_tls_reg__ ":(%1,%2," #scale "), %0" : "=r"(output) : "r"(base), "r"(index) );
#define speedy_tls_get_int32(base, index, scale, output) \
	__asm__ __volatile__ ( "movl " __speedy_tls_reg__ ":(%1,%2," #scale "), %0" : "=r"(output) : "r"(base), "r"(index) );

#define speedy_tls_put_int8(base, index, scale, input) \
	__asm__ __volatile__ ( "movb %0, " __speedy_tls_reg__ ":(%1,%2," #scale ")" : : "r"(input), "r"(base), "r"(index) );
#define speedy_tls_put_int16(base, index, scale, input) \
	__asm__ __volatile__ ( "movw %0, " __speedy_tls_reg__ ":(%1,%2," #scale ")" : : "r"(input), "r"(base), "r"(index) );
#define speedy_tls_put_int32(base, index, scale, input) \
	__asm__ __volatile__ ( "movl %0, " __speedy_tls_reg__ ":(%1,%2," #scale ")" : : "r"(input), "r"(base), "r"(index) );

//-------------------------------------------------------------------------------------------------------------------------------------
//MACROS THAT USE TLS VALUES (NOT SAFE FOR SMP)
//-------------------------------------------------------------------------------------------------------------------------------------

#define speedy_tls_add_int8(base, index, scale, input) \
	__asm__ __volatile__ ( "addb %0, " __speedy_tls_reg__ ":(%1,%2," #scale ")" : : "r"(input), "r"(base), "r"(index) );
#define speedy_tls_add_int16(base, index, scale, input) \
	__asm__ __volatile__ ( "addw %0, " __speedy_tls_reg__ ":(%1,%2," #scale ")" : : "r"(input), "r"(base), "r"(index) );
#define speedy_tls_add_int32(base, index, scale, input) \
	__asm__ __volatile__ ( "addl %0, " __speedy_tls_reg__ ":(%1,%2," #scale ")" : : "r"(input), "r"(base), "r"(index) );

#define speedy_tls_inc_int8(base, index, scale) \
	__asm__ __volatile__ ( "incb " __speedy_tls_reg__ ":(%0,%1," #scale ")" : : "r"(base), "r"(index) );
#define speedy_tls_inc_int16(base, index, scale) \
	__asm__ __volatile__ ( "incw " __speedy_tls_reg__ ":(%0,%1," #scale ")" : : "r"(base), "r"(index) );
#define speedy_tls_inc_int32(base, index, scale) \
	__asm__ __volatile__ ( "incl " __speedy_tls_reg__ ":(%0,%1," #scale ")" : : "r"(base), "r"(index) );

#define speedy_tls_dec_int8(base, index, scale) \
	__asm__ __volatile__ ( "decb " __speedy_tls_reg__ ":(%0,%1," #scale ")" : : "r"(base), "r"(index) );
#define speedy_tls_dec_int16(base, index, scale) \
	__asm__ __volatile__ ( "decw " __speedy_tls_reg__ ":(%0,%1," #scale ")" : : "r"(base), "r"(index) );
#define speedy_tls_dec_int32(base, index, scale) \
	__asm__ __volatile__ ( "decl " __speedy_tls_reg__ ":(%0,%1," #scale ")" : : "r"(base), "r"(index) );

//-------------------------------------------------------------------------------------------------------------------------------------
//MACROS THAT USE TLS VALUES (SAFE FOR SMP)
//-------------------------------------------------------------------------------------------------------------------------------------

#define speedy_tls_atomic_add_int8(base, index, scale, input) \
	__asm__ __volatile__ ( "lock; addb %0, " __speedy_tls_reg__ ":(%1,%2," #scale ")" : : "r"(input), "r"(base), "r"(index) );
#define speedy_tls_atomic_add_int16(base, index, scale, input) \
	__asm__ __volatile__ ( "lock; addw %0, " __speedy_tls_reg__ ":(%1,%2," #scale ")" : : "r"(input), "r"(base), "r"(index) );
#define speedy_tls_atomic_add_int32(base, index, scale, input) \
	__asm__ __volatile__ ( "lock; addl %0, " __speedy_tls_reg__ ":(%1,%2," #scale ")" : : "r"(input), "r"(base), "r"(index) );

#define speedy_tls_atomic_inc_int8(base, index, scale) \
	__asm__ __volatile__ ( "lock; incb " __speedy_tls_reg__ ":(%0,%1," #scale ")" : : "r"(base), "r"(index) );
#define speedy_tls_atomic_inc_int16(base, index, scale) \
	__asm__ __volatile__ ( "lock; incw " __speedy_tls_reg__ ":(%0,%1," #scale ")" : : "r"(base), "r"(index) );
#define speedy_tls_atomic_inc_int32(base, index, scale) \
	__asm__ __volatile__ ( "lock; incl " __speedy_tls_reg__ ":(%0,%1," #scale ")" : : "r"(base), "r"(index) );

#define speedy_tls_atomic_dec_int8(base, index, scale) \
	__asm__ __volatile__ ( "lock; decb " __speedy_tls_reg__ ":(%0,%1," #scale ")" : : "r"(base), "r"(index) );
#define speedy_tls_atomic_dec_int16(base, index, scale) \
	__asm__ __volatile__ ( "lock; decw " __speedy_tls_reg__ ":(%0,%1," #scale ")" : : "r"(base), "r"(index) );
#define speedy_tls_atomic_dec_int32(base, index, scale) \
	__asm__ __volatile__ ( "lock; decl " __speedy_tls_reg__ ":(%0,%1," #scale ")" : : "r"(base), "r"(index) );

//Gets value of TLS integer and saves in original_value, and then adds input to the TLS integer.
#define speedy_tls_atomic_get_and_add_int8(base, index, scale, input, original_value) \
	__asm__ __volatile__ ( "lock; xaddb %0, " __speedy_tls_reg__ ":(%2,%3," #scale ")" : "=r"(original_value) : "0"(input), "r"(base), "r"(index) );
#define speedy_tls_atomic_get_and_add_int16(base, index, scale, input, original_value) \
	__asm__ __volatile__ ( "lock; xaddw %0, " __speedy_tls_reg__ ":(%2,%3," #scale ")" : "=r"(original_value) : "0"(input), "r"(base), "r"(index) );
#define speedy_tls_atomic_get_and_add_int32(base, index, scale, input, original_value) \
	__asm__ __volatile__ ( "lock; xaddl %0, " __speedy_tls_reg__ ":(%2,%3," #scale ")" : "=r"(original_value) : "0"(input), "r"(base), "r"(index) );

//-------------------------------------------------------------------------------------------------------------------------------------
//MACROS THAT USE LOCAL VARIABLES INSTEAD OF TLS (NOT SMP SAFE)
//-------------------------------------------------------------------------------------------------------------------------------------

//Saves value of var in original_value and then adds num to var and saves in var.
#define speedy_local_get_and_add_int8(var, num, original_value) \
        __asm__ __volatile__ ( "xaddb %0,%1" : "=r" (original_value), "+m" (var) : "0" (num) : "memory" );
#define speedy_local_get_and_add_int16(var, num, original_value) \
        __asm__ __volatile__ ( "xaddw %0,%1" : "=r" (original_value), "+m" (var) : "0" (num) : "memory" );
#define speedy_local_get_and_add_int32(var, num, original_value) \
        __asm__ __volatile__ ( "xaddl %0,%1" : "=r" (original_value), "+m" (var) : "0" (num) : "memory" );

//-------------------------------------------------------------------------------------------------------------------------------------
//MACROS THAT USE LOCAL VARIABLES INSTEAD OF TLS (SMP SAFE)
//-------------------------------------------------------------------------------------------------------------------------------------

//Saves value of var in original_value and then adds num to var and saves in var.
#define speedy_local_atomic_get_and_add_int8(var, num, original_value) \
        __asm__ __volatile__ ( "lock; xaddb %0,%1" : "=r" (original_value), "+m" (var) : "0" (num) : "memory" );
#define speedy_local_atomic_get_and_add_int16(var, num, original_value) \
        __asm__ __volatile__ ( "lock; xaddw %0,%1" : "=r" (original_value), "+m" (var) : "0" (num) : "memory" );
#define speedy_local_atomic_get_and_add_int32(var, num, original_value) \
        __asm__ __volatile__ ( "lock; xaddl %0,%1" : "=r" (original_value), "+m" (var) : "0" (num) : "memory" );


#endif //__intel__

#endif //__SPEEDY_TLS_H__

