/* BSE - Better Sound Engine
 * Copyright (C) 2006 Tim Janik
 * Copyright (C) 1984, 1987, 1988, 1989, 1995, 2000 Stephen L. Moshier
 *
 * This software is provided "as is"; redistribution and modification
 * is permitted, provided that the following disclaimer is retained.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * In no event shall the authors or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 */
#define _ISOC99_SOURCE  /* for INFINITY and NAN */
#define _GNU_SOURCE     /* provides: _ISOC99_SOURCE */
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "bsefilter.h"

typedef struct {
  int    n_poles;
  int    n_zeros;
  int    z_counter;	/* incremented as z^N coefficients are found, indexes poles and zeros */
  int    n_solved_poles;
  /* common state */
  double gain_scale;
  double ripple_epsilon;
  double nyquist_frequency;
  double tan_angle_frequency;
  double wc;                    /* tan_angle_frequency or normalized to 1.0 for elliptic */
  double cgam;                  /* cos (band_filter_center_frequency) */
  double stopband_edge;         /* derived from ifr->stopband_edge or ifr->stopband_db */
  double wr;
  double numerator_accu;
  double denominator_accu;
  /* chebyshev state */
  double chebyshev_phi;
  double chebyshev_band_cbp;
  /* elliptic state */
  double elliptic_phi;
  double elliptic_k;
  double elliptic_u;
  double elliptic_m;
  double elliptic_Kk;  /* complete elliptic integral of the first kind of 1-elliptic_m */
  double elliptic_Kpk; /* complete elliptic integral of the first kind of elliptic_m */
  /* common output */
  double  gain;
  double  spz[BSE_IIR_CARRAY_SIZE];	        /* s-plane poles and zeros */
  BseComplex zcpz[BSE_IIR_CARRAY_SIZE];	/* z-plane poles and zeros */
  /* normalized z-plane transfer function */
  double  zn[BSE_IIR_CARRAY_SIZE];              /* numerator coefficients [order+1] */
  double  zd[BSE_IIR_CARRAY_SIZE];              /* denominator coefficients [order+1] */
} EllfDesignState;

#define MACHEP                  (BSE_DOUBLE_EPSILON)                    /* the machine roundoff error */
#define PIO2                    (BSE_PI_DIV_2)                          /* pi/2 */
#define MAXNUM                  (BSE_DOUBLE_MAX_NORMAL)                 /* 2**1024*(1-MACHEP) */

#if 0
#define error_printf(...)       fprintf (stderr, __VA_ARGS__)
#define ellf_debugf(...)        fprintf (stderr, __VA_ARGS__)
#define ellf_outputf(...)       fprintf (stdout, __VA_ARGS__)
#define ellf_inputf(...)        fprintf (stdout, __VA_ARGS__)
#else
#define error_printf(...)       while (0) { printf (__VA_ARGS__); }
#define ellf_debugf(...)        while (0) { printf (__VA_ARGS__); }
#define ellf_outputf(...)       while (0) { printf (__VA_ARGS__); }
#define ellf_inputf(...)        while (0) { printf (__VA_ARGS__); }
#endif

static const char* ellf_filter_design (const BseIIRFilterRequest *ifr,
                                       EllfDesignState           *ds);

bool
_bse_filter_design_ellf (const BseIIRFilterRequest      *ifr,
                         BseIIRFilterDesign             *fid)
{
  EllfDesignState ds = { 0, };
  const char *errmsg = ellf_filter_design (ifr, &ds);
  fflush (stdout);
  fflush (stderr);
  // ellf_debugf ("DEBUG: %.20g %.20g %.20g %.20g %.20g\n", a, cos(a), cang, ds->cgam, 0.);
  if (errmsg)
    return false;
  fid->order = ds.n_solved_poles;
  fid->sampling_frequency = ifr->sampling_frequency;
  fid->gain = ds.gain;
  fid->n_zeros = 0;
  fid->n_poles = 0;
  uint i;
  for (i = 0; i < ds.n_solved_poles; i++)
    {
      double a = ds.zcpz[i].re, b = ds.zcpz[i].im;
      if (b >= 0.0)
        fid->zp[fid->n_poles++] = bse_complex (a, b);
      a = ds.zcpz[ds.n_solved_poles + i].re, b = ds.zcpz[ds.n_solved_poles + i].im;
      if (b >= 0.0)
        fid->zz[fid->n_zeros++] = bse_complex (a, b);
    }
  for (i = 0; i <= fid->order; i++)
    {
#if 0
      fid->zn[i] = ds.zn[i];
      fid->zd[i] = ds.zd[i];
#endif
    }
  if (ifr->type == BSE_IIR_FILTER_LOW_PASS)
    fid->center_frequency = 0;
  else if (ifr->type == BSE_IIR_FILTER_HIGH_PASS)
    fid->center_frequency = 0.5 * fid->sampling_frequency;
  else /* band filter */
    fid->center_frequency = 0.5 * fid->sampling_frequency / PI * (PIO2 - asin (ds.cgam));
  return true;
}

/* This code calculates design coefficients for
 * digital filters of the Butterworth, Chebyshev, or
 * elliptic varieties.
 *
 * The program displays relevant pass band and stop band edge
 * frequencies and stop band attenuation. The z-plane coefficients
 * are printed in these forms:
 *   Numerator and denominator z polynomial coefficients
 *   Pole and zero locations
 *   Polynomial coefficients of quadratic factors
 * 
 * After giving all the coefficients, the program prints a
 * table of the frequency response of the filter.  You can
 * get a picture by reading the table into gnuplot.
 * 
 * Filter design:
 * 
 * The output coefficients of primary interest are shown as follows:
 * 
 * (z-plane pole location:)
 * pole     3.0050282041410E-001    9.3475816516366E-001
 * (quadratic factors:)
 * q. f.
 * z**2    9.6407477241696E-001
 * z**1   -6.0100564082819E-001
 * (center frequency, gain at f0, and gain at 0 Hz:)
 * f0  2.00496167E+003  gain  2.9238E+001  DC gain  7.3364E-001
 * 
 * zero     1.7886295237392E-001    9.8387399816648E-001
 * q. f.
 * z**2    1.0000000000000E+000
 * z**1   -3.5772590474783E-001
 * f0  2.21379064E+003  gain  0.0000E+000  DC gain  1.6423E+000
 * 
 * To make a biquad filter from this, the equation for the
 * output y(i) at the i-th sample as a function of the input
 * x(i) at the i-th sample is
 * 
 * y(i) + -6.0100564082819E-001 y(i-1) +  9.6407477241696E-001 y(i-2)
 * = x(i) + -3.5772590474783E-001 x(i-1) +  1.0000000000000E+000 x(i-2).
 * 
 * Thus the two coefficients for the pole would normally be
 * negated in a typical implementation of the filter.
 * 
 * References:
 * 
 * A. H. Gray, Jr., and J. D. Markel, "A Computer Program for
 * Designing Digital Elliptic Filters", IEEE Transactions on
 * Acoustics, Speech, and Signal Processing 6, 529-538
 * (December, 1976)
 * 
 * B. Gold and C. M. Rader, Digital Processing of Signals,
 * McGraw-Hill, Inc. 1969, pp 61-90
 * 
 * M. Abramowitz and I. A. Stegun, eds., Handbook of Mathematical
 * Functions, National Bureau of Standards AMS 55, 1964,
 * Chapters 16 and 17
 */

/* --- prototypes --- */
static double ellik  (double phi, double m); // incomplete elliptic integral of the first kind
static double ellpk  (double x); // complete elliptic integral of the first kind
static int    ellpj  (double u, double m, double *sn, double *cn, double *dn, double *ph); // Jacobian Elliptic Functions
static int    math_set_error (char *name, int code);

