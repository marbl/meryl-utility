
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

#include "kmers.H"

namespace merylutil::inline kmers::v1 {

merylHistogram::merylHistogram() {
  _histMax       = 32 * 1024 * 1024;      //  256 MB of histogram data.
  _hist          = new uint64 [_histMax];

  for (uint64 ii=0; ii<_histMax; ii++)
    _hist[ii] = 0;
}


merylHistogram::~merylHistogram() {
  delete [] _hist;
  delete [] _histVs;
  delete [] _histOs;
}



void
merylHistogram::clear(void) {
  _numUnique     = 0;
  _numDistinct   = 0;
  _numTotal      = 0;

  for (uint64 ii=0; ii<_histMax; ii++)
    _hist[ii] = 0;

  _histBig.clear();

  delete [] _histVs;
  delete [] _histOs;

  _histLen       = 0;
  _histVs        = nullptr;
  _histOs        = nullptr;
}



void
merylHistogram::dump(stuffedBits *bits) {

  //  This only writes the latest version.

  bits->setBinary(64, _numUnique);
  bits->setBinary(64, _numDistinct);
  bits->setBinary(64, _numTotal);

  //  Find the maximum value, and count how many values we have
  //  in the histogram.

  uint64   numValues = _histBig.size();

  for (uint32 ii=0; ii<_histMax; ii++)
    if (_hist[ii] > 0)
      numValues++;

  bits->setBinary(64, numValues);

  //  Now the data!

  for (uint32 ii=0; ii<_histMax; ii++) {
    if (_hist[ii] > 0) {
      bits->setBinary(64,       ii);     //  Value
      bits->setBinary(64, _hist[ii]);    //  Number of occurrences
    }
  }

  for (auto it=_histBig.begin(); it != _histBig.end(); it++) {
    bits->setBinary(64, it->first);      //  Value
    bits->setBinary(64, it->second);     //  Number of occurrences
  }
}



void
merylHistogram::dump(FILE        *outFile) {
  stuffedBits  *bits = new stuffedBits;

  dump(bits);

  bits->dumpToFile(outFile);

  delete bits;
}



void
merylHistogram::load_v01(stuffedBits *bits) {

  _numUnique      = bits->getBinary(64);
  _numDistinct    = bits->getBinary(64);
  _numTotal       = bits->getBinary(64);

  uint32 histLast = bits->getBinary(32);
  uint32 hbigLen  = bits->getBinary(32);

  //  (over) allocate space for the histogram list and delete the accumulator.

  _histLen = 0;
  _histMax = histLast + hbigLen + 1;
  _histVs  = new uint64 [_histMax];
  _histOs  = new uint64 [_histMax];

  delete [] _hist;
  _hist    = NULL;

  //  Load histogram values and convert into _histVs and _histOs.

  uint64 *hh = bits->getBinary(64, histLast, new uint64 [histLast + 1]);

  for (uint32 ii=0; ii<histLast; ii++) {
    if (_hist[ii] > 0) {
      _histVs[_histLen] =    ii;
      _histOs[_histLen] = hh[ii];
      _histLen++;
    }
  }

  delete [] hh;
}



void
merylHistogram::load_v03(stuffedBits *bits) {

  _numUnique   = bits->getBinary(64);
  _numDistinct = bits->getBinary(64);
  _numTotal    = bits->getBinary(64);
  _histLen     = bits->getBinary(64);
  _histMax     = _histLen;

  //  Allocate space and delete the accumulator.

  _histVs = new uint64 [_histMax];
  _histOs = new uint64 [_histMax];

  delete [] _hist;
  _hist    = NULL;

  //  Load the values into our list.

  for (uint64 ii=0; ii<_histLen; ii++) {
    _histVs[ii] = bits->getBinary(64);
    _histOs[ii] = bits->getBinary(64);
  }
}



void
merylHistogram::load(stuffedBits *bits,
                     uint32       version) {

  switch (version) {
    case 1:
    case 2:
      load_v01(bits);
      break;
    case 3:
      load_v03(bits);
      break;
    default:
      fprintf(stderr, "merylHistogram::load()-- Unknown version %u\n", version), exit(1);
      break;
  }
}



void
merylHistogram::load(FILE        *inFile,
                     uint32       version) {
  stuffedBits  *bits = new stuffedBits;

  bits->loadFromFile(inFile);

  load(bits, version);

  delete bits;
}



void
merylHistogram::load(char const *histoname) {
  uint32                lLen = 0;
  uint32                lMax = 0;
  char                 *l    = nullptr;
  compressedFileReader *c    = new compressedFileReader(histoname);

  clear();

  delete [] _hist;

  _histLen = 0;
  _histMax = 0;
  _hist    = nullptr;

  while (readLine(l, lLen, lMax, c->file()) == true) {
    splitToWords  s(l);

    int64 hv = s.toint64(0);   //  There are 'ho' kmers
    int64 ho = s.toint64(1);   //  with value 'hv'.

    if (hv == 1)
      _numUnique  = ho;

    _numDistinct += ho;
    _numTotal    += ho * hv;

    increaseArrayPair(_histVs, _histOs, _histLen, _histMax, 32768);
    _histVs[_histLen] = hv;
    _histOs[_histLen] = ho;

    _histLen++;
  }

  delete    c;
  delete [] l;
}


}  //  namespace merylutil::kmers::v1
