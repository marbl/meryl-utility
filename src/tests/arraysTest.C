
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

#include "types.H"
#include "arrays.H"

using namespace merylutil;


int32
main(int32 argc, char **argv) {
  uint32   Alen=0, Amax=0;   uint32  *A=nullptr;
  uint32   Blen=0, Bmax=0;   uint32  *B=nullptr;
  uint32   Clen=0, Cmax=0;   uint32  *C=nullptr;
  uint32   Dlen=0, Dmax=0;   uint32  *D=nullptr;

  fprintf(stderr, "Testing increaseArray() and related.\n");
  fprintf(stderr, "For best results, run in valgrind.\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Calling allocateArray().\n");

  allocateArray(A, Amax, 4000);     //  Allocate new arrays, both will
  allocateArray(B, Bmax = 4000);    //  set elements to zero by default.

  for (uint32 ii=0; ii<4000; ii++)  //  Let valgrind check for overflow.
    A[ii] = B[ii] = ii;

  fprintf(stderr, "Testing allocateArray().\n");

  for (uint32 ii=0; ii<4000; ii++) {
    assert(A[ii] == ii);
    assert(B[ii] == ii);
  }

  for (uint32 ii=0; ii<4000; ii++)  //  Let valgrind check for overflow.
    A[ii] = ii << 8;

  for (uint32 ii=0; ii<4000; ii++)
    B[ii] = ii << 16;

  Alen = 4000;   //  Set the length of the two arrays;
  Blen = 4000;   //  necessary for duplicateArray() below!

  //

  fprintf(stderr, "Calling duplicateArray().\n");

  duplicateArray(C, Clen, Cmax, A, Alen, Amax);        //  Duplicate the arrays,
  duplicateArray(D, Dlen, Dmax, B, Blen, Bmax);        //  the last call will force
  duplicateArray(D, Dlen, Dmax, B, Blen, Bmax, true);  //  a reallocation.

  fprintf(stderr, "Testing duplicateArray().\n");
  for (uint32 ii=0; ii<4000; ii++) {
    assert(C[ii] == A[ii]);
    assert(D[ii] == B[ii]);
  }

  //

  fprintf(stderr, "Testing increaseArray(single).\n");

  for (uint32 ii=0; ii<6000; ii++) {
    increaseArray(A, ii, Amax, 511, _raAct::copyDataClearNew);
    A[ii] = ii;
  }

  fprintf(stderr, "  Reallocated A from 4000 to 6000 with max %lu\n", Amax);

  for (uint32 ii=0; ii<6000; ii++)
    assert(A[ii] == ii);
  for (uint32 ii=6000; ii<Amax; ii++)
    assert(A[ii] == 0);

  //

  fprintf(stderr, "Testing increaseArray(pair).\n");

  for (uint32 ii=0; ii<6000; ii++) {
    increaseArrayPair(C, D, ii, Cmax, 511, _raAct::copyDataClearNew);
    C[ii] = ii << 8;
    D[ii] = ii << 16;
  }

  fprintf(stderr, "  Reallocated C and D from 4000 to 6000 with max %lu\n", Cmax);

  for (uint32 ii=0; ii<6000; ii++) {
    assert(C[ii] == ii << 8);
    assert(D[ii] == ii << 16);
  }
  for (uint32 ii=6000; ii<Cmax; ii++) {
    assert(C[ii] == 0);
    assert(D[ii] == 0);
  }

  delete [] A;
  delete [] B;
  delete [] C;
  delete [] D;

  return(0);
}