/* --- math errors --- */
static int math_global_error = 0;
#define MATH_ERROR_DOMAIN		1	/* argument domain error */
#define MATH_ERROR_SING		        2	/* argument singularity */
#define MATH_ERROR_OVERFLOW	        3	/* overflow range error */
#define MATH_ERROR_UNDERFLOW	        4	/* underflow range error */
#define MATH_ERROR_TOTAL_LOSS		5	/* total loss of precision */
#define MATH_ERROR_PARTIAL_LOSS		6	/* partial loss of precision */

/* Common error handling routine
 *
 * SYNOPSIS:
 * char *fctnam;
 * int code;
 * int math_set_error();
 *
 * math_set_error(fctnam, code);
 *
 * DESCRIPTION:
 * This routine may be called to report one of the following
 * error conditions (in the include file mconf.h).
 *   Mnemonic        Value          Significance
 *    MATH_ERROR_DOMAIN            1       argument domain error
 *    MATH_ERROR_SING              2       function singularity
 *    MATH_ERROR_OVERFLOW          3       overflow range error
 *    MATH_ERROR_UNDERFLOW         4       underflow range error
 *    MATH_ERROR_TOTAL_LOSS        5       total loss of precision
 *    MATH_ERROR_PARTIAL_LOSS      6       partial loss of precision
 *
 * The default version of the file prints the function name,
 * passed to it by the pointer fctnam, followed by the
 * error condition.  The display is directed to the standard
 * output device.  The routine then returns to the calling
 * program.  Users may wish to modify the program to abort by
 * calling exit() under severe error conditions such as domain
 * errors.
 *
 * Since all error conditions pass control to this function,
 * the display may be easily changed, eliminated, or directed
 * to an error logging device.
 */
static int
math_set_error (char *name, int code)
{
  /* Notice: the order of appearance of the following
   * messages is bound to the error codes defined
   * in mconf.h.
   */
  static const char *ermsg[7] = {
    "unknown",      /* error code 0 */
    "domain",       /* error code 1 */
    "singularity",  /* et seq.      */
    "overflow",
    "underflow",
    "total loss of precision",
    "partial loss of precision"
  };
  
  /* Display string passed by calling program,
   * which is supposed to be the name of the
   * function in which the error occurred:
   */
  error_printf ("\n%s ", name);
  
  /* Set global error message word */
  math_global_error = code;
  
  /* Display error message defined
   * by the code argument.
   */
  if ((code <= 0) || (code >= 7))
    code = 0;
  error_printf ("%s error\n", ermsg[code]);
  
  /* Return to calling
   * program
   */
  return 0;
}

/* --- elliptic functions --- */
/* ellik.c - Incomplete elliptic integral of the first kind
 *
 * SYNOPSIS:
 * double phi, m, y, ellik();
 * y = ellik (phi, m);
 *
 * DESCRIPTION:
 * Approximates the integral
 *                phi
 *                 -
 *                | |
 *                |           dt
 * F(phi_\m)  =    |    ------------------
 *                |                   2
 *              | |    sqrt(1 - m sin t)
 *               -
 *                0
 * of amplitude phi and modulus m, using the arithmetic -
 * geometric mean algorithm.
 *
 * ACCURACY:
 * Tested at random points with m in [0, 1] and phi as indicated.
 *                      Relative error:
 * arithmetic   domain     # trials      peak         rms
 *    IEEE     -10,10       200000      7.4e-16     1.0e-16
 */
static double
ellik (double phi, double m)
{
  double a, b, c, e, temp, t, K;
  int d, mod, sign, npio2;
  
  if (m == 0.0)
    return phi;
  a = 1.0 - m;
  if (a == 0.0)
    {
      if (fabs (phi) >= PIO2)
        {
          math_set_error ("ellik", MATH_ERROR_SING);
          return MAXNUM;
        }
      return log ( tan ((PIO2 + phi) / 2.0) );
    }
  npio2 = floor (phi/PIO2);
  if (npio2 & 1)
    npio2 += 1;
  if (npio2)
    {
      K = ellpk (a);
      phi = phi - npio2 * PIO2;
    }
  else
    K = 0.0;
  if (phi < 0.0)
    {
      phi = -phi;
      sign = -1;
    }
  else
    sign = 0;
  b = sqrt (a);
  t = tan (phi);
  if (fabs (t) > 10.0)
    {
      /* Transform the amplitude */
      e = 1.0/(b*t);
      /* ... but avoid multiple recursions.  */
      if (fabs (e) < 10.0)
        {
          e = atan (e);
          if (npio2 == 0)
            K = ellpk (a);
          temp = K - ellik (e, m);
          goto done;
        }
    }
  a = 1.0;
  c = sqrt (m);
  d = 1;
  mod = 0;
  
  while (fabs (c/a) > MACHEP)
    {
      temp = b/a;
      phi = phi + atan (t*temp) + mod * PI;
      mod = (phi + PIO2)/PI;
      t = t * (1.0 + temp)/(1.0 - temp * t * t);
      c = (a - b)/2.0;
      temp = sqrt (a * b);
      a = (a + b)/2.0;
      b = temp;
      d += d;
    }
  
  temp = (atan (t) + mod * PI)/(d * a);
  
 done:
  if (sign < 0)
    temp = -temp;
  temp += npio2 * K;
  return temp;
}

/* ellpj - Jacobian Elliptic Functions
 *
 * SYNOPSIS:
 * double u, m, sn, cn, dn, phi;
 * int ellpj();
 * ellpj (u, m, _&sn, _&cn, _&dn, _&phi);
 *
 * DESCRIPTION:
 * Evaluates the Jacobian elliptic functions sn(u|m), cn(u|m),
 * and dn(u|m) of parameter m between 0 and 1, and real
 * argument u.
 *
 * These functions are periodic, with quarter-period on the
 * real axis equal to the complete elliptic integral
 * ellpk(1.0-m).
 *
 * Relation to incomplete elliptic integral:
 * If u = ellik(phi,m), then sn(u|m) = sin(phi),
 * and cn(u|m) = cos(phi).  Phi is called the amplitude of u.
 *
 * Computation is by means of the arithmetic-geometric mean
 * algorithm, except when m is within 1e-9 of 0 or 1.  In the
 * latter case with m close to 1, the approximation applies
 * only for phi < pi/2.
 *
 * ACCURACY:
 * Tested at random points with u between 0 and 10, m between
 * 0 and 1.
 *            Absolute error (* = relative error):
 * arithmetic   function   # trials      peak         rms
 *    DEC       sn           1800       4.5e-16     8.7e-17
 *    IEEE      phi         10000       9.2e-16*    1.4e-16*
 *    IEEE      sn          50000       4.1e-15     4.6e-16
 *    IEEE      cn          40000       3.6e-15     4.4e-16
 *    IEEE      dn          10000       1.3e-12     1.8e-14
 * Peak error observed in consistency check using addition
 * theorem for sn(u+v) was 4e-16 (absolute).  Also tested by
 * the above relation to the incomplete elliptic integral.
 * Accuracy deteriorates when u is large.
 */
