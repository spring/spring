/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
/*

			       B G E T

			   Buffer allocator

    Designed and implemented in April of 1972 by John Walker, based on the
    Case Algol OPRO$ algorithm implemented in 1966.

    Reimplemented in 1975 by John Walker for the Interdata 70.
    Reimplemented in 1977 by John Walker for the Marinchip 9900.
    Reimplemented in 1982 by Duff Kurland for the Intel 8080.

    Portable C version implemented in September of 1990 by an older, wiser
    instance of the original implementor.

    Souped up and/or weighed down  slightly  shortly  thereafter  by  Greg
    Lutz.

    AMIX  edition, including the new compaction call-back option, prepared
    by John Walker in July of 1992.

    Bug in built-in test program fixed, ANSI compiler warnings eradicated,
    buffer pool validator  implemented,  and  guaranteed  repeatable  test
    added by John Walker in October of 1995.

		Messed up by Stefan Johansson 2005

    This program is in the public domain.

     1. This is the book of the generations of Adam.   In the day that God
	created man, in the likeness of God made he him;
     2. Male and female created he them;  and  blessed	them,  and  called
	their name Adam, in the day when they were created.
     3. And  Adam  lived  an hundred and thirty years,	and begat a son in
	his own likeness, and after his image; and called his name Seth:
     4. And the days of  Adam  after  he  had  begotten  Seth  were  eight
	hundred years: and he begat sons and daughters:
     5. And  all  the  days  that Adam lived were nine	hundred and thirty
	years: and he died.
     6. And Seth lived an hundred and five years, and begat Enos:
     7. And Seth lived after he begat Enos eight hundred and seven  years,
	and begat sons and daughters:
     8.  And  all the days of Seth were nine hundred and twelve years: and
	 he died.
     9. And Enos lived ninety years, and begat Cainan:
    10. And Enos lived after he begat  Cainan eight  hundred  and  fifteen
	years, and begat sons and daughters:
    11. And  all  the days of Enos were nine hundred  and five years:  and
	he died.
    12. And Cainan lived seventy years and begat Mahalaleel:
    13. And Cainan lived  after he  begat  Mahalaleel  eight  hundred  and
	forty years, and begat sons and daughters:
    14. And  all the days of Cainan were nine  hundred and ten years:  and
	he died.
    15. And Mahalaleel lived sixty and five years, and begat Jared:
    16. And Mahalaleel lived  after  he  begat	Jared  eight  hundred  and
	thirty years, and begat sons and daughters:
    17. And  all  the  days  of Mahalaleel  were eight hundred	ninety and
	five years: and he died.
    18. And Jared lived an hundred sixty and  two  years,   and  he  begat
	Enoch:
    19. And  Jared  lived  after he begat Enoch  eight hundred years,  and
	begat sons and daughters:
    20. And all the days of Jared  were nine hundred sixty and two  years:
	and he died.
    21. And Enoch lived sixty and five years, and begat Methuselah:
    22. And  Enoch  walked   with  God	after  he  begat Methuselah  three
	hundred years, and begat sons and daughters:
    23. And all the days of  Enoch  were  three  hundred  sixty  and  five
	years:
    24. And Enoch walked with God: and he was not; for God took him.
    25. And  Methuselah  lived	an  hundred  eighty and  seven years,  and
	begat Lamech.
    26. And Methuselah lived after he  begat Lamech seven  hundred  eighty
	and two years, and begat sons and daughters:
    27. And  all the days of Methuselah  were nine hundred  sixty and nine
	years: and he died.
    28. And Lamech lived an hundred eighty  and two  years,  and  begat  a
	son:
    29. And  he called his name Noah, saying,  This same shall	comfort us
	concerning  our  work and toil of our hands, because of the ground
	which the LORD hath cursed.
    30. And  Lamech  lived  after  he begat Noah  five hundred	ninety and
	five years, and begat sons and daughters:
    31. And all the days of Lamech were  seven hundred seventy	and  seven
	years: and he died.
    32. And  Noah  was five hundred years old:	and Noah begat Shem,  Ham,
	and Japheth.

    And buffers begat buffers, and links begat	links,	and  buffer  pools
    begat  links  to chains of buffer pools containing buffers, and lo the
    buffers and links and pools of buffers and pools of links to chains of
    pools  of  buffers were fruitful and they multiplied and the Operating
    System looked down upon them and said that it was Good.


    INTRODUCTION
    ============

    BGET  is a comprehensive memory allocation package which is easily
    configured to the needs of an application.	BGET is  efficient  in
    both  the  time  needed to allocate and release buffers and in the
    memory  overhead  required	for  buffer   pool   management.    It
    automatically    consolidates   contiguous	 space	 to   minimise
    fragmentation.  BGET is configured	by  compile-time  definitions,
    Major options include:

	*   A  built-in  test  program	to  exercise  BGET   and
	    demonstrate how the various functions are used.

        *   Allocation  by  either the "first fit" or "best fit"
	    method.

	*   Wiping buffers at release time to catch  code  which
	    references previously released storage.

	*   Built-in  routines to dump individual buffers or the
	    entire buffer pool.

	*   Retrieval of allocation and pool size statistics.

	*   Quantisation of buffer sizes to a power  of  two  to
	    satisfy hardware alignment constraints.

	*   Automatic  pool compaction, growth, and shrinkage by
	    means of call-backs to user defined functions.

    Applications  of  BGET  can  range	from  storage  management   in
    ROM-based  embedded programs to providing the framework upon which
    a  multitasking  system  incorporating   garbage   collection   is
    constructed.   BGET  incorporates  extensive  internal consistency
    checking using the <assert.h> mechanism; all these checks  can  be
    turned off by compiling with NDEBUG defined, yielding a version of
    BGET with minimal size and maximum speed.

    The  basic	algorithm  underlying  BGET  has withstood the test of
    time;  more  than  25  years   have   passed   since   the	 first
    implementation  of	this  code.  And yet, it is substantially more
    efficient than the native allocation  schemes  of  many  operating
    systems: the Macintosh and Microsoft Windows to name two, on which
    programs have obtained substantial speed-ups by layering  BGET  as
    an application level memory manager atop the underlying system's.

    BGET has been implemented on the largest mainframes and the lowest
    of	microprocessors.   It  has served as the core for multitasking
    operating systems, multi-thread applications, embedded software in
    data  network switching processors, and a host of C programs.  And
    while it has accreted flexibility and additional options over  the
    years,  it	remains  fast, memory efficient, portable, and easy to
    integrate into your program.


    BGET IMPLEMENTATION ASSUMPTIONS
    ===============================

    BGET is written in as portable a dialect of C  as  possible.   The
    only   fundamental	 assumption   about  the  underlying  hardware
    architecture is that memory is allocated is a linear  array  which
    can  be  addressed  as a vector of C "char" objects.  On segmented
    address space architectures, this generally means that BGET should
    be used to allocate storage within a single segment (although some
    compilers	simulate   linear   address   spaces   on    segmented
    architectures).   On  segmented  architectures,  then, BGET buffer
    pools  may not be larger than a segment, but since BGET allows any
    number of separate buffer pools, there is no limit	on  the  total
    storage  which  can  be  managed,  only  on the largest individual
    object which can be allocated.  Machines  with  a  linear  address
    architecture,  such  as  the VAX, 680x0, Sparc, MIPS, or the Intel
    80386 and above in native mode, may use BGET without restriction.


    GETTING STARTED WITH BGET
    =========================

    Although BGET can be configured in a multitude of fashions,  there
    are  three	basic  ways  of  working  with	BGET.	The  functions
    mentioned below are documented in the following  section.	Please
    excuse  the  forward  references which are made in the interest of
    providing a roadmap to guide you  to  the  BGET  functions  you're
    likely to need.

    Embedded Applications
    ---------------------

    Embedded applications  typically  have  a  fixed  area  of	memory
    dedicated  to  buffer  allocation (often in a separate RAM address
    space distinct from the ROM that contains  the  executable	code).
    To	use  BGET in such an environment, simply call bpool() with the
    start address and length of the buffer  pool  area	in  RAM,  then
    allocate  buffers  with  bget()  and  release  them  with  brel().
    Embedded applications with very limited RAM but abundant CPU speed
    may  benefit  by configuring BGET for BestFit allocation (which is
    usually not worth it in other environments).

    Malloc() Emulation
    ------------------

    If the C library malloc() function is too  slow,  not  present  in
    your  development environment (for example, an a native Windows or
    Macintosh program), or otherwise unsuitable, you  can  replace  it
    with  BGET.  Initially define a buffer pool of an appropriate size
    with bpool()--usually obtained by making a call to	the  operating
    system's  low-level  memory allocator.  Then allocate buffers with
    bget(), bgetz(), and bgetr() (the last two permit  the  allocation
    of	buffers initialised to zero and [inefficient] re-allocation of
    existing buffers for  compatibility  with  C  library  functions).
    Release buffers by calling brel().	If a buffer allocation request
    fails, obtain more storage from the underlying  operating  system,
    add it to the buffer pool by another call to bpool(), and continue
    execution.

    Automatic Storage Management
    ----------------------------

    You can use BGET as your application's native memory  manager  and
    implement  automatic  storage  pool  expansion,  contraction,  and
    optionally application-specific  memory  compaction  by  compiling
    BGET  with	the  BECtl  variable defined, then calling bectl() and
    supplying  functions  for  storage	compaction,  acquisition,  and
    release,  as  well as a standard pool expansion increment.	All of
    these functions are optional (although it doesn't make much  sense
    to	provide  a  release  function without an acquisition function,
    does it?).	Once the call-back functions have  been  defined  with
    bectl(),  you simply use bget() and brel() to allocate and release
    storage as before.	You can supply an  initial  buffer  pool  with
    bpool()  or  rely  on  automatic  allocation to acquire the entire
    pool.  When a call on  bget()  cannot  be  satisfied,  BGET  first
    checks  if	a compaction function has been supplied.  If so, it is
    called (with the space required to satisfy the allocation  request
    and a sequence number to allow the compaction routine to be called
    successively without looping).  If the compaction function is able
    to  free any storage (it needn't know whether the storage it freed
    was adequate) it should return a  nonzero  value,  whereupon  BGET
    will retry the allocation request and, if it fails again, call the
    compaction function again with the next-higher sequence number.

    If	the  compaction  function  returns zero, indicating failure to
    free space, or no compaction function is defined, BGET next  tests
    whether  a	non-NULL  allocation function was supplied to bectl().
    If so, that function is called with  an  argument  indicating  how
    many  bytes  of  additional  space are required.  This will be the
    standard pool expansion increment supplied in the call to  bectl()
    unless  the  original  bget()  call requested a buffer larger than
    this; buffers larger than the standard pool block can  be  managed
    "off  the books" by BGET in this mode.  If the allocation function
    succeeds in obtaining the storage, it returns a pointer to the new
    block  and	BGET  expands  the  buffer  pool;  if  it  fails,  the
    allocation request fails and returns NULL to  the  caller.	 If  a
    non-NULL  release  function  is  supplied,	expansion blocks which
    become totally empty are released  to  the	global	free  pool  by
    passing their addresses to the release function.

    Equipped  with  appropriate  allocation,  release,	and compaction
    functions, BGET can be used as part of very  sophisticated	memory
    management	 strategies,  including  garbage  collection.	(Note,
    however, that BGET is *not* a garbage  collector  by  itself,  and
    that  developing  such a system requires much additional logic and
    careful design of the application's memory allocation strategy.)


    BGET FUNCTION DESCRIPTIONS
    ==========================

    Functions implemented in this file (some are enabled by certain of
    the optional settings below):

	    void bpool(void *buffer, bufsize len);

    Create a buffer pool of <len> bytes, using the storage starting at
    <buffer>.	You  can  call	bpool()  subsequently  to   contribute
    additional storage to the overall buffer pool.

	    void *bget(bufsize size);

    Allocate  a  buffer of <size> bytes.  The address of the buffer is
    returned, or NULL if insufficient memory was available to allocate
    the buffer.

	    void *bgetz(bufsize size);

    Allocate a buffer of <size> bytes and clear it to all zeroes.  The
    address of the buffer is returned, or NULL if insufficient	memory
    was available to allocate the buffer.

	    void *bgetr(void *buffer, bufsize newsize);

    Reallocate a buffer previously allocated by bget(),  changing  its
    size  to  <newsize>  and  preserving  all  existing data.  NULL is
    returned if insufficient memory is	available  to  reallocate  the
    buffer, in which case the original buffer remains intact.

	    void brel(void *buf);

    Return  the  buffer  <buf>, previously allocated by bget(), to the
    free space pool.

	    void bectl(int (*compact)(bufsize sizereq, int sequence),
		       void *(*acquire)(bufsize size),
		       void (*release)(void *buf),
		       bufsize pool_incr);

    Expansion control: specify functions through which the package may
    compact  storage  (or  take  other	appropriate  action)  when  an
    allocation	request  fails,  and  optionally automatically acquire
    storage for expansion blocks  when	necessary,  and  release  such
    blocks when they become empty.  If <compact> is non-NULL, whenever
    a buffer allocation request fails, the <compact> function will  be
    called with arguments specifying the number of bytes (total buffer
    size,  including  header  overhead)  required   to	 satisfy   the
    allocation request, and a sequence number indicating the number of
    consecutive  calls	on  <compact>  attempting  to	satisfy   this
    allocation	request.   The sequence number is 1 for the first call
    on <compact> for a given allocation  request,  and	increments  on
    subsequent	calls,	permitting  the  <compact>  function  to  take
    increasingly dire measures in an attempt to free up  storage.   If
    the  <compact>  function  returns  a nonzero value, the allocation
    attempt is re-tried.  If <compact> returns 0 (as  it  must	if  it
    isn't  able  to  release  any  space  or add storage to the buffer
    pool), the allocation request fails, which can  trigger  automatic
    pool expansion if the <acquire> argument is non-NULL.  At the time
    the  <compact>  function  is  called,  the	state  of  the	buffer
    allocator  is  identical  to  that	at  the  moment the allocation
    request was made; consequently, the <compact>  function  may  call
    brel(), bpool(), bstats(), and/or directly manipulate  the	buffer
    pool  in  any  manner which would be valid were the application in
    control.  This does not, however, relieve the  <compact>  function
    of the need to ensure that whatever actions it takes do not change
    things   underneath  the  application  that  made  the  allocation
    request.  For example, a <compact> function that released a buffer
    in	the  process  of  being reallocated with bgetr() would lead to
    disaster.  Implementing a safe and effective  <compact>  mechanism
    requires  careful  design of an application's memory architecture,
    and cannot generally be easily retrofitted into existing code.

    If <acquire> is non-NULL, that function will be called whenever an
    allocation	request  fails.  If the <acquire> function succeeds in
    allocating the requested space and returns a pointer  to  the  new
    area,  allocation will proceed using the expanded buffer pool.  If
    <acquire> cannot obtain the requested space, it should return NULL
    and   the	entire	allocation  process  will  fail.   <pool_incr>
    specifies the normal expansion block size.	Providing an <acquire>
    function will cause subsequent bget()  requests  for  buffers  too
    large  to  be  managed in the linked-block scheme (in other words,
    larger than <pool_incr> minus the buffer overhead) to be satisfied
    directly by calls to the <acquire> function.  Automatic release of
    empty pool blocks will occur only if all pool blocks in the system
    are the size given by <pool_incr>.

	    void bstats(bufsize *curalloc, bufsize *totfree,
			bufsize *maxfree, long *nget, long *nrel);

    The amount	of  space  currently  allocated  is  stored  into  the
    variable  pointed  to by <curalloc>.  The total free space (sum of
    all free blocks in the pool) is stored into the  variable  pointed
    to	by  <totfree>, and the size of the largest single block in the
    free space	pool  is  stored  into	the  variable  pointed	to  by
    <maxfree>.	 The  variables  pointed  to  by <nget> and <nrel> are
    filled, respectively, with	the  number  of  successful  (non-NULL
    return) bget() calls and the number of brel() calls.

	    void bstatse(bufsize *pool_incr, long *npool,
			 long *npget, long *nprel,
			 long *ndget, long *ndrel);

    Extended  statistics: The expansion block size will be stored into
    the variable pointed to by <pool_incr>, or the negative thereof if
    automatic  expansion  block  releases are disabled.  The number of
    currently active pool blocks will  be  stored  into  the  variable
    pointed  to  by  <npool>.  The variables pointed to by <npget> and
    <nprel> will be filled with, respectively, the number of expansion
    block   acquisitions   and	releases  which  have  occurred.   The
    variables pointed to by <ndget> and <ndrel> will  be  filled  with
    the  number  of  bget()  and  brel()  calls, respectively, managed
    through blocks directly allocated by the acquisition  and  release
    functions.

	    void bufdump(void *buf);

    The buffer pointed to by <buf> is dumped on standard output.

	    void bpoold(void *pool, int dumpalloc, int dumpfree);

    All buffers in the buffer pool <pool>, previously initialised by a
    call on bpool(), are listed in ascending memory address order.  If
    <dumpalloc> is nonzero, the  contents  of  allocated  buffers  are
    dumped;  if <dumpfree> is nonzero, the contents of free blocks are
    dumped.

	    int bpoolv(void *pool);

    The  named	buffer	pool,  previously  initialised	by  a  call on
    bpool(), is validated for bad pointers, overwritten data, etc.  If
    compiled with NDEBUG not defined, any error generates an assertion
    failure.  Otherwise 1 is returned if the pool is valid,  0	if  an
    error is found.


    BGET CONFIGURATION
    ==================
*/

