
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

#include "bits.H"



wordArray::wordArray(uint32 wordWidth, uint32 segmentSize) {

  _valueWidth       = (uint64)wordWidth;      //  In bits.
  _segmentSize      = (uint64)segmentSize;    //  In bits.

  _valuesPerSegment = _segmentSize / _valueWidth;

  _nextElement      = 0;

  _segmentsLen      = 0;
  _segmentsMax      = 16;
  _segments         = new uint128 * [_segmentsMax];

  for (uint32 ss=0; ss<_segmentsMax; ss++)
    _segments[ss] = nullptr;
}



wordArray::~wordArray() {
  for (uint32 i=0; i<_segmentsLen; i++)
    delete [] _segments[i];

  delete [] _segments;
}



void
wordArray::clear(void) {
  _nextElement = 0;
  _segmentsLen = 0;
}



void
wordArray::allocate(uint64 nElements) {
  uint64 nSegs = nElements / _valuesPerSegment + 1;

  //fprintf(stderr, "wordArray::allocate()-- allocating space for " F_U64 " elements, in " F_U64 " segments.\n",
  //        nElements, nSegs);

  assert(_segmentsLen == 0);

  resizeArray(_segments, _segmentsLen, _segmentsMax, nSegs, resizeArray_copyData | resizeArray_clearNew);

  for (uint32 seg=0; seg<nSegs; seg++)
    if (_segments[seg] == nullptr)
      _segments[seg] = new uint128 [_segmentSize / 128];

  //  For debug and test, set all bits in the allocated space.
  //for (uint32 seg=0; seg<nSegs; seg++)
  //  memset(_segments[seg], 0xff, sizeof(uint128) * _segmentSize / 128);

  _segmentsLen = nSegs;
}



void
wordArray::show(void) {
  fprintf(stderr, "wordArray:   valueWidth  %2" F_U64P "\n", _valueWidth);
  fprintf(stderr, "wordArray:   segmentSize %8" F_U64P "   valuesPerSegment %8" F_U64P "\n", _segmentSize, _valuesPerSegment);
  fprintf(stderr, "\n");

  uint32  bit  = 128;
  uint32  word = 0;
  char    bits[65];

  for (uint32 ss=0; ss<_segmentsLen; ss++) {
    fprintf(stderr, "Segment %u:\n", ss);

    for(uint32 wrd=0, bit=0; bit<_valuesPerSegment * _valueWidth; bit++) {
      if ((bit % 128) == 0) {
        displayWord(_segments[ss][wrd++], bits);
      }

      if ((bit % _valueWidth) == 0)
        fprintf(stderr, "word %2u: ", wrd);

      fprintf(stderr, "%c", bits[bit % 128]);

      if ((bit % _valueWidth) == _valueWidth - 1)
        fprintf(stderr, "\n");
    }
  }

  fprintf(stderr, "\n");
  fprintf(stderr, "\n");
}