static int
ellpj (double u, double m,
       double *sn, double *cn, double *dn,
       double *ph)
{
  double ai, b, phi, t, twon;
  double a[9], c[9];
  int i;
  /* Check for special cases */
  if (m < 0.0 || m > 1.0)
    {
      math_set_error ("ellpj", MATH_ERROR_DOMAIN);
      *sn = 0.0;
      *cn = 0.0;
      *ph = 0.0;
      *dn = 0.0;
      return -1;
    }
  if (m < 1.0e-9)
    {
      t = sin (u);
      b = cos (u);
      ai = 0.25 * m * (u - t*b);
      *sn = t - ai*b;
      *cn = b + ai*t;
      *ph = u - ai;
      *dn = 1.0 - 0.5*m*t*t;
      return 0;
    }
  
  if (m >= 0.9999999999)
    {
      ai = 0.25 * (1.0-m);
      b = cosh (u);
      t = tanh (u);
      phi = 1.0/b;
      twon = b * sinh (u);
      *sn = t + ai * (twon - u)/(b*b);
      *ph = 2.0*atan (exp (u)) - PIO2 + ai*(twon - u)/b;
      ai *= t * phi;
      *cn = phi - ai * (twon - u);
      *dn = phi + ai * (twon + u);
      return 0;
    }
  /*	A. G. M. scale		*/
  a[0] = 1.0;
  b = sqrt (1.0 - m);
  c[0] = sqrt (m);
  twon = 1.0;
  i = 0;
  
  while (fabs (c[i]/a[i]) > MACHEP)
    {
      if (i > 7)
        {
          math_set_error ("ellpj", MATH_ERROR_OVERFLOW);
          goto done;
        }
      ai = a[i];
      ++i;
      c[i] = (ai - b)/2.0;
      t = sqrt (ai * b);
      a[i] = (ai + b)/2.0;
      b = t;
      twon *= 2.0;
    }
  
 done:
  /* backward recurrence */
  phi = twon * a[i] * u;
  do
    {
      t = c[i] * sin (phi) / a[i];
      b = phi;
      phi = (asin (t) + phi)/2.0;
    }
  while (--i);
  
  *sn = sin (phi);
  t = cos (phi);
  *cn = t;
  *dn = t/cos (phi-b);
  *ph = phi;
  return 0;
}

/* polevl - Evaluate polynomial
 *
 * SYNOPSIS:
 * int N;
 * double x, y, coef[N+1], polevl[];
 * y = polevl(x, coef, N);
 *
 * DESCRIPTION:
 * Evaluates polynomial of degree N:
 *                     2          N
 * y  =  C  + C x + C x  +...+ C x
 *        0    1     2          N
 *
 * Coefficients are stored in reverse order:
 * coef[0] = C  , ..., coef[N] = C  .
 *            N                   0
 * SPEED:
 * In the interest of speed, there are no checks for out
 * of bounds arithmetic.  This routine is used by most of
 * the functions in the library.  Depending on available
 * equipment features, the user may wish to rewrite the
 * program in microcode or assembly language.
 */
static inline double
polevl (double x, const double coef[], int N)
{
  double ans;
  int i;
  const double *p;
  
  p = coef;
  ans = *p++;
  i = N;
  
  do
    ans = ans * x  +  *p++;
  while (--i);
  
  return ans;
}

/* ellpk - Complete elliptic integral of the first kind
 *
 * SYNOPSIS:
 * double m1, y, ellpk();
 * y = ellpk (m1);
 *
 * DESCRIPTION:
 * Approximates the integral
 *            pi/2
 *             -
 *            | |
 *            |           dt
 * K(m)  =    |    ------------------
 *            |                   2
 *          | |    sqrt(1 - m sin t)
 *           -
 *            0
 * where m = 1 - m1, using the approximation
 *     P(x)  -  log x Q(x).
 *
 * The argument m1 is used rather than m so that the logarithmic
 * singularity at m = 1 will be shifted to the origin; this
 * preserves maximum accuracy.
 * K(0) = pi/2.
 *
 * ACCURACY:
 *                      Relative error:
 * arithmetic   domain     # trials      peak         rms
 *    DEC        0,1        16000       3.5e-17     1.1e-17
 *    IEEE       0,1        30000       2.5e-16     6.8e-17
 * ERROR MESSAGES:
 *   message         condition      value returned
 * ellpk domain       x<0, x>1           0.0
 */
static double
ellpk (double x)
{
  static const double P_ellpk[] = {
    1.37982864606273237150E-4,
    2.28025724005875567385E-3,
    7.97404013220415179367E-3,
    9.85821379021226008714E-3,
    6.87489687449949877925E-3,
    6.18901033637687613229E-3,
    8.79078273952743772254E-3,
    1.49380448916805252718E-2,
    3.08851465246711995998E-2,
    9.65735902811690126535E-2,
    1.38629436111989062502E0
  };
  static const double Q_ellpk[] = {
    2.94078955048598507511E-5,
    9.14184723865917226571E-4,
    5.94058303753167793257E-3,
    1.54850516649762399335E-2,
    2.39089602715924892727E-2,
    3.01204715227604046988E-2,
    3.73774314173823228969E-2,
    4.88280347570998239232E-2,
    7.03124996963957469739E-2,
    1.24999999999870820058E-1,
    4.99999999999999999821E-1
  };

  if ((x < 0.0) || (x > 1.0))
    {
      math_set_error ("ellpk", MATH_ERROR_DOMAIN);
      return 0.0;
    }
  
  if (x > MACHEP)
    {
      return polevl (x,P_ellpk,10) - log (x) * polevl (x,Q_ellpk,10);
    }
  else
    {
      if (x == 0.0)
        {
          math_set_error ("ellpk", MATH_ERROR_SING);
          return MAXNUM;
        }
      else
        {
          /* C1 = ln(4) */
          return BSE_LN4 - 0.5 * log (x); /* C1 - 0.5 * log (x) */
        }
    }
}

/* jacobi_theta_by_nome():
 * Find parameter corresponding to given nome by expansion
 * in theta functions:
 * AMS55 #16.38.5, 16.38.7
 *
 *       1/2
 * (2K)                   4     9
 * (--)     =  1 + 2q + 2q  + 2q  + ...  =  Theta (0,q)
 * (pi)                                          3
 *
 *
 *       1/2
 * (2K)     1/4       1/4        2    6    12    20
 * (--)    m     =  2q    (1 + q  + q  + q   + q   + ...) = Theta (0,q)
 * (pi)                                                           2
 *
 * The nome q(m) = exp(- pi K(1-m)/K(m)).
 *
 *                                1/2
 * Given q, this program returns m   .
 */
static double
jacobi_theta_by_nome (double q)
{
  double t1, a = 1.0, b = 1.0, r = 1.0, p = q;
  do
    {
      r *= p;
      a += 2.0 * r;
      t1 = fabs (r / a);
      
      r *= p;
      b += r;
      p *= q;
      double t2 = fabs (r / b);
      if (t2 > t1)
	t1 = t2;
    }
  while (t1 > MACHEP);
  a = b / a;
  a = 4.0 * sqrt (q) * a * a;	/* see above formulas, solved for m */
  return a;
}