#define SizeQuant   4		      /* Buffer allocation size quantum:
					 all buffers allocated are a
					 multiple of this size.  This
					 MUST be a power of two. */

#define FreeWipe    1		      /* Wipe free buffers to a guaranteed
					 pattern of garbage to trip up
					 miscreants who attempt to use
					 pointers into released buffers. */

#define BestFit     0		      /* Use a best fit algorithm when
					 searching for space for an
					 allocation request.  This uses
					 memory more efficiently, but
					 allocation will be much slower. */

#define BECtl	    1		      /* Define this symbol to enable the
					 bectl() function for automatic
					 pool space control.  */

#include <stdio.h>

#include <assert.h>
#include <memory.h>

/*  Declare the interface, including the requested buffer size type,
bufsize.  */

#include "bget.h"

#define MemSize     int 	      /* Type for size arguments to memxxx()
functions such as memcmp(). */

/* Queue links */


static bufsize exp_incr = 0;	      /* Expansion block size */
static bufsize pool_len = 0;	      /* 0: no bpool calls have been made
																		-1: not all pool blocks are
																		the same size
																		>0: (common) block size for all
																		bpool calls made so far
																		*/
/*  Minimum allocation quantum: */

#define QLSize	(sizeof(struct qlinks))
#define SizeQ	((SizeQuant > QLSize) ? SizeQuant : QLSize)

