/*
 * Australian Public Licence B (OZPLB)
 * 
 * Version 1-0
 * 
 * Copyright (c) 2004 National ICT Australia
 * 
 * All rights reserved. 
 * 
 * Developed by: Embedded, Real-time and Operating Systems Program (ERTOS)
 *               National ICT Australia
 *               http://www.ertos.nicta.com.au
 * 
 * Permission is granted by National ICT Australia, free of charge, to
 * any person obtaining a copy of this software and any associated
 * documentation files (the "Software") to deal with the Software without
 * restriction, including (without limitation) the rights to use, copy,
 * modify, adapt, merge, publish, distribute, communicate to the public,
 * sublicense, and/or sell, lend or rent out copies of the Software, and
 * to permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimers.
 * 
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimers in the documentation and/or other materials provided
 *       with the distribution.
 * 
 *     * Neither the name of National ICT Australia, nor the names of its
 *       contributors, may be used to endorse or promote products derived
 *       from this Software without specific prior written permission.
 * 
 * EXCEPT AS EXPRESSLY STATED IN THIS LICENCE AND TO THE FULL EXTENT
 * PERMITTED BY APPLICABLE LAW, THE SOFTWARE IS PROVIDED "AS-IS", AND
 * NATIONAL ICT AUSTRALIA AND ITS CONTRIBUTORS MAKE NO REPRESENTATIONS,
 * WARRANTIES OR CONDITIONS OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO ANY REPRESENTATIONS, WARRANTIES OR CONDITIONS
 * REGARDING THE CONTENTS OR ACCURACY OF THE SOFTWARE, OR OF TITLE,
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, NONINFRINGEMENT,
 * THE ABSENCE OF LATENT OR OTHER DEFECTS, OR THE PRESENCE OR ABSENCE OF
 * ERRORS, WHETHER OR NOT DISCOVERABLE.
 * 
 * TO THE FULL EXTENT PERMITTED BY APPLICABLE LAW, IN NO EVENT SHALL
 * NATIONAL ICT AUSTRALIA OR ITS CONTRIBUTORS BE LIABLE ON ANY LEGAL
 * THEORY (INCLUDING, WITHOUT LIMITATION, IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHERWISE) FOR ANY CLAIM, LOSS, DAMAGES OR OTHER
 * LIABILITY, INCLUDING (WITHOUT LIMITATION) LOSS OF PRODUCTION OR
 * OPERATION TIME, LOSS, DAMAGE OR CORRUPTION OF DATA OR RECORDS; OR LOSS
 * OF ANTICIPATED SAVINGS, OPPORTUNITY, REVENUE, PROFIT OR GOODWILL, OR
 * OTHER ECONOMIC LOSS; OR ANY SPECIAL, INCIDENTAL, INDIRECT,
 * CONSEQUENTIAL, PUNITIVE OR EXEMPLARY DAMAGES, ARISING OUT OF OR IN
 * CONNECTION WITH THIS LICENCE, THE SOFTWARE OR THE USE OF OR OTHER
 * DEALINGS WITH THE SOFTWARE, EVEN IF NATIONAL ICT AUSTRALIA OR ITS
 * CONTRIBUTORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH CLAIM, LOSS,
 * DAMAGES OR OTHER LIABILITY.
 * 
 * If applicable legislation implies representations, warranties, or
 * conditions, or imposes obligations or liability on National ICT
 * Australia or one of its contributors in respect of the Software that
 * cannot be wholly or partly excluded, restricted or modified, the
 * liability of National ICT Australia or the contributor is limited, to
 * the full extent permitted by the applicable legislation, at its
 * option, to:
 * a.  in the case of goods, any one or more of the following:
 * i.  the replacement of the goods or the supply of equivalent goods;
 * ii.  the repair of the goods;
 * iii. the payment of the cost of replacing the goods or of acquiring
 *  equivalent goods;
 * iv.  the payment of the cost of having the goods repaired; or
 * b.  in the case of services:
 * i.  the supplying of the services again; or
 * ii.  the payment of the cost of having the services supplied again.
 * 
 * The construction, validity and performance of this licence is governed
 * by the laws in force in New South Wales, Australia.
 */