/* --- filter design functions --- */
static int
find_elliptic_locations_in_lambda_plane (const BseIIRFilterRequest *ifr,
                                         EllfDesignState           *ds)
{
  double k = ds->wc / ds->wr;
  double m = k * k;
  ds->elliptic_Kk = ellpk (1.0 - m);
  ds->elliptic_Kpk = ellpk (m);
  ellf_debugf ("check: k=%.20g m=%.20g Kk=%.20g Kpk=%.20g\n", k, m, ds->elliptic_Kk, ds->elliptic_Kpk); // BSE info
  double q = exp (-PI * ifr->order * ds->elliptic_Kpk / ds->elliptic_Kk);	/* the nome of k1 */
  double m1 = jacobi_theta_by_nome (q); /* see below */
  /* Note m1 = ds->ripple_epsilon / sqrt(A*A - 1.0) */
  double a = ds->ripple_epsilon / m1;
  a =  a * a + 1;
  a = 10.0 * log (a) / log (10.0);
  ellf_debugf ("dbdown %.9E\n", a);
  a = 180.0 * asin (k) / PI;
  double b = 1.0 / (1.0 + ds->ripple_epsilon * ds->ripple_epsilon);
  b = sqrt (1.0 - b);
  ellf_debugf ("theta=%.9E, rho=%.9E\n", a, b);
  m1 *= m1;
  double m1p = 1.0 - m1;
  double Kk1 = ellpk (m1p);
  double Kpk1 = ellpk (m1);
  double r = Kpk1 * ds->elliptic_Kk / (Kk1 * ds->elliptic_Kpk);
  ellf_debugf ("consistency check: n= %.14E\n", r);
  ellf_debugf ("consistency check: r=%.20g Kpk1=%.20g Kk1=%.20g m1=%.20g m1p=%.20g\n", r, Kpk1, Kk1, m1, m1p); // BSE info
  /*   -1
   * sn   j/ds->ripple_epsilon\m  =  j ellik(atan(1/ds->ripple_epsilon), m)
   */
  b = 1.0 / ds->ripple_epsilon;
  ds->elliptic_phi = atan (b);
  double u = ellik (ds->elliptic_phi, m1p);
  ellf_debugf ("phi=%.20g m=%.20g u=%.20g\n", ds->elliptic_phi, m1p, u);
  /* consistency check on inverse sn */
  double sn, cn, dn;
  ellpj (u, m1p, &sn, &cn, &dn, &ds->elliptic_phi);
  a = sn / cn;
  ellf_debugf ("consistency check: sn/cn = %.20g = %.20g = 1/ripple\n", a, b);
  ds->elliptic_k = k;
  ds->elliptic_u = u * ds->elliptic_Kk / (ifr->order * Kk1);	/* or, u = u * Kpk / Kpk1 */
  ds->elliptic_m = m;
  return 0;
}

/* calculate s plane poles and zeros, normalized to wc = 1 */
static int
find_s_plane_poles_and_zeros (const BseIIRFilterRequest *ifr,
                              EllfDesignState           *ds)
{
  double *spz = ds->spz;
  int i, j;
  for (i = 0; i < BSE_IIR_CARRAY_SIZE; i++)
    spz[i] = 0.0;
  ds->n_poles = (ifr->order + 1) / 2;
  ds->n_zeros = 0;
  if (ifr->kind == BSE_IIR_FILTER_BUTTERWORTH)
    {
      double m;
      /* Butterworth poles equally spaced around the unit circle */
      if (ifr->order & 1)
        m = 0.0;
      else
        m = PI / (2.0 * ifr->order);
      for (i = 0; i < ds->n_poles; i++)
        {	/* poles */
          int lr = i + i;
          spz[lr] = -cos (m);
          spz[lr + 1] = sin (m);
          m += PI / ifr->order;
        }	
      /* high pass or band reject
       */
      if (ifr->type == BSE_IIR_FILTER_HIGH_PASS || ifr->type == BSE_IIR_FILTER_BAND_STOP)
        {
          int ii = 0;
          /* map s => 1/s */
          for (j = 0; j < ds->n_poles; j++)
            {
              int ir = j + j;
              ii = ir + 1;
              double b = spz[ir]*spz[ir] + spz[ii]*spz[ii];
              spz[ir] = spz[ir] / b;
              spz[ii] = spz[ii] / b;
            }
          /* The zeros at infinity map to the origin.
           */
          ds->n_zeros = ds->n_poles;
          if (ifr->type == BSE_IIR_FILTER_BAND_STOP)
            {
              ds->n_zeros += ifr->order / 2;
            }
          for (j = 0; j < ds->n_zeros; j++)
            {
              int ir = ii + 1;
              ii = ir + 1;
              spz[ir] = 0.0;
              spz[ii] = 0.0;
            }
        }
    }
  if (ifr->kind == BSE_IIR_FILTER_CHEBYSHEV1)
    {
      /* For Chebyshev, find radii of two Butterworth circles
       * See Gold & Rader, page 60
       */
      double rho = (ds->chebyshev_phi - 1.0) * (ds->chebyshev_phi + 1);  /* rho = ds->ripple_epsilon^2 = {sqrt(1+ds->ripple_epsilon^2)}^2 - 1 */
      ds->ripple_epsilon = sqrt (rho);
      /* sqrt(1 + 1/ds->ripple_epsilon^2) + 1/ds->ripple_epsilon  = {sqrt(1 + ds->ripple_epsilon^2)  +  1} / ds->ripple_epsilon
       */
      ds->chebyshev_phi = (ds->chebyshev_phi + 1.0) / ds->ripple_epsilon;
      ellf_debugf ("Chebychev: phi-before=%.20g ripple=%.20g\n", ds->chebyshev_phi, ds->ripple_epsilon); // BSE info
      ds->chebyshev_phi = pow (ds->chebyshev_phi, 1.0 / ifr->order);  /* raise to the 1/n power */
      ellf_debugf ("Chebychev: phi-raised=%.20g rn=%.20g\n", ds->chebyshev_phi, ifr->order * 1.0); // BSE info
      double b = 0.5 * (ds->chebyshev_phi + 1.0 / ds->chebyshev_phi); /* y coordinates are on this circle */
      double a = 0.5 * (ds->chebyshev_phi - 1.0 / ds->chebyshev_phi); /* x coordinates are on this circle */
      double m;
      if (ifr->order & 1)
        m = 0.0;
      else
        m = PI / (2.0 * ifr->order);
      for (i = 0; i < ds->n_poles; i++)
        {	/* poles */
          int lr = i + i;
          spz[lr] = -a * cos (m);
          spz[lr + 1] = b * sin (m);
          m += PI / ifr->order;
        }	
      /* high pass or band reject
       */
      if (ifr->type == BSE_IIR_FILTER_HIGH_PASS || ifr->type == BSE_IIR_FILTER_BAND_STOP)
        {
          int ii = 0;
          /* map s => 1/s */
          for (j = 0; j < ds->n_poles; j++)
            {
              int ir = j + j;
              ii = ir + 1;
              b = spz[ir]*spz[ir] + spz[ii]*spz[ii];
              spz[ir] = spz[ir] / b;
              spz[ii] = spz[ii] / b;
            }
          /* The zeros at infinity map to the origin.
           */
          ds->n_zeros = ds->n_poles;
          if (ifr->type == BSE_IIR_FILTER_BAND_STOP)
            {
              ds->n_zeros += ifr->order / 2;
            }
          for (j = 0; j < ds->n_zeros; j++)
            {
              int ir = ii + 1;
              ii = ir + 1;
              spz[ir] = 0.0;
              spz[ii] = 0.0;
            }
        }
    }
  if (ifr->kind == BSE_IIR_FILTER_ELLIPTIC)
    {
      double phi1 = 0.0, m = ds->elliptic_m;
      ds->n_zeros = ifr->order / 2;
      double sn1, cn1, dn1;
      ellpj (ds->elliptic_u, 1.0 - m, &sn1, &cn1, &dn1, &phi1);
      for (i = 0; i < ds->n_zeros; i++)
        {	/* zeros */
          double a = ifr->order - 1 - i - i;
          double b = (ds->elliptic_Kk * a) / ifr->order;
          double sn, cn, dn;
          ellpj (b, m, &sn, &cn, &dn, &ds->elliptic_phi);
          int lr = 2 * ds->n_poles + 2 * i;
          spz[lr] = 0.0;
          a = ds->wc / (ds->elliptic_k * sn);	/* elliptic_k = sqrt(m) */
          spz[lr + 1] = a;
        }
      for (i = 0; i < ds->n_poles; i++)
        {	/* poles */
          double a = ifr->order - 1 - i - i;
          double b = a * ds->elliptic_Kk / ifr->order;
          double sn, cn, dn;
          ellpj (b, m, &sn, &cn, &dn, &ds->elliptic_phi);
          double r = ds->elliptic_k * sn * sn1;
          b = cn1 * cn1 + r * r;
          a = -ds->wc * cn * dn * sn1 * cn1 / b;
          int lr = i + i;
          spz[lr] = a;
          b = ds->wc * sn * dn1 / b;
          spz[lr + 1] = b;
        }	
      if (ifr->type == BSE_IIR_FILTER_HIGH_PASS || ifr->type == BSE_IIR_FILTER_BAND_STOP)
        {
          int ii = 0, nt = ds->n_poles + ds->n_zeros;
          for (j = 0; j < nt; j++)
            {
              int ir = j + j;
              ii = ir + 1;
              double b = spz[ir]*spz[ir] + spz[ii]*spz[ii];
              spz[ir] = spz[ir] / b;
              spz[ii] = spz[ii] / b;
            }
          while (ds->n_poles > ds->n_zeros)
            {
              int ir = ii + 1;
              ii = ir + 1;
              ds->n_zeros += 1;
              spz[ir] = 0.0;
              spz[ii] = 0.0;
            }
        }
    }
  ellf_outputf ("s plane poles:\n");
  j = 0;
  for (i = 0; i < ds->n_poles + ds->n_zeros; i++)
    {
      double a = spz[j];
      ++j;
      double b = spz[j];
      ++j;
      ellf_outputf ("%.9E %.9E\n", a, b);
      if (i == ds->n_poles - 1)
        ellf_outputf ("s plane zeros:\n");
    }
  return 0;
}

