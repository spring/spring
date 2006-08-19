/* See the import.pl script for potential modifications */
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

/* Long Double expansions are
  Copyright (C) 2001 Stephen L. Moshier <moshier@na-net.ornl.gov>
  and are incorporated herein by permission of the author.  The author 
  reserves the right to distribute this material elsewhere under different
  copying permissions.  These modifications are distributed here under 
  the following terms:

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1l of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA */

/* __ieee754_j1(x), __ieee754_y1(x)
 * Bessel function of the first and second kinds of order zero.
 * Method -- j1(x):
 *	1. For tiny x, we use j1(x) = x/2 - x^3/16 + x^5/384 - ...
 *	2. Reduce x to |x| since j1(x)=-j1(-x),  and
 *	   for x in (0,2)
 *		j1(x) = x/2 + x*z*R0/S0,  where z = x*x;
 *	   for x in (2,inf)
 * 		j1(x) = sqrt(2/(pi*x))*(p1(x)*cos(x1)-q1(x)*sin(x1))
 * 		y1(x) = sqrt(2/(pi*x))*(p1(x)*sin(x1)+q1(x)*cos(x1))
 * 	   where x1 = x-3*pi/4. It is better to compute sin(x1),cos(x1)
 *	   as follow:
 *		cos(x1) =  cos(x)cos(3pi/4)+sin(x)sin(3pi/4)
 *			=  1/sqrt(2) * (sin(x) - cos(x))
 *		sin(x1) =  sin(x)cos(3pi/4)-cos(x)sin(3pi/4)
 *			= -1/sqrt(2) * (sin(x) + cos(x))
 * 	   (To avoid cancellation, use
 *		sin(x) +- cos(x) = -cos(2x)/(sin(x) -+ cos(x))
 * 	    to compute the worse one.)
 *
 *	3 Special cases
 *		j1(nan)= nan
 *		j1(0) = 0
 *		j1(inf) = 0
 *
 * Method -- y1(x):
 *	1. screen out x<=0 cases: y1(0)=-inf, y1(x<0)=NaN
 *	2. For x<2.
 *	   Since
 *		y1(x) = 2/pi*(j1(x)*(ln(x/2)+Euler)-1/x-x/2+5/64*x^3-...)
 *	   therefore y1(x)-2/pi*j1(x)*ln(x)-1/x is an odd function.
 *	   We use the following function to approximate y1,
 *		y1(x) = x*U(z)/V(z) + (2/pi)*(j1(x)*ln(x)-1/x), z= x^2
 *	   Note: For tiny x, 1/x dominate y1 and hence
 *		y1(tiny) = -2/pi/tiny
 *	3. For x>=2.
 * 		y1(x) = sqrt(2/(pi*x))*(p1(x)*sin(x1)+q1(x)*cos(x1))
 * 	   where x1 = x-3*pi/4. It is better to compute sin(x1),cos(x1)
 *	   by method mentioned above.
 */

#include "math.h"
#include "math_private.h"

namespace streflop_libm {
#ifdef __STDC__
static Extended pone (Extended), qone (Extended);
#else
static Extended pone (), qone ();
#endif

#ifdef __STDC__
static const Extended
#else
static Extended
#endif
  huge = 1e4930L,
 one = 1.0l,
 invsqrtpi = 5.6418958354775628694807945156077258584405e-1l,
  tpi =  6.3661977236758134307553505349005744813784e-1l,

  /* J1(x) = .5 x + x x^2 R(x^2) / S(x^2)
     0 <= x <= 2
     Peak relative error 4.5e-21l */
R[5] = {
    -9.647406112428107954753770469290757756814E7l,
    2.686288565865230690166454005558203955564E6l,
    -3.689682683905671185891885948692283776081E4l,
    2.195031194229176602851429567792676658146E2l,
    -5.124499848728030297902028238597308971319E-1l,
},