/*
 Authors: Ben Leslie
 Description: 
  Complex arithmetic as per 7.3
 Status: Functions defined, not implemented
*/

#error The complex library is incomplete

#define complex _Complex
#define _Complex_I (const float _Complex) foo

#define I _Complex_I

/* 7.3.5 Trigonometric functions */

/* 7.3.5.1 cacos */
double complex cacos(double complex z);
float complex cacosf(float complex z);
long double complex cacosl(long double complex z);

/* 7.3.5.2 casin */
double complex casin(double complex z);
float complex casinf(float complex z);
long double complex casinl(long double complex z);

/* 7.3.5.3 catan */
double complex catan(double complex z);
float complex catanf(float complex z);
long double complex catanl(long double complex z);

/* 7.3.5.4 ccos */
double complex ccos(double complex z);
float complex ccosf(float complex z);
long double complex ccosl(long double complex z);

/* 7.3.5.5 csin */
double complex csin(double complex z);
float complex csinf(float complex z);
long double complex csinl(long double complex z);

/* 7.3.5.6 ctan */
double complex ctan(double complex z);
float complex ctanf(float complex z);
long double complex ctanl(long double complex z);

/* 7.3.6 Hyperbolic functions */

/* 7.3.6.1 cacosh */
double complex cacosh(double complex z);
float complex cacoshf(float complex z);
long double complex cacoshf(long double complex z);

/* 7.3.6.2 casinh */
double complex casinh(double complex z);
float complex casinhf(float complex z);
long double complex casinhl(long double complex z);

/* 7.3.6.3 catanh */
double complex catanh(double complex z);
float complex catanhf(float complex z);
long double complex catanhl(long double complex z);

/* 7.3.6.4 ccosh */
double complex ccosh(double complex z);
float complex ccoshf(float complex z);
long double complex ccoshl(long double complex z);

/* 7.3.6.5 csinh */
double complex csinh(double complex z);
float complex csinhf(float complex z);
long double complex csinhl(long double complex z);

/* 7.3.6.6 ctanh */
double complex ctanh(double complex z);
float complex ctanhl(float complex z);
long double complex ctanhl(long double complex z);

/* 7.3.7 Exponential and logarithmic functions */

/* 7.3.7.1 cexp */
double complex cexp(double complex z);
float complex cexpf(float complex z);
long double complex cexpl(long double complex z);

/* 7.3.7.2 clog */
double complex clog(double complex z);
float complex clogf(float complex z);
long double complex clogl(long double complex z);

/* 7.3.8 Power and absolute value functions */

/* 7.3.8.1 cabs */
double complex cabs(double complex z);
float complex cabsf(float complex z);
long double complex cabsl(long double complex z);

/* 7.3.8.2 cpow */
double complex cpow(double complex z);
float complex cpowf(float complex z);
long double complex cpowl(long double complex z);

/* 7.3.8.3 csqrt */
double complex csqrt(double complex z);
float complex csqrtf(float complex z);
long double complex csqrtl(long double complex z);

/* 7.3.9 Manipulation functions */

/* 7.3.9.1 carg */
double complex carg(double complex z);
float complex cargf(float complex z);
long double complex cargl(long double complex z);

/* 7.3.9.2 cimag */
double complex cimag(double complex z);
float complex cimagf(float complex z);
long double complex cimagl(long double complex z);

/* 7.3.9.3 conj */
double complex conj(double complex z);
float complex conjf(float complex z);
long double complex conjl(long double complex z);

/* 7.3.9.4 cproj */
double complex cproj(double complex z);
float complex cprojf(float complex z);
long double complex cprojl(long double complex z);

/* 7.3.9.5 creal */
double complex creal(double complex z);
float complex crealf(float complex z);
long double complex creall(long double complex z);