#define V   (void)		      /* To denote unwanted returned values */

/* End sentinel: value placed in bsize field of dummy block delimiting
end of pool block.  The most negative number which will  fit  in  a
bufsize, defined in a way that the compiler will accept. */

#define ESent	((bufsize) (-(((1L << (sizeof(bufsize) * 8 - 2)) - 1) * 2) - 2))

/*  BGET  --  Allocate a buffer.  */

BGet::BGet()
{
	freelist.bh.bsize=0;
	freelist.bh.prevfree=0;
	freelist.ql.blink=&freelist;
	freelist.ql.flink=&freelist;

	acqfcn=0;
}

void* BGet::bget(bufsize requested_size)
{
	bufsize size = requested_size;
	struct bfhead *b;
#ifdef BestFit
	struct bfhead *best;
#endif
	void *buf;

	assert(size >= 0);

	if (size < SizeQ) { 	      /* Need at least room for the */
		size = SizeQ;		      /*    queue links.  */
	}
#ifdef SizeQuant
#if SizeQuant > 1
	size = (size + (SizeQuant - 1)) & (~(SizeQuant - 1));
#endif
#endif

	size += sizeof(struct bhead);     /* Add overhead in allocated buffer
																		to size required. */
	b = freelist.ql.flink;
#ifdef BestFit
	best = &freelist;
#endif


	/* Scan the free list searching for the first buffer big enough
	to hold the requested size buffer. */

#ifdef BestFit
	while (b != &freelist) {
		if (b->bh.bsize >= size) {
			if ((best == &freelist) || (b->bh.bsize < best->bh.bsize)) {
				best = b;
			}
		}
		b = b->ql.flink;		  /* Link to next buffer */
	}
	b = best;
#endif /* BestFit */

	while (b != &freelist) {
		if ((bufsize) b->bh.bsize >= size) {

			/* Buffer  is big enough to satisfy  the request.  Allocate it
			to the caller.  We must decide whether the buffer is  large
			enough  to  split  into  the part given to the caller and a
			free buffer that remains on the free list, or  whether  the
			entire  buffer  should  be  removed	from the free list and
			given to the caller in its entirety.   We  only  split  the
			buffer if enough room remains for a header plus the minimum
			quantum of allocation. */

			if ((b->bh.bsize - size) > (SizeQ + (sizeof(struct bhead)))) {
				struct bhead *ba, *bn;

				ba = BH(((char *) b) + (b->bh.bsize - size));
				bn = BH(((char *) ba) + size);
				assert(bn->prevfree == b->bh.bsize);
				/* Subtract size from length of free block. */
				b->bh.bsize -= size;
				/* Link allocated buffer to the previous free buffer. */
				ba->prevfree = b->bh.bsize;
				/* Plug negative size into user buffer. */
				ba->bsize = -(bufsize) size;
				/* Mark buffer after this one not preceded by free block. */
				bn->prevfree = 0;

				buf = (void *) ((((char *) ba) + sizeof(struct bhead)));
				return buf;
			} else {
				struct bhead *ba;

				ba = BH(((char *) b) + b->bh.bsize);
				assert(ba->prevfree == b->bh.bsize);

				/* The buffer isn't big enough to split.  Give  the  whole
				shebang to the caller and remove it from the free list. */

				assert(b->ql.blink->ql.flink == b);
				assert(b->ql.flink->ql.blink == b);
				b->ql.blink->ql.flink = b->ql.flink;
				b->ql.flink->ql.blink = b->ql.blink;

				/* Negate size to mark buffer allocated. */
				b->bh.bsize = -(b->bh.bsize);

				/* Zero the back pointer in the next buffer in memory
				to indicate that this buffer is allocated. */
				ba->prevfree = 0;

				/* Give user buffer starting at queue links. */
				buf =  (void *) &(b->ql);
				return buf;
			}
		}
		b = b->ql.flink;		  /* Link to next buffer */
	}
#ifdef BECtl
	/* No buffer available with requested size free. */

	/* Don't give up yet -- look in the reserve supply. */

	if (acqfcn != NULL) {
		if (size > exp_incr - sizeof(struct bhead)) {

			/* Request	is  too  large	to  fit in a single expansion
			block.  Try to satisy it by a direct buffer acquisition. */

			struct bdhead *bdh;

			size += sizeof(struct bdhead) - sizeof(struct bhead);
			if ((bdh = BDH((*acqfcn)((bufsize) size))) != NULL) {

				/*  Mark the buffer special by setting the size field
				of its header to zero.  */
				bdh->bh.bsize = 0;
				bdh->bh.prevfree = 0;
				bdh->tsize = size;
				buf =  (void *) (bdh + 1);
				return buf;
			}

		} else {

			/*	Try to obtain a new expansion block */

			void *newpool;

			if ((newpool = (*acqfcn)((bufsize) exp_incr)) != NULL) {
				bpool(newpool, exp_incr);
				buf =  bget(requested_size);  /* This can't, I say, can't
																			get into a loop. */
				return buf;
			}
		}
	}

	/*	Still no buffer available */

#endif /* BECtl */

	return NULL;
}

