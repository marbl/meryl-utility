
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

#include "arrays.H"
#include "strings.H"
#include "files.H"

#include "sampledDistribution-v1.H"

namespace merylutil::inline math::inline v1 {


void
sampledDistribution::loadDistribution(char const *path) {

  if ((path == nullptr) || (path[0] == 0))
    return;

  _dataLen = 0;
  _dataMax = 1048576;
  _data    = new uint32 [_dataMax];

  memset(_data, 0, sizeof(uint32) * _dataMax);

  splitToWords  S;

  uint32  Lnum = 0;
  uint32  Llen = 0;
  uint32  Lmax = 1024;
  char   *L    = new char [Lmax];

  FILE   *D    = merylutil::openInputFile(path);

  while (merylutil::readLine(L, Llen, Lmax, D) == true) {
    S.split(L);

    uint32  val = Lnum++;
    uint32  cnt = 0;

    if      (S.numWords() == 1) {
      val = S.touint32(0);
      cnt = 1;
    }

    else if (S.numWords() == 2) {
      val = S.touint32(0);
      cnt = S.touint32(1);
    }

    else {
      fprintf(stderr, "too many words on line '%s'\n", L);
      exit(1);
    }

    while (_dataMax <= val)
      resizeArray(_data, _dataLen, _dataMax, 2 * _dataMax, _raAct::copyData | _raAct::clearNew);

    _data[val] += cnt;
    _dataSum   += cnt;

    _dataLen = std::max(_dataLen, val + 1);
  }

  merylutil::closeFile(D);

  delete [] L;
}


//  Imagine our input histogram (value occurences) is expanded into an
//  array of (_dataSum) values, where each 'value' in the input histogram
//  is listed 'occurences' times (and that it's sorted).
//
//  For input 0 <= 'd' < 1, we want to return the 'value' that is at that
//  position in the array.
//
//  Scan the _data, looking for the _data element that contains the 'lim'th
//  entry in the (imagined) array.
//
uint32
sampledDistribution::getValue(double d) {
  if (d < 0.0)  d = 0.0;          //  Limit our input (random) number
  if (d > 1.0)  d = 1.0;          //  to valid scaling range.

  uint64  lim = (uint64)floor(_dataSum * d);
  uint64  val = 0;

  while (_data[val] <= lim) {     //  If _data[value] is more than the current
    lim -= _data[val];            //  limit, we're found the value, otherwise,
    val += 1;                     //  decrement the limit by the occurrences
  }                               //  for this value and move to the next.

  assert(val < _dataLen);

  return(val);
}


}  //  merylutil::files::v1