/* convert s plane poles and zeros to the z plane. */
static int
convert_s_plane_to_z_plane (const BseIIRFilterRequest *ifr,
                            EllfDesignState           *ds)
{
  BseComplex r, cnum, cden, cwc, ca, cb, b4ac;
  double C;
  
  if (ifr->kind == BSE_IIR_FILTER_ELLIPTIC)
    C = ds->tan_angle_frequency;
  else
    C = ds->wc;
  
  int i;
  for (i = 0; i < BSE_IIR_CARRAY_SIZE; i++)
    {
      ds->zcpz[i].re = 0.0;
      ds->zcpz[i].im = 0.0;
    }

  int nc = ds->n_poles;
  ds->z_counter = -1;
  int icnt, ii = -1;
  for (icnt = 0; icnt < 2; icnt++)
    {
      BseComplex *z_pz = ds->zcpz;
      /* The maps from s plane to z plane */
      do
	{
          int ir = ii + 1;
          ii = ir + 1;
          r.re = ds->spz[ir];
          r.im = ds->spz[ii];
          
          switch (ifr->type)
            {
            case BSE_IIR_FILTER_LOW_PASS:
            case BSE_IIR_FILTER_HIGH_PASS:
              /* Substitute  s - r  =  s/wc - r = (1/wc)(z-1)/(z+1) - r
               *
               *     1  1 - r wc (       1 + r wc)
               * =  --- -------- (z  -  --------)
               *    z+1    wc    (       1 - r wc)
               *
               * giving the root in the z plane.
               */
              cnum.re = 1 + C * r.re;
              cnum.im = C * r.im;
              cden.re = 1 - C * r.re;
              cden.im = -C * r.im;
              ds->z_counter += 1;
              z_pz[ds->z_counter] = bse_complex_div (cnum, cden);
              if (r.im != 0.0)
                {
                  /* fill in complex conjugate root */
                  ds->z_counter += 1;
                  z_pz[ds->z_counter].re = z_pz[ds->z_counter - 1].re;
                  z_pz[ds->z_counter].im = -z_pz[ds->z_counter - 1].im;
                }
              break;
            case BSE_IIR_FILTER_BAND_PASS:
            case BSE_IIR_FILTER_BAND_STOP:
              /* Substitute  s - r  =>  s/wc - r
               *
               *     z^2 - 2 z cgam + 1
               * =>  ------------------  -  r
               *         (z^2 + 1) wc  
               *
               *         1
               * =  ------------  [ (1 - r wc) z^2  - 2 cgam z  +  1 + r wc ]
               *    (z^2 + 1) wc  
               *
               * and solve for the roots in the z plane.
               */
              if (ifr->kind == BSE_IIR_FILTER_CHEBYSHEV1)
                cwc.re = ds->chebyshev_band_cbp;
              else
                cwc.re = ds->tan_angle_frequency;
              cwc.im = 0.0;
              cnum = bse_complex_mul (r, cwc);
              ca = bse_complex_sub (BSE_COMPLEX_ONE, cnum);   /* a = 1 - r wc */
              b4ac = bse_complex_mul (cnum, cnum); /* 1 - (r wc)^2 */
              b4ac = bse_complex_sub (BSE_COMPLEX_ONE, b4ac);
              b4ac.re *= 4.0;               /* 4ac */
              b4ac.im *= 4.0;
              cb.re = -2.0 * ds->cgam;          /* b */
              cb.im = 0.0;
              cnum = bse_complex_mul (cb, cb);     /* b^2 */
              b4ac = bse_complex_sub (cnum, b4ac); /* b^2 - 4 ac */
              b4ac = bse_complex_sqrt (b4ac);
              cb.re = -cb.re;  /* -b */
              cb.im = -cb.im;
              ca.re *= 2.0; /* 2a */
              ca.im *= 2.0;
              cnum = bse_complex_add (b4ac, cb);   /* -b + sqrt(b^2 - 4ac) */
              cnum = bse_complex_div (cnum, ca);
              ds->z_counter += 1;
              z_pz[ds->z_counter] = cnum;
              if (cnum.im != 0.0)
                {
                  ds->z_counter += 1;
                  z_pz[ds->z_counter].re = cnum.re;
                  z_pz[ds->z_counter].im = -cnum.im;
                }
              if ((r.im != 0.0) || (cnum.im == 0))
                {
                  cnum = bse_complex_sub (cb, b4ac);  /* -b - sqrt(b^2 - 4ac) */
                  cnum = bse_complex_div (cnum, ca);  /* ... /2a */
                  ds->z_counter += 1;
                  z_pz[ds->z_counter] = cnum;
                  if (cnum.im != 0.0)
                    {
                      ds->z_counter += 1;
                      z_pz[ds->z_counter].re = cnum.re;
                      z_pz[ds->z_counter].im = -cnum.im;
                    }
                }
              break;
            } /* end switch */
	}
      while (--nc > 0);
      
      if (icnt == 0)
	{
          ds->n_solved_poles = ds->z_counter + 1;
          if (ds->n_zeros <= 0)
            {
              if (ifr->kind == BSE_IIR_FILTER_BUTTERWORTH || ifr->kind == BSE_IIR_FILTER_CHEBYSHEV1)
                return 0;
              else
                break;
            }
	}
      nc = ds->n_zeros;
    } /* end for() loop */
  return 0;
}