/*  BGETZ  --  Allocate a buffer and clear its contents to zero.  We clear
the  entire  contents  of  the buffer to zero, not just the
region requested by the caller. */

void *BGet::bgetz(bufsize size)
{
	char *buf = (char *) bget(size);

	if (buf != NULL) {
		struct bhead *b;
		bufsize rsize;

		b = BH(buf - sizeof(struct bhead));
		rsize = -(b->bsize);
		if (rsize == 0) {
			struct bdhead *bd;

			bd = BDH(buf - sizeof(struct bdhead));
			rsize = bd->tsize - sizeof(struct bdhead);
		} else {
			rsize -= sizeof(struct bhead);
		}
		assert(rsize >= size);
		V memset(buf, 0, (MemSize) rsize);
	}
	return ((void *) buf);
}

/*  BGETR  --  Reallocate a buffer.  This is a minimal implementation,
simply in terms of brel()  and  bget().	 It  could  be
enhanced to allow the buffer to grow into adjacent free
blocks and to avoid moving data unnecessarily.  */

void* BGet::bgetr(void *buf, bufsize size)
{
	void *nbuf;
	bufsize osize;		      /* Old size of buffer */
	struct bhead *b;

	if ((nbuf = bget(size)) == NULL) { /* Acquire new buffer */
		return NULL;
	}
	if (buf == NULL) {
		return nbuf;
	}
	b = BH(((char *) buf) - sizeof(struct bhead));
	osize = -b->bsize;
#ifdef BECtl
	if (osize == 0) {
		/*  Buffer acquired directly through acqfcn. */
		struct bdhead *bd;

		bd = BDH(((char *) buf) - sizeof(struct bdhead));
		osize = bd->tsize - sizeof(struct bdhead);
	} else
#endif
		osize -= sizeof(struct bhead);
	assert(osize > 0);
	V memcpy((char *) nbuf, (char *) buf, /* Copy the data */
		(MemSize) ((size < osize) ? size : osize));
	brel(buf);
	return nbuf;
}

