/* See the import.pl script for potential modifications */
/* k_rem_pio2f.c -- Simple version of k_rem_pio2.c
 * Conversion to Simple by Ian Lance Taylor, Cygnus Support, ian@cygnus.com.
 */

/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice 
 * is preserved.
 * ====================================================
 */

#if defined(LIBM_SCCS) && !defined(lint)
static char rcsid[] = "$NetBSD: k_rem_pio2f.c,v 1.4f 1995/05/10 20:46:28 jtc Exp $";
#endif

#include "SMath.h"
#include "math_private.h"

/* In the Simple version, the input parameter x contains 8 bit
   integers, not 24 bit integers.  113 bit precision is not supported.  */

namespace streflop_libm {
#ifdef __STDC__
static const int init_jk[] = {4,7,9}; /* initial value for jk */
#else
static int init_jk[] = {4,7,9}; 
#endif

#ifdef __STDC__
static const Simple PIo2[] = {
#else
static Simple PIo2[] = {
#endif
  1.5703125000e+00f, /* 0x3fc90000 */
  4.5776367188e-04f, /* 0x39f00000 */
  2.5987625122e-05f, /* 0x37da0000 */
  7.5437128544e-08f, /* 0x33a20000 */
  6.0026650317e-11f, /* 0x2e840000 */
  7.3896444519e-13f, /* 0x2b500000 */
  5.3845816694e-15f, /* 0x27c20000 */
  5.6378512969e-18f, /* 0x22d00000 */
  8.3009228831e-20f, /* 0x1fc40000 */
  3.2756352257e-22f, /* 0x1bc60000 */
  6.3331015649e-25f, /* 0x17440000 */
};

#ifdef __STDC__
static const Simple			
#else
static Simple			
#endif
zero   = 0.0f,
one    = 1.0f,
two8   =  2.5600000000e+02f, /* 0x43800000 */
twon8  =  3.9062500000e-03f; /* 0x3b800000 */

#ifdef __STDC__
	int __kernel_rem_pio2f(Simple *x, Simple *y, int e0, int nx, int prec, const int32_t *ipio2) 
#else
	int __kernel_rem_pio2f(x,y,e0,nx,prec,ipio2) 	
	Simple x[], y[]; int e0,nx,prec; int32_t ipio2[];
#endif
{
	int32_t jz,jx,jv,jp,jk,carry,n,iq[20],i,j,k,m,q0,ih;
	Simple z,fw,f[20],fq[20],q[20];

    /* initialize jk*/
	jk = init_jk[prec];
	jp = jk;

    /* determine jx,jv,q0, note that 3>q0 */
	jx =  nx-1;
	jv = (e0-3)/8; if(jv<0) jv=0;
	q0 =  e0-8*(jv+1);

    /* set up f[0] to f[jx+jk] where f[jx+jk] = ipio2[jv+jk] */
	j = jv-jx; m = jx+jk;
	for(i=0;i<=m;i++,j++) f[i] = (j<0)? zero : (Simple) ipio2[j];

    /* compute q[0],q[1],...q[jk] */
	for (i=0;i<=jk;i++) {
	    for(j=0,fw=0.0f;j<=jx;j++) fw += x[j]*f[jx+i-j]; q[i] = fw;
	}

	jz = jk;
recompute:
    /* distill q[] into iq[] reversingly */
	for(i=0,j=jz,z=q[jz];j>0;i++,j--) {
	    fw    =  (Simple)((int32_t)(twon8* z));
	    iq[i] =  (int32_t)(z-two8*fw);
	    z     =  q[j-1]+fw;
	}

    /* compute n */
	z  = __scalbnf(z,q0);		/* actual value of z */
	z -= (Simple)8.0f*__floorf(z*(Simple)0.125f);	/* trim off integer >= 8 */
	n  = (int32_t) z;
	z -= (Simple)n;
	ih = 0;
	if(q0>0) {	/* need iq[jz-1] to determine n */
	    i  = (iq[jz-1]>>(8-q0)); n += i;
	    iq[jz-1] -= i<<(8-q0);
	    ih = iq[jz-1]>>(7-q0);
	} 
	else if(q0==0) ih = iq[jz-1]>>8;
	else if(z>=(Simple)0.5f) ih=2;

	if(ih>0) {	/* q > 0.5f */
	    n += 1; carry = 0;
	    for(i=0;i<jz ;i++) {	/* compute 1-q */
		j = iq[i];
		if(carry==0) {
		    if(j!=0) {
			carry = 1; iq[i] = 0x100- j;
		    }
		} else  iq[i] = 0xff - j;
	    }
	    if(q0>0) {		/* rare case: chance is 1 in 12 */
	        switch(q0) {
	        case 1:
	    	   iq[jz-1] &= 0x7f; break;
	    	case 2:
	    	   iq[jz-1] &= 0x3f; break;
	        }
	    }
	    if(ih==2) {
		z = one - z;
		if(carry!=0) z -= __scalbnf(one,q0);
	    }
	}

    /* check if recomputation is needed */
	if(z==zero) {
	    j = 0;
	    for (i=jz-1;i>=jk;i--) j |= iq[i];
	    if(j==0) { /* need recomputation */
		for(k=1;iq[jk-k]==0;k++);   /* k = no. of terms needed */

		for(i=jz+1;i<=jz+k;i++) {   /* add q[jz+1] to q[jz+k] */
		    f[jx+i] = (Simple) ipio2[jv+i];
		    for(j=0,fw=0.0f;j<=jx;j++) fw += x[j]*f[jx+i-j];
		    q[i] = fw;
		}
		jz += k;
		goto recompute;
	    }
	}

    /* chop off zero terms */
	if(z==(Simple)0.0f) {
	    jz -= 1; q0 -= 8;
	    while(iq[jz]==0) { jz--; q0-=8;}
	} else { /* break z into 8-bit if necessary */
	    z = __scalbnf(z,-q0);
	    if(z>=two8) { 
		fw = (Simple)((int32_t)(twon8*z));
		iq[jz] = (int32_t)(z-two8*fw);
		jz += 1; q0 += 8;
		iq[jz] = (int32_t) fw;
	    } else iq[jz] = (int32_t) z ;
	}

    /* convert integer "bit" chunk to floating-point value */
	fw = __scalbnf(one,q0);
	for(i=jz;i>=0;i--) {
	    q[i] = fw*(Simple)iq[i]; fw*=twon8;
	}

    /* compute PIo2[0,...,jp]*q[jz,...,0] */
	for(i=jz;i>=0;i--) {
	    for(fw=0.0f,k=0;k<=jp&&k<=jz-i;k++) fw += PIo2[k]*q[i+k];
	    fq[jz-i] = fw;
	}

    /* compress fq[] into y[] */
	switch(prec) {
	    case 0:
		fw = 0.0f;
		for (i=jz;i>=0;i--) fw += fq[i];
		y[0] = (ih==0)? fw: -fw; 
		break;
	    case 1:
	    case 2:
		fw = 0.0f;
		for (i=jz;i>=0;i--) fw += fq[i]; 
		y[0] = (ih==0)? fw: -fw; 
		fw = fq[0]-fw;
		for (i=1;i<=jz;i++) fw += fq[i];
		y[1] = (ih==0)? fw: -fw; 
		break;
	    case 3:	/* painful */
		for (i=jz;i>0;i--) {
		    fw      = fq[i-1]+fq[i]; 
		    fq[i]  += fq[i-1]-fw;
		    fq[i-1] = fw;
		}
		for (i=jz;i>1;i--) {
		    fw      = fq[i-1]+fq[i]; 
		    fq[i]  += fq[i-1]-fw;
		    fq[i-1] = fw;
		}
		for (fw=0.0f,i=jz;i>=2;i--) fw += fq[i]; 
		if(ih==0) {
		    y[0] =  fq[0]; y[1] =  fq[1]; y[2] =  fw;
		} else {
		    y[0] = -fq[0]; y[1] = -fq[1]; y[2] = -fw;
		}
	}
	return n&7;
}
}