static int
z_plane_zeros_poles_to_numerator_denomerator (const BseIIRFilterRequest *ifr,
                                              EllfDesignState           *ds)
{
  BseComplex lin[2];
  
  lin[1].re = 1.0;
  lin[1].im = 0.0;
  
  if (ifr->kind == BSE_IIR_FILTER_BUTTERWORTH || ifr->kind == BSE_IIR_FILTER_CHEBYSHEV1)
    { /* Butterworth or Chebyshev */
      /* generate the remaining zeros */
      while (2 * ds->n_solved_poles - 1 > ds->z_counter)
        {
          if (ifr->type != BSE_IIR_FILTER_HIGH_PASS)
            {
              ellf_debugf ("adding zero at Nyquist frequency\n");
              ds->z_counter += 1;
              ds->zcpz[ds->z_counter].re = -1.0; /* zero at Nyquist frequency */
              ds->zcpz[ds->z_counter].im = 0.0;
            }
          if (ifr->type == BSE_IIR_FILTER_BAND_PASS || ifr->type == BSE_IIR_FILTER_HIGH_PASS)
            {
              ellf_debugf ("adding zero at 0 Hz\n");
              ds->z_counter += 1;
              ds->zcpz[ds->z_counter].re = 1.0; /* zero at 0 Hz */
              ds->zcpz[ds->z_counter].im = 0.0;
            }
        }
    }
  else
    { /* elliptic */
      while (2 * ds->n_solved_poles - 1 > ds->z_counter)
        {
          ds->z_counter += 1;
          ds->zcpz[ds->z_counter].re = -1.0; /* zero at Nyquist frequency */
          ds->zcpz[ds->z_counter].im = 0.0;
          if (ifr->type == BSE_IIR_FILTER_BAND_PASS || ifr->type == BSE_IIR_FILTER_BAND_STOP)
            {
              ds->z_counter += 1;
              ds->zcpz[ds->z_counter].re = 1.0; /* zero at 0 Hz */
              ds->zcpz[ds->z_counter].im = 0.0;
            }
        }
    }
  ellf_outputf ("order = %d\n", ds->n_solved_poles);

  /* Expand the poles and zeros into numerator and
   * denominator polynomials
   */
  int j, icnt;
  for (icnt = 0; icnt < 2; icnt++)
    {
      double yy[BSE_IIR_CARRAY_SIZE] = { 0, };
      for (j = 0; j < BSE_IIR_CARRAY_SIZE; j++)
        ds->zn[j] = 0.0;
      ds->zn[0] = 1.0;
      for (j = 0; j < ds->n_solved_poles; j++)
        {
          int jj = j;
          if (icnt)
            jj += ds->n_solved_poles;
          double a = ds->zcpz[jj].re;
          double b = ds->zcpz[jj].im;
          int i;
          for (i = 0; i <= j; i++)
            {
              int jh = j - i;
              ds->zn[jh + 1] = ds->zn[jh + 1] - a * ds->zn[jh] + b * yy[jh];
              yy[jh + 1] =  yy[jh + 1]  - b * ds->zn[jh] - a * yy[jh];
            }
        }
      if (icnt == 0)
        {
          for (j = 0; j <= ds->n_solved_poles; j++)
            ds->zd[j] = ds->zn[j];
        }
    }
#if 0   // original ellf gain calculation; this code produces wrong results for high order filters
  /* Scale factors of the pole and zero polynomials */
  double a = 1.0;
  switch (ifr->type)
    {
      double gam;
    case BSE_IIR_FILTER_HIGH_PASS:
      a = -1.0;
      /* fall through */
    case BSE_IIR_FILTER_LOW_PASS:
    case BSE_IIR_FILTER_BAND_STOP:
      ds->numerator_accu = 1.0;
      ds->denominator_accu = 1.0;
      for (j = 1; j <= ds->n_solved_poles; j++)
        {
          ds->numerator_accu = a * ds->numerator_accu + ds->zn[j];
          ds->denominator_accu = a * ds->denominator_accu + ds->zd[j];
        }
      break;
    case BSE_IIR_FILTER_BAND_PASS:
      gam = PI / 2.0 - asin (ds->cgam);  /* = acos(cgam) */
      int mh = ds->n_solved_poles / 2;
      ds->numerator_accu = ds->zn[mh];
      ds->denominator_accu = ds->zd[mh];
      double ai = 0.0;
      if (mh > ((ds->n_solved_poles / 4) * 2))
        {
          ai = 1.0;
          ds->numerator_accu = 0.0;
          ds->denominator_accu = 0.0;
        }
      for (j = 1; j <= mh; j++)
        {
          a = gam * j - ai * PI / 2.0;
          double cng = cos (a);
          int jh = mh + j;
          int jl = mh - j;
          ds->numerator_accu = ds->numerator_accu + cng * (ds->zn[jh] + (1.0 - 2.0 * ai) * ds->zn[jl]);
          ds->denominator_accu = ds->denominator_accu + cng * (ds->zd[jh] + (1.0 - 2.0 * ai) * ds->zd[jl]);
        }
    }
  ellf_debugf ("FILTERGAIN-ellf: %.17g / %.17g * scale = %.17g\n", ds->numerator_accu, ds->denominator_accu,
               ds->numerator_accu / ds->denominator_accu * ds->gain_scale);
#endif
  /* calculate gain by evaluating poles and zeros */
  BseComplex z;
  switch (ifr->type)
    {
    case BSE_IIR_FILTER_LOW_PASS:
    case BSE_IIR_FILTER_BAND_STOP:
    case BSE_IIR_FILTER_HIGH_PASS:
      if (ifr->type == BSE_IIR_FILTER_HIGH_PASS)
        z = bse_complex (-1, 0);
      else
        z = bse_complex (+1, 0);
      break;
    case BSE_IIR_FILTER_BAND_PASS:
      /* cgam = cos (band_pass_center_frequency) */
      z = bse_complex (ds->cgam, sin (PIO2 - asin (ds->cgam))); /* PI/2 - asin (cgam) = acos (cgam) = band-center-frequency */
      break;
    }
  BseComplex num = bse_complex (+1, 0);
  BseComplex den = bse_complex (+1, 0);
  uint i;
  for (i = 0; i < ds->n_solved_poles; i++)
    {
      const BseComplex zero = bse_complex (ds->zcpz[ds->n_solved_poles + i].re, ds->zcpz[ds->n_solved_poles + i].im);
      const BseComplex pole = bse_complex (ds->zcpz[i].re, ds->zcpz[i].im);
      num = bse_complex_mul (num, bse_complex_sub (z, zero));
      den = bse_complex_mul (den, bse_complex_sub (z, pole));
    }
  ds->numerator_accu = bse_complex_abs (num);
  ds->denominator_accu = bse_complex_abs (den);
  ellf_debugf ("FILTERGAIN-bse:  %.17g / %.17g * scale = %.17g\n", ds->numerator_accu, ds->denominator_accu,
               ds->numerator_accu / ds->denominator_accu * ds->gain_scale);
  return 0;
}

/* display quadratic factors */
static int
print_quadratic_factors (const BseIIRFilterRequest *ifr,
                         const EllfDesignState     *ds,
                         double x, double y,
                         bool                       is_pole) /* 1 if poles, 0 if zeros */
{
  double a, b, r, f, g, g0;
  
  if (y > 1.0e-16)      /* check for imaginary pole/zero */
    {
      a = -2.0 * x;
      b = x * x + y * y;
    }
  else                  /* real valued pole/zero */
    {
      a = -x;
      b = 0.0;
    }
  ellf_outputf ("q. f.\nz**2 %23.13E\nz**1 %23.13E\n", b, a);
  if (b != 0.0)
    {
      /* resonant frequency */
      r = sqrt (b);
      f = PI / 2.0 - asin (-a/(2.0 * r));
      f = f * ifr->sampling_frequency / (2.0 * PI);
      /* gain at resonance */
      g = 1.0 + r;
      g = g * g - (a * a / r);
      g = (1.0 - r) * sqrt (g);
      g0 = 1.0 + a + b;	/* gain at d.c. */
    }
  else
    {
      /* It is really a first-order network.
       * Give the gain at ds->nyquist_frequency and D.C.
       */
      f = ds->nyquist_frequency;
      g = 1.0 - a;
      g0 = 1.0 + a;
    }
  
  if (is_pole)
    {
      if (g != 0.0)
        g = 1.0 / g;
      else
        g = MAXNUM;
      if (g0 != 0.0)
        g0 = 1.0 / g0;
      else
        g = MAXNUM;
    }
  ellf_outputf ("f0 %16.8E  gain %12.4E  DC gain %12.4E\n\n", f, g, g0);
  return 0;
}