/*  BREL  --  Release a buffer.  */

void BGet::brel(void *buf)
{
	struct bfhead *b, *bn;

	b = BFH(((char *) buf) - sizeof(struct bhead));
	assert(buf != NULL);

	/* Buffer size must be negative, indicating that the buffer is
	allocated. */

	if (b->bh.bsize >= 0) {
		bn = NULL;
	}
	assert(b->bh.bsize < 0);

	/*	Back pointer in next buffer must be zero, indicating the
	same thing: */

	assert(BH((char *) b - b->bh.bsize)->prevfree == 0);

	/* If the back link is nonzero, the previous buffer is free.  */

	if (b->bh.prevfree != 0) {

		/* The previous buffer is free.  Consolidate this buffer  with	it
		by  adding  the  length  of	this  buffer  to the previous free
		buffer.  Note that we subtract the size  in	the  buffer  being
		released,  since  it's  negative to indicate that the buffer is
		allocated. */

		register bufsize size = b->bh.bsize;

		/* Make the previous buffer the one we're working on. */
		assert(BH((char *) b - b->bh.prevfree)->bsize == b->bh.prevfree);
		b = BFH(((char *) b) - b->bh.prevfree);
		b->bh.bsize -= size;
	} else {

		/* The previous buffer isn't allocated.  Insert this buffer
		on the free list as an isolated free block. */

		assert(freelist.ql.blink->ql.flink == &freelist);
		assert(freelist.ql.flink->ql.blink == &freelist);
		b->ql.flink = &freelist;
		b->ql.blink = freelist.ql.blink;
		freelist.ql.blink = b;
		b->ql.blink->ql.flink = b;
		b->bh.bsize = -b->bh.bsize;
	}

	/* Now we look at the next buffer in memory, located by advancing from
	the  start  of  this  buffer  by its size, to see if that buffer is
	free.  If it is, we combine  this  buffer  with	the  next  one	in
	memory, dechaining the second buffer from the free list. */

	bn =  BFH(((char *) b) + b->bh.bsize);
	if (bn->bh.bsize > 0) {

		/* The buffer is free.	Remove it from the free list and add
		its size to that of our buffer. */

		assert(BH((char *) bn + bn->bh.bsize)->prevfree == bn->bh.bsize);
		assert(bn->ql.blink->ql.flink == bn);
		assert(bn->ql.flink->ql.blink == bn);
		bn->ql.blink->ql.flink = bn->ql.flink;
		bn->ql.flink->ql.blink = bn->ql.blink;
		b->bh.bsize += bn->bh.bsize;

		/* Finally,  advance  to   the	buffer	that   follows	the  newly
		consolidated free block.  We must set its  backpointer  to  the
		head  of  the  consolidated free block.  We know the next block
		must be an allocated block because the process of recombination
		guarantees  that  two  free	blocks will never be contiguous in
		memory.  */

		bn = BFH(((char *) b) + b->bh.bsize);
	}
#ifdef FreeWipe
	V memset(((char *) b) + sizeof(struct bfhead), 0x55,
		(MemSize) (b->bh.bsize - sizeof(struct bfhead)));
#endif
	assert(bn->bh.bsize < 0);

	/* The next buffer is allocated.  Set the backpointer in it  to  point
	to this buffer; the previous free buffer in memory. */

	bn->bh.prevfree = b->bh.bsize;
}

