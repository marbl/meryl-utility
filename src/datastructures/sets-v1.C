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

#include "sets-v1.H"

namespace merylutil::inline sets::inline v1 {


int
nothing(int x) {
  fprintf(stderr, "x");
}

//
//  Want datastructure that allows:
//   - storage of a list of 'set of integers'
//   - addition of a new 'set of integers'
//   - deletion would be nice but not required
//   - return index of existing 'set of integers' given a query set
//     or doesn't-exist
//   - simple implementation, no trees
//
//   - iteration over integers in the set
//



}  //    merylutil::sets::v1