static int
gainscale_and_print_deno_nume_zeros2_poles2 (const BseIIRFilterRequest *ifr, /* zplnc */
                                             EllfDesignState           *ds)
{
  int j;
  ds->gain = ds->denominator_accu / (ds->numerator_accu * ds->gain_scale);
  if ((ifr->kind == BSE_IIR_FILTER_BUTTERWORTH || ifr->kind == BSE_IIR_FILTER_CHEBYSHEV1) && ds->numerator_accu == 0)
    ds->gain = 1.0;
  ellf_outputf ("constant gain factor %23.13E\n", ds->gain);
  for (j = 0; j <= ds->n_solved_poles; j++)
    ds->zn[j] = ds->gain * ds->zn[j];
  
  ellf_outputf ("z plane Denominator      Numerator\n");
  for (j = 0; j <= ds->n_solved_poles; j++)
    {
      ellf_outputf ("%2d %17.9E %17.9E\n", j, ds->zd[j], ds->zn[j]);
    }

  /* I /think/ at this point the polynomial is factorized in 2nd order filters,
   * so that it can be implemented without stability problems -- stw
   */
  ellf_outputf ("poles and zeros with corresponding quadratic factors\n");
  for (j = 0; j < ds->n_solved_poles; j++)
    {
      double a = ds->zcpz[j].re;
      double b = ds->zcpz[j].im;
      if (b >= 0.0)
        {
          ellf_outputf ("pole  %23.13E %23.13E\n", a, b);
          print_quadratic_factors (ifr, ds, a, b, true);
        }
      a = ds->zcpz[ds->n_solved_poles + j].re;
      b = ds->zcpz[ds->n_solved_poles + j].im;
      if (b >= 0.0)
        {
          ellf_outputf ("zero  %23.13E %23.13E\n", a, b);
          print_quadratic_factors (ifr, ds, a, b, false);
        }
    }
  return 0;
}

/* Calculate frequency response at f Hz mulitplied by amp */
static double
response (const BseIIRFilterRequest *ifr,
          EllfDesignState           *ds,
          double f, double amp)
{
  BseComplex x, num, den, w;
  double u;
  int j;
  
  /* exp(j omega T) */
  u = 2.0 * PI * f /ifr->sampling_frequency;
  x.re = cos (u);
  x.im = sin (u);
  
  num.re = 1.0;
  num.im = 0.0;
  den.re = 1.0;
  den.im = 0.0;
  for (j = 0; j < ds->n_solved_poles; j++)
    {
      w = bse_complex_sub (x, ds->zcpz[j]);
      den = bse_complex_mul (w, den);
      w = bse_complex_sub (x, ds->zcpz[ds->n_solved_poles + j]);
      num = bse_complex_mul (w, num);
    }
  w = bse_complex_div (num, den);
  w.re *= amp;
  w.im *= amp;
  u = bse_complex_abs (w);
  return u;
}

/* Print table of filter frequency response */
static void
print_filter_table (const BseIIRFilterRequest *ifr,
                    EllfDesignState           *ds)
{
  double f, limit = 0.05 * ds->nyquist_frequency * 21;
  
  for (f=0; f < limit; f += limit / 21.)
    {
      double r = response (ifr, ds, f, ds->gain);
      if (r <= 0.0)
        r = -999.99;
      else
        r = BSE_DECIBEL20_FACTOR * log (r);
      ellf_debugf ("%10.1f  %10.2f\n", f, r);
      // f = f + 0.05 * ds->nyquist_frequency;
    }
}