  S[4] =
{
  1.543584977988497274437410333029029035089E9l,
  2.133542369567701244002565983150952549520E7l,
  1.394077011298227346483732156167414670520E5l,
  5.252401789085732428842871556112108446506E2l,
  /*  1.000000000000000000000000000000000000000E0l, */
};

#ifdef __STDC__
static const Extended zero = 0.0l;
#else
static Extended zero = 0.0l;
#endif


#ifdef __STDC__
Extended
__ieee754_j1l (Extended x)
#else
Extended
__ieee754_j1l (x)
     Extended x;
#endif
{
  Extended z, c, r, s, ss, cc, u, v, y;
  int32_t ix;
  u_int32_t se, i0, i1;

  GET_LDOUBLE_WORDS (se, i0, i1, x);
  ix = se & 0x7fff;
  if (ix >= 0x7fff)
    return one / x;
  y = fabsl (x);
  if (ix >= 0x4000)
    {				/* |x| >= 2.0l */
      __sincosl (y, &s, &c);
      ss = -s - c;
      cc = s - c;
      if (ix < 0x7ffe)
	{			/* make sure y+y not overflow */
	  z = __cosl (y + y);
	  if ((s * c) > zero)
	    cc = z / ss;
	  else
	    ss = z / cc;
	}
      /*
       * j1(x) = 1/sqrt(pi) * (P(1,x)*cc - Q(1,x)*ss) / sqrt(x)
       * y1(x) = 1/sqrt(pi) * (P(1,x)*ss + Q(1,x)*cc) / sqrt(x)
       */
      if (ix > 0x4080)
	z = (invsqrtpi * cc) / __ieee754_sqrtl (y);
      else
	{
	  u = pone (y);
	  v = qone (y);
	  z = invsqrtpi * (u * cc - v * ss) / __ieee754_sqrtl (y);
	}
      if (se & 0x8000)
	return -z;
      else
	return z;
    }
  if (ix < 0x3fde) /* |x| < 2^-33 */
    {
      if (huge + x > one)
	return 0.5l * x;		/* inexact if x!=0 necessary */
    }
  z = x * x;
  r = z * (R[0] + z * (R[1]+ z * (R[2] + z * (R[3] + z * R[4]))));
  s = S[0] + z * (S[1] + z * (S[2] + z * (S[3] + z)));
  r *= x;
  return (x * 0.5l + r / s);
}


/* Y1(x) = 2/pi * (log(x) * j1(x) - 1/x) + x R(x^2)
   0 <= x <= 2
   Peak relative error 2.3e-23l */
#ifdef __STDC__
static const Extended U0[6] = {
#else
static Extended U0[6] = {
#endif
  -5.908077186259914699178903164682444848615E10l,
  1.546219327181478013495975514375773435962E10l,
  -6.438303331169223128870035584107053228235E8l,
  9.708540045657182600665968063824819371216E6l,
  -6.138043997084355564619377183564196265471E4l,
  1.418503228220927321096904291501161800215E2l,
};
#ifdef __STDC__
static const Extended V0[5] = {
#else
static Extended V0[5] = {
#endif
  3.013447341682896694781964795373783679861E11l,
  4.669546565705981649470005402243136124523E9l,
  3.595056091631351184676890179233695857260E7l,
  1.761554028569108722903944659933744317994E5l,
  5.668480419646516568875555062047234534863E2l,
  /*  1.000000000000000000000000000000000000000E0l, */
};


#ifdef __STDC__
Extended
__ieee754_y1l (Extended x)
#else
Extended
__ieee754_y1l (x)
     Extended x;
#endif
{
  Extended z, s, c, ss, cc, u, v;
  int32_t ix;
  u_int32_t se, i0, i1;

  GET_LDOUBLE_WORDS (se, i0, i1, x);
  ix = se & 0x7fff;
  /* if Y1(NaN) is NaN, Y1(-inf) is NaN, Y1(inf) is 0 */
  if (se & 0x8000)
    return zero / (zero * x);
  if (ix >= 0x7fff)
    return one / (x + x * x);
  if ((i0 | i1) == 0)
    return -HUGE_VALL + x;  /* -inf and overflow exception.  */
  if (ix >= 0x4000)
    {				/* |x| >= 2.0l */
      __sincosl (x, &s, &c);
      ss = -s - c;
      cc = s - c;
      if (ix < 0x7fe00000)
	{			/* make sure x+x not overflow */
	  z = __cosl (x + x);
	  if ((s * c) > zero)
	    cc = z / ss;
	  else
	    ss = z / cc;
	}
      /* y1(x) = sqrt(2/(pi*x))*(p1(x)*sin(x0)+q1(x)*cos(x0))
       * where x0 = x-3pi/4
       *      Better formula:
       *              cos(x0) = cos(x)cos(3pi/4)+sin(x)sin(3pi/4)
       *                      =  1/sqrt(2) * (sin(x) - cos(x))
       *              sin(x0) = sin(x)cos(3pi/4)-cos(x)sin(3pi/4)
       *                      = -1/sqrt(2) * (cos(x) + sin(x))
       * To avoid cancellation, use
       *              sin(x) +- cos(x) = -cos(2x)/(sin(x) -+ cos(x))
       * to compute the worse one.
       */
      if (ix > 0x4080)
	z = (invsqrtpi * ss) / __ieee754_sqrtl (x);
      else
	{
	  u = pone (x);
	  v = qone (x);
	  z = invsqrtpi * (u * ss + v * cc) / __ieee754_sqrtl (x);
	}
      return z;
    }
  if (ix <= 0x3fbe)
    {				/* x < 2**-65 */
      return (-tpi / x);
    }
  z = x * x;
 u = U0[0] + z * (U0[1] + z * (U0[2] + z * (U0[3] + z * (U0[4] + z * U0[5]))));
 v = V0[0] + z * (V0[1] + z * (V0[2] + z * (V0[3] + z * (V0[4] + z))));
  return (x * (u / v) +
	  tpi * (__ieee754_j1l (x) * __ieee754_logl (x) - one / x));
}


/* For x >= 8, the asymptotic expansions of pone is
 *	1 + 15/128 s^2 - 4725/2^15 s^4 - ...,	where s = 1/x.
 * We approximate pone by
 * 	pone(x) = 1 + (R/S)
 */

/* J1(x) cosX + Y1(x) sinX  =  sqrt( 2/(pi x)) P1(x)
   P1(x) = 1 + z^2 R(z^2), z=1/x
   8 <= x <= inf  (0 <= z <= 0.125l)
   Peak relative error 5.2e-22l  */

#ifdef __STDC__
static const Extended pr8[7] = {
#else
static Extended pr8[7] = {
#endif
  8.402048819032978959298664869941375143163E-9l,
  1.813743245316438056192649247507255996036E-6l,
  1.260704554112906152344932388588243836276E-4l,
  3.439294839869103014614229832700986965110E-3l,
  3.576910849712074184504430254290179501209E-2l,
  1.131111483254318243139953003461511308672E-1l,
  4.480715825681029711521286449131671880953E-2l,
};
#ifdef __STDC__
static const Extended ps8[6] = {
#else
static Extended ps8[6] = {
#endif
  7.169748325574809484893888315707824924354E-8l,
  1.556549720596672576431813934184403614817E-5l,
  1.094540125521337139209062035774174565882E-3l,
  3.060978962596642798560894375281428805840E-2l,
  3.374146536087205506032643098619414507024E-1l,
  1.253830208588979001991901126393231302559E0l,
  /* 1.000000000000000000000000000000000000000E0l, */
};

/* J1(x) cosX + Y1(x) sinX  =  sqrt( 2/(pi x)) P1(x)
   P1(x) = 1 + z^2 R(z^2), z=1/x
   4.54541015625l <= x <= 8
   Peak relative error 7.7e-22l  */
#ifdef __STDC__
static const Extended pr5[7] = {
#else
static Extended pr5[7] = {
#endif
  4.318486887948814529950980396300969247900E-7l,
  4.715341880798817230333360497524173929315E-5l,
  1.642719430496086618401091544113220340094E-3l,
  2.228688005300803935928733750456396149104E-2l,
  1.142773760804150921573259605730018327162E-1l,
  1.755576530055079253910829652698703791957E-1l,
  3.218803858282095929559165965353784980613E-2l,
};
#ifdef __STDC__
static const Extended ps5[6] = {
#else
static Extended ps5[6] = {
#endif
  3.685108812227721334719884358034713967557E-6l,
  4.069102509511177498808856515005792027639E-4l,
  1.449728676496155025507893322405597039816E-2l,
  2.058869213229520086582695850441194363103E-1l,
  1.164890985918737148968424972072751066553E0l,
  2.274776933457009446573027260373361586841E0l,
  /*  1.000000000000000000000000000000000000000E0l,*/
};

/* J1(x) cosX + Y1(x) sinX  =  sqrt( 2/(pi x)) P1(x)
   P1(x) = 1 + z^2 R(z^2), z=1/x
   2.85711669921875l <= x <= 4.54541015625l
   Peak relative error 6.5e-21l  */
#ifdef __STDC__
static const Extended pr3[7] = {
#else
static Extended pr3[7] = {
#endif
  1.265251153957366716825382654273326407972E-5l,
  8.031057269201324914127680782288352574567E-4l,
  1.581648121115028333661412169396282881035E-2l,
  1.179534658087796321928362981518645033967E-1l,
  3.227936912780465219246440724502790727866E-1l,
  2.559223765418386621748404398017602935764E-1l,
  2.277136933287817911091370397134882441046E-2l,
};
#ifdef __STDC__
static const Extended ps3[6] = {
#else
static Extended ps3[6] = {
#endif
  1.079681071833391818661952793568345057548E-4l,
  6.986017817100477138417481463810841529026E-3l,
  1.429403701146942509913198539100230540503E-1l,
  1.148392024337075609460312658938700765074E0l,
  3.643663015091248720208251490291968840882E0l,
  3.990702269032018282145100741746633960737E0l,
  /* 1.000000000000000000000000000000000000000E0l, */
};

/* J1(x) cosX + Y1(x) sinX  =  sqrt( 2/(pi x)) P1(x)
   P1(x) = 1 + z^2 R(z^2), z=1/x
   2 <= x <= 2.85711669921875l
   Peak relative error 3.5e-21l  */
#ifdef __STDC__
static const Extended pr2[7] = {
#else
static Extended pr2[7] = {
#endif
  2.795623248568412225239401141338714516445E-4l,
  1.092578168441856711925254839815430061135E-2l,
  1.278024620468953761154963591853679640560E-1l,
  5.469680473691500673112904286228351988583E-1l,
  8.313769490922351300461498619045639016059E-1l,
  3.544176317308370086415403567097130611468E-1l,
  1.604142674802373041247957048801599740644E-2l,
};
#ifdef __STDC__
static const Extended ps2[6] = {
#else
static Extended ps2[6] = {
#endif
  2.385605161555183386205027000675875235980E-3l,
  9.616778294482695283928617708206967248579E-2l,
  1.195215570959693572089824415393951258510E0l,
  5.718412857897054829999458736064922974662E0l,
  1.065626298505499086386584642761602177568E1l,
  6.809140730053382188468983548092322151791E0l,
 /* 1.000000000000000000000000000000000000000E0l, */
};


#ifdef __STDC__
static Extended
pone (Extended x)
#else
static Extended
pone (x)
     Extended x;
#endif
{
#ifdef __STDC__
  const Extended *p, *q;
#else
  Extended *p, *q;
#endif
  Extended z, r, s;
  int32_t ix;
  u_int32_t se, i0, i1;

  GET_LDOUBLE_WORDS (se, i0, i1, x);
  ix = se & 0x7fff;
  if (ix >= 0x4002) /* x >= 8 */
    {
      p = pr8;
      q = ps8;
    }
  else
    {
      i1 = (ix << 16) | (i0 >> 16);
      if (i1 >= 0x40019174)	/* x >= 4.54541015625l */
	{
	  p = pr5;
	  q = ps5;
	}
      else if (i1 >= 0x4000b6db)	/* x >= 2.85711669921875l */
	{
	  p = pr3;
	  q = ps3;
	}
      else if (ix >= 0x4000)	/* x better be >= 2 */
	{
	  p = pr2;
	  q = ps2;
	}
    }
  z = one / (x * x);
 r = p[0] + z * (p[1] +
		 z * (p[2] + z * (p[3] + z * (p[4] + z * (p[5] + z * p[6])))));
 s = q[0] + z * (q[1] + z * (q[2] + z * (q[3] + z * (q[4] + z * (q[5] + z)))));
  return one + z * r / s;
}


/* For x >= 8, the asymptotic expansions of qone is
 *	3/8 s - 105/1024 s^3 - ..., where s = 1/x.
 * We approximate pone by
 * 	qone(x) = s*(0.375l + (R/S))
 */

/* Y1(x)cosX - J1(x)sinX = sqrt( 2/(pi x)) Q1(x),
   Q1(x) = z(.375 + z^2 R(z^2)), z=1/x
   8 <= x <= inf
   Peak relative error 8.3e-22l */

#ifdef __STDC__
static const Extended qr8[7] = {
#else
static Extended qr8[7] = {
#endif
  -5.691925079044209246015366919809404457380E-10l,
  -1.632587664706999307871963065396218379137E-7l,
  -1.577424682764651970003637263552027114600E-5l,
  -6.377627959241053914770158336842725291713E-4l,
  -1.087408516779972735197277149494929568768E-2l,
  -6.854943629378084419631926076882330494217E-2l,
  -1.055448290469180032312893377152490183203E-1l,
};
#ifdef __STDC__
static const Extended qs8[7] = {
#else
static Extended qs8[7] = {
#endif
  5.550982172325019811119223916998393907513E-9l,
  1.607188366646736068460131091130644192244E-6l,
  1.580792530091386496626494138334505893599E-4l,
  6.617859900815747303032860443855006056595E-3l,
  1.212840547336984859952597488863037659161E-1l,
  9.017885953937234900458186716154005541075E-1l,
  2.201114489712243262000939120146436167178E0l,
  /* 1.000000000000000000000000000000000000000E0l, */
};

/* Y1(x)cosX - J1(x)sinX = sqrt( 2/(pi x)) Q1(x),
   Q1(x) = z(.375 + z^2 R(z^2)), z=1/x
   4.54541015625l <= x <= 8
   Peak relative error 4.1e-22l */
#ifdef __STDC__
static const Extended qr5[7] = {
#else
static Extended qr5[7] = {
#endif
  -6.719134139179190546324213696633564965983E-8l,
  -9.467871458774950479909851595678622044140E-6l,
  -4.429341875348286176950914275723051452838E-4l,
  -8.539898021757342531563866270278505014487E-3l,
  -6.818691805848737010422337101409276287170E-2l,
  -1.964432669771684034858848142418228214855E-1l,
  -1.333896496989238600119596538299938520726E-1l,
};
#ifdef __STDC__
static const Extended qs5[7] = {
#else
static Extended qs5[7] = {
#endif
  6.552755584474634766937589285426911075101E-7l,
  9.410814032118155978663509073200494000589E-5l,
  4.561677087286518359461609153655021253238E-3l,
  9.397742096177905170800336715661091535805E-2l,
  8.518538116671013902180962914473967738771E-1l,
  3.177729183645800174212539541058292579009E0l,
  4.006745668510308096259753538973038902990E0l,
  /* 1.000000000000000000000000000000000000000E0l, */
};

/* Y1(x)cosX - J1(x)sinX = sqrt( 2/(pi x)) Q1(x),
   Q1(x) = z(.375 + z^2 R(z^2)), z=1/x
   2.85711669921875l <= x <= 4.54541015625l
   Peak relative error 2.2e-21l */
#ifdef __STDC__
static const Extended qr3[7] = {
#else
static Extended qr3[7] = {
#endif
  -3.618746299358445926506719188614570588404E-6l,
  -2.951146018465419674063882650970344502798E-4l,
  -7.728518171262562194043409753656506795258E-3l,
  -8.058010968753999435006488158237984014883E-2l,
  -3.356232856677966691703904770937143483472E-1l,
  -4.858192581793118040782557808823460276452E-1l,
  -1.592399251246473643510898335746432479373E-1l,
};
#ifdef __STDC__
static const Extended qs3[7] = {
#else
static Extended qs3[7] = {
#endif
  3.529139957987837084554591421329876744262E-5l,
  2.973602667215766676998703687065066180115E-3l,
  8.273534546240864308494062287908662592100E-2l,
  9.613359842126507198241321110649974032726E-1l,
  4.853923697093974370118387947065402707519E0l,
  1.002671608961669247462020977417828796933E1l,
  7.028927383922483728931327850683151410267E0l,
  /* 1.000000000000000000000000000000000000000E0l, */
};

/* Y1(x)cosX - J1(x)sinX = sqrt( 2/(pi x)) Q1(x),
   Q1(x) = z(.375 + z^2 R(z^2)), z=1/x
   2 <= x <= 2.85711669921875l
   Peak relative error 6.9e-22l */
#ifdef __STDC__
static const Extended qr2[7] = {
#else
static Extended qr2[7] = {
#endif
  -1.372751603025230017220666013816502528318E-4l,
  -6.879190253347766576229143006767218972834E-3l,
  -1.061253572090925414598304855316280077828E-1l,
  -6.262164224345471241219408329354943337214E-1l,
  -1.423149636514768476376254324731437473915E0l,
  -1.087955310491078933531734062917489870754E0l,
  -1.826821119773182847861406108689273719137E-1l,
};
#ifdef __STDC__
static const Extended qs2[7] = {
#else
static Extended qs2[7] = {
#endif
  1.338768933634451601814048220627185324007E-3l,
  7.071099998918497559736318523932241901810E-2l,
  1.200511429784048632105295629933382142221E0l,
  8.327301713640367079030141077172031825276E0l,
  2.468479301872299311658145549931764426840E1l,
  2.961179686096262083509383820557051621644E1l,
  1.201402313144305153005639494661767354977E1l,
 /* 1.000000000000000000000000000000000000000E0l, */
};


#ifdef __STDC__
static Extended
qone (Extended x)
#else
static Extended
qone (x)
     Extended x;
#endif
{
#ifdef __STDC__
  const Extended *p, *q;
#else
  Extended *p, *q;
#endif
  static Extended s, r, z;
  int32_t ix;
  u_int32_t se, i0, i1;

  GET_LDOUBLE_WORDS (se, i0, i1, x);
  ix = se & 0x7fff;
  if (ix >= 0x4002)		/* x >= 8 */
    {
      p = qr8;
      q = qs8;
    }
  else
    {
      i1 = (ix << 16) | (i0 >> 16);
      if (i1 >= 0x40019174)	/* x >= 4.54541015625l */
	{
	  p = qr5;
	  q = qs5;
	}
      else if (i1 >= 0x4000b6db)	/* x >= 2.85711669921875l */
	{
	  p = qr3;
	  q = qs3;
	}
      else if (ix >= 0x4000)	/* x better be >= 2 */
	{
	  p = qr2;
	  q = qs2;
	}
    }
  z = one / (x * x);
  r =
    p[0] + z * (p[1] +
		z * (p[2] + z * (p[3] + z * (p[4] + z * (p[5] + z * p[6])))));
  s =
    q[0] + z * (q[1] +
		z * (q[2] +
		     z * (q[3] + z * (q[4] + z * (q[5] + z * (q[6] + z))))));
  return (.375 + z * r / s) / x;
}
}
