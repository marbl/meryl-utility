
/******************************************************************************
 *
 *  This file is part of meryl-utility, a collection of miscellaneous code
 *  used by Meryl, Canu and others.
 *
 *  This software is based on:
 *    'Canu' v2.0              (https://github.com/marbl/canu)
 *  which is based on:
 *    'Celera Assembler' r4587 (http://wgs-assembler.sourceforge.net)
 *    the 'kmer package' r1994 (http://kmer.sourceforge.net)
 *
 *  Except as indicated otherwise, this is a 'United States Government Work',
 *  and is released in the public domain.
 *
 *  File 'README.licenses' in the root directory of this distribution
 *  contains full conditions and disclaimers.
 */

/*
   A C-program for MT19937, with initialization improved 2002/1/26.
   Coded by Takuji Nishimura and Makoto Matsumoto.

   Before using, initialize the state by using init_genrand(seed)
   or init_by_array(init_key, key_length).

   Copyright (C) 1997 - 2002, Makoto Matsumoto and Takuji Nishimura,
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   3. The names of its contributors may not be used to endorse or promote
   products derived from this software without specific prior written
   permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


   Any feedback is very welcome.
   http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/emt.html
   email: m-mat @ math.sci.hiroshima-u.ac.jp (remove space)
*/

#include "mt19937ar-v1.H"

#include <stdlib.h>
#include <math.h>
#include <time.h>

namespace merylutil::inline math::inline v1 {


mtRandom::mtRandom() {
  mtSetSeed(getpid() * time(NULL));
}

mtRandom::mtRandom(uint32 s) {
  mtSetSeed(s);
}


//  initialize with a single seed
void
mtRandom::mtSetSeed(uint32 s) {

  _seed  = s;
  _mt[0] = s;

  // See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier.  In the previous versions, MSBs of the seed
  // affect only MSBs of the array mt[].
  // 2002/01/09 modified by Makoto Matsumoto

  for (_mti=1; _mti<MT_N; _mti++)
    _mt[_mti] = (1812433253UL * (_mt[_mti-1] ^ (_mt[_mti-1] >> 30)) + _mti);

  _mag01[0] = 0;
  _mag01[1] = MT_MATRIX_A;
}



/* initialize by an array with array-length */
/* init_key is the array for initializing keys */
/* key_length is its length */
/* slight change for C++, 2004/2/26 */
mtRandom::mtRandom(uint32 *init_key, uint32 key_length) {

  mtSetSeed(19650218UL);

  int   i   = 1;
  int   j   = 0;
  int   k   = (MT_N > key_length ? MT_N : key_length);

  for (; k; k--) {
    _mt[i] = (_mt[i] ^ ((_mt[i-1] ^ (_mt[i-1] >> 30)) * 1664525UL)) + init_key[j] + j; /* non linear */
    i++;
    j++;
    if (i >= MT_N) {
      _mt[0] = _mt[MT_N-1];
      i=1;
    }
    if (j >= key_length)
      j=0;
  }

  for (k=MT_N-1; k; k--) {
    _mt[i] = (_mt[i] ^ ((_mt[i-1] ^ (_mt[i-1] >> 30)) * 1566083941UL)) - i; /* non linear */
    i++;
    if (i>=MT_N) {
      _mt[0] = _mt[MT_N-1];
      i=1;
    }
  }

  _mt[0] = 0x80000000UL; /* MSB is 1; assuring non-zero initial array */
}


/* generates a random number on [0,0xffffffff]-interval */
uint32
mtRandom::mtRandom32(void) {
  uint32 y = 0;

  //  generate MT_N words at one time
  //
  if (_mti >= MT_N) {
    int kk;

    for (kk=0; kk < MT_N - MT_M; kk++) {
      y = (_mt[kk] & MT_UPPER_MASK) | (_mt[kk+1] & MT_LOWER_MASK);
      _mt[kk] = _mt[kk + MT_M] ^ (y >> 1) ^ _mag01[y & 0x00000001UL];
    }
    for (; kk < MT_N-1; kk++) {
      y = (_mt[kk] & MT_UPPER_MASK) | (_mt[kk + 1] & MT_LOWER_MASK);
      _mt[kk] = _mt[kk + (MT_M - MT_N)] ^ (y >> 1) ^ _mag01[y & 0x00000001UL];
    }
    y = (_mt[MT_N-1] & MT_UPPER_MASK) | (_mt[0] & MT_LOWER_MASK);
    _mt[MT_N-1] = _mt[MT_M-1] ^ (y >> 1) ^ _mag01[y & 0x00000001UL];

    _mti = 0;
  }

  y = _mt[_mti++];

  /* Tempering */
  y ^= (y >> 11);
  y ^= (y << 7)  & 0x9d2c5680UL;
  y ^= (y << 15) & 0xefc60000UL;
  y ^= (y >> 18);

  _iter++;

  return(y);
}

//  generates a random number on gaussian distribution with 0 median and 1 std.dev.
double
mtRandom::mtRandomGaussian(double mean, double stddev) {
  double  x1=0, x2=0, w=0;

  //  from http://www.taygeta.com/random/gaussian.html
  //
  //  supposedly equivalent to
  //
  //  y1 = sqrt(-2*ln(x1)) cos(2*pi*x2)
  //  y2 = sqrt(-2*ln(x1)) sin(2*pi*x2)
  //
  //  but stable when x1 close to zero

  do {
    x1 = 2.0 * mtRandomRealClosed() - 1.0;
    x2 = 2.0 * mtRandomRealClosed() - 1.0;
    w = x1 * x1 + x2 * x2;
  } while (w >= 1.0);

  w = sqrt( (-2.0 * log(w)) / w);

  return(mean + x1 * w * stddev);  //  A second random can be generated from x2 * w.
}

//  Ganerate a number from an exponential distribution using Inverse Transform Sampling.
//
double
mtRandom::mtRandomExponential(double mode, double lambda) {
  return(mode - 1/lambda * log(mtRandomRealOpen()));
}

}  //  merylutil::files::v1