/* --- main IIR filter design function --- */
static const char*
ellf_filter_design (const BseIIRFilterRequest *ifr,
                    EllfDesignState           *ds)
{
  double passband_edge1 = ifr->passband_edge;
  double passband_edge0 = ifr->passband_edge2;
  
  if (ifr->kind != BSE_IIR_FILTER_BUTTERWORTH &&
      ifr->kind != BSE_IIR_FILTER_CHEBYSHEV1 &&
      ifr->kind != BSE_IIR_FILTER_ELLIPTIC)
    return "unknown kind";
  if (ifr->type != BSE_IIR_FILTER_LOW_PASS &&
      ifr->type != BSE_IIR_FILTER_BAND_PASS &&
      ifr->type != BSE_IIR_FILTER_HIGH_PASS &&
      ifr->type != BSE_IIR_FILTER_BAND_STOP)
    return "unknown type";
  if (ifr->order <= 0)
    return "order too small";

  if (ifr->kind == BSE_IIR_FILTER_CHEBYSHEV1 || ifr->kind == BSE_IIR_FILTER_ELLIPTIC)
    {
      if (ifr->passband_ripple_db <= 0.0)
        return "passband_ripple_db too small";
      if (ifr->kind == BSE_IIR_FILTER_CHEBYSHEV1)
        {
          /* For Chebyshev filter, ripples go from 1.0 to 1/sqrt(1+ds->ripple_epsilon^2) */
          ds->chebyshev_phi = exp (ifr->passband_ripple_db / BSE_DECIBEL20_FACTOR);

          if ((ifr->order & 1) == 0)
            ds->gain_scale = ds->chebyshev_phi;
          else
            ds->gain_scale = 1.0;
        }
      else
        { /* elliptic */
          ds->ripple_epsilon = exp (ifr->passband_ripple_db / BSE_DECIBEL10_FACTOR);
          ds->gain_scale = 1.0;
          if ((ifr->order & 1) == 0)
            ds->gain_scale = sqrt (ds->ripple_epsilon);
          ds->ripple_epsilon = sqrt (ds->ripple_epsilon - 1.0);
        }
    }
  
  if (ifr->sampling_frequency <= 0.0)
    return "sampling_frequency too small";
  
  ds->nyquist_frequency = 0.5 * ifr->sampling_frequency;
  
  if (passband_edge1 <= 0.0)
    return "passband_edge1 too small";
  if (passband_edge1 >= ds->nyquist_frequency)
    return "passband_edge1 too high";
  
  if (ifr->type == BSE_IIR_FILTER_BAND_PASS || ifr->type == BSE_IIR_FILTER_BAND_STOP)
    {
      if (passband_edge0 <= 0.0)
        return "passband_edge too small";
      if (passband_edge0 >= ds->nyquist_frequency)
        return "passband_edge too high";
    }
  else
    {
      passband_edge0 = 0.0;
    }
  if (passband_edge1 < passband_edge0)
    {
      double tmp = passband_edge1;
      passband_edge1 = passband_edge0;
      passband_edge0 = tmp;
    }
  double high_edge, band_width;
  if (ifr->type == BSE_IIR_FILTER_HIGH_PASS)
    {
      band_width = passband_edge1;
      high_edge = ds->nyquist_frequency;
    }
  else
    {
      band_width = passband_edge1 - passband_edge0;
      high_edge = passband_edge1;
    }
  /* Frequency correspondence for bilinear transformation
   *
   *  Wanalog = tan(2 pi Fdigital T / 2)
   *
   * where T = 1/ifr->sampling_frequency
   */
  double ang = band_width * PI / ifr->sampling_frequency; /* angle frequency */
  double cang = cos (ang);
  ds->tan_angle_frequency = sin (ang) / cang; /* Wanalog */
  if (ifr->kind == BSE_IIR_FILTER_BUTTERWORTH || ifr->kind == BSE_IIR_FILTER_CHEBYSHEV1)
    {
      ds->wc = ds->tan_angle_frequency;
      /*ellf_debugf("cos(1/2 (Whigh-Wlow) T) = %.5e, wc = %.5e\n", cang, ds->wc);*/
    }
  
  if (ifr->kind == BSE_IIR_FILTER_ELLIPTIC)
    { /* elliptic */
      double tmp_cgam = cos ((high_edge + passband_edge0) * PI / ifr->sampling_frequency) / cang;
      ds->cgam = tmp_cgam;
      if (ifr->stopband_edge > 0.0)
        ds->stopband_edge = ifr->stopband_edge;
      else if (ifr->stopband_db >= 0.0)
        return "need stopband_db or stopband_edge";
      else /* stopband_db < 0.0 */
        { /* calculate band edge from db down */
          double a = exp (-ifr->stopband_db / BSE_DECIBEL10_FACTOR);
          double m1 = ds->ripple_epsilon / sqrt (a - 1.0);
          m1 *= m1;
          double m1p = 1.0 - m1;
          double Kk1 = ellpk (m1p);
          double Kpk1 = ellpk (m1);
          double q = exp (-PI * Kpk1 / (ifr->order * Kk1));
          double k = jacobi_theta_by_nome (q);
          if (ifr->type == BSE_IIR_FILTER_HIGH_PASS || ifr->type == BSE_IIR_FILTER_BAND_STOP)
            ds->wr = k;
          else
            ds->wr = 1.0 / k;
          if (ifr->type == BSE_IIR_FILTER_LOW_PASS || ifr->type == BSE_IIR_FILTER_HIGH_PASS)
            {
              ds->stopband_edge = atan (ds->tan_angle_frequency * ds->wr) * ifr->sampling_frequency / PI;
            }
          else
            {
              double a = ds->tan_angle_frequency * ds->wr;
              a *= a;
              double b = a * (1.0 - tmp_cgam * tmp_cgam) + a * a;
              b = (tmp_cgam + sqrt (b)) / (1.0 + a);
              ds->stopband_edge = (PI / 2.0 - asin (b)) * ifr->sampling_frequency / (2.0 * PI);
            }
        }
      switch (ifr->type)
	{
	case BSE_IIR_FILTER_LOW_PASS:
          if (ds->stopband_edge <= passband_edge1)
            return "need stopband_edge > passband_edge";
          break;
	case BSE_IIR_FILTER_BAND_PASS:
          if (ds->stopband_edge >= passband_edge0 && ds->stopband_edge <= passband_edge1)
            return "need stopband_edge < passband_edge or stopband_edge > passband_edge2";
          break;
	case BSE_IIR_FILTER_HIGH_PASS:
          if (ds->stopband_edge >= passband_edge1)
            return "need stopband_edge < passband_edge";
          break;
	case BSE_IIR_FILTER_BAND_STOP:
          if (ds->stopband_edge <= passband_edge0)
            return "need stopband_edge > passband_edge2";
          if (ds->stopband_edge >= passband_edge1)
            return "need stopband_edge < passband_edge";
          break;
	}
      ang = ds->stopband_edge * PI / ifr->sampling_frequency;
      cang = cos (ang);
      double sang = sin (ang);

      if (ifr->type == BSE_IIR_FILTER_LOW_PASS || ifr->type == BSE_IIR_FILTER_HIGH_PASS)
	{
          ds->wr = sang / (cang * ds->tan_angle_frequency);
	}
      else
	{
          double q = cang * cang  -  sang * sang;
          sang = 2.0 * cang * sang;
          cang = q;
          ds->wr = (tmp_cgam - cang) / (sang * ds->tan_angle_frequency);
	}

      if (ifr->type == BSE_IIR_FILTER_HIGH_PASS || ifr->type == BSE_IIR_FILTER_BAND_STOP)
	ds->wr = 1.0 / ds->wr;
      if (ds->wr < 0.0)
	ds->wr = -ds->wr;

      const double tmp_y0 = 1.0;
      double tmp_y1 = ds->wr;
      /* ds->chebyshev_band_cbp = ds->wr; */
      if (ifr->type == BSE_IIR_FILTER_HIGH_PASS || ifr->type == BSE_IIR_FILTER_BAND_STOP)
	tmp_y1 = 1.0 / tmp_y1;
      if (ifr->type == BSE_IIR_FILTER_LOW_PASS || ifr->type == BSE_IIR_FILTER_HIGH_PASS)
	{
          int i;
          for (i = 1; i <= 2; i++)
            {
              double tmp_y = i == 1 ? tmp_y0 : tmp_y1;
              ds->zd[i] = atan (ds->tan_angle_frequency * tmp_y) * ifr->sampling_frequency / PI ;
            }
          ellf_debugf ("pass band %.9E\n", ds->zd[1]);
          ellf_debugf ("stop band %.9E\n", ds->zd[2]);
	}
      else
	{
          int i;
          for (i = 1; i <= 2; i++)
            {
              double tmp_y = i == 1 ? tmp_y0 : tmp_y1;
              double a = ds->tan_angle_frequency * tmp_y;
              double b = atan (a);
              double q = sqrt (1.0 + a * a - tmp_cgam * tmp_cgam);
              q = atan2 (q, tmp_cgam);
              ds->zd[i] = (q + b) * ds->nyquist_frequency / PI;
              ds->zn[i] = (q - b) * ds->nyquist_frequency / PI;
            }
          ellf_debugf ("pass band %.9E %.9E\n", ds->zn[1], ds->zd[1]);
          ellf_debugf ("stop band %.9E %.9E\n", ds->zn[2], ds->zd[2]);
	}
      ds->wc = 1.0;
      find_elliptic_locations_in_lambda_plane (ifr, ds);	/* find locations in lambda plane */
      if ((2 * ifr->order + 2) > BSE_IIR_CARRAY_SIZE)
	goto toosml;
    } /* elliptic */
  
  /* Transformation from low-pass to band-pass critical frequencies
   *
   * Center frequency
   *                     cos(1/2 (Whigh+Wlow) T)
   *  cos(Wcenter T) = ----------------------
   *                     cos(1/2 (Whigh-Wlow) T)
   *
   *
   * Band edges
   *            cos(Wcenter T) - cos(Wdigital T)
   *  Wanalog = -----------------------------------
   *                        sin(Wdigital T)
   */
  
  if (ifr->kind == BSE_IIR_FILTER_CHEBYSHEV1)
    { /* Chebyshev */
      double a = PI * (high_edge + passband_edge0) / ifr->sampling_frequency ;
      ds->cgam = cos (a) / cang;
      a = 2.0 * PI * passband_edge1 / ifr->sampling_frequency;
      ds->chebyshev_band_cbp = (ds->cgam - cos (a)) / sin (a);
    }
  if (ifr->kind == BSE_IIR_FILTER_BUTTERWORTH)
    { /* Butterworth */
      double a = PI * (high_edge + passband_edge0) / ifr->sampling_frequency ;
      ds->cgam = cos (a) / cang;
      a = 2.0 * PI * passband_edge1 / ifr->sampling_frequency;
      /* ds->chebyshev_band_cbp = (ds->cgam - cos (a)) / sin (a); */
      ds->gain_scale = 1.0;
    }
  
  ellf_debugf ("State: gain_scale=%.20g ripple_epsilon=%.20g nyquist_frequency=%.20g " // BSE info
               "tan_angle_frequency=%.20g stopband_edge=%.20g wc=%.20g wr=%.20g cgam=%.20g\n",
               ds->gain_scale, ds->ripple_epsilon, ds->nyquist_frequency,
               ds->tan_angle_frequency, ds->stopband_edge, ds->wc, ds->wr, ds->cgam);
  
  find_s_plane_poles_and_zeros (ifr, ds);		/* find s plane poles and zeros */
  
  if ((ifr->type == BSE_IIR_FILTER_BAND_PASS || ifr->type == BSE_IIR_FILTER_BAND_STOP) && 4 * ifr->order + 2 > BSE_IIR_CARRAY_SIZE)
    goto toosml;
  
  convert_s_plane_to_z_plane (ifr, ds);	/* convert s plane to z plane */
  // volatile_sink ("x");
  z_plane_zeros_poles_to_numerator_denomerator (ifr, ds);
  ellf_debugf ("an=%.20g pn=%.20g scale=%.20g\n", ds->denominator_accu, ds->numerator_accu, ds->gain_scale); // BSE info
  gainscale_and_print_deno_nume_zeros2_poles2 (ifr, ds);
  print_filter_table (ifr, ds); /* tabulate transfer function */
  return NULL;

 toosml:
  return "storage arrays too small";
}