#ifdef BECtl

/*  BECTL  --  Establish automatic pool expansion control  */

void	BGet::bectl(void *(*acquire) (bufsize size), bufsize pool_incr)
{
	acqfcn = acquire;
	exp_incr = pool_incr;
}
#endif

/*  BPOOL  --  Add a region of memory to the buffer pool.  */

void BGet::bpool(void *buf, bufsize len)
{
	struct bfhead *b = BFH(buf);
	struct bhead *bn;

#ifdef SizeQuant
	len &= ~(SizeQuant - 1);
#endif
#ifdef BECtl
	if (pool_len == 0) {
		pool_len = len;
	} else if (len != pool_len) {
		pool_len = -1;
	}
#endif /* BECtl */

	/* Since the block is initially occupied by a single free  buffer,
	it  had	better	not  be  (much) larger than the largest buffer
	whose size we can store in bhead.bsize. */

	assert(len - sizeof(struct bhead) <= -((bufsize) ESent + 1));

	/* Clear  the  backpointer at  the start of the block to indicate that
	there  is  no  free  block  prior  to  this   one.    That   blocks
	recombination when the first block in memory is released. */

	b->bh.prevfree = 0;

	/* Chain the new block to the free list. */

	assert(freelist.ql.blink->ql.flink == &freelist);
	assert(freelist.ql.flink->ql.blink == &freelist);
	b->ql.flink = &freelist;
	b->ql.blink = freelist.ql.blink;
	freelist.ql.blink = b;
	b->ql.blink->ql.flink = b;

	/* Create a dummy allocated buffer at the end of the pool.	This dummy
	buffer is seen when a buffer at the end of the pool is released and
	blocks  recombination  of  the last buffer with the dummy buffer at
	the end.  The length in the dummy buffer  is  set  to  the  largest
	negative  number  to  denote  the  end  of  the pool for diagnostic
	routines (this specific value is  not  counted  on  by  the  actual
	allocation and release functions). */

	len -= sizeof(struct bhead);
	b->bh.bsize = (bufsize) len;
#ifdef FreeWipe
	V memset(((char *) b) + sizeof(struct bfhead), 0x55,
		(MemSize) (len - sizeof(struct bfhead)));
#endif
	bn = BH(((char *) b) + len);
	bn->prevfree = (bufsize) len;
	/* Definition of ESent assumes two's complement! */
	assert((~0) == -1);
	bn->bsize = ESent;
}

