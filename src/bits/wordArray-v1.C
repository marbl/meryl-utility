
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

namespace merylutil::inline bits::inline v1 {

wordArray::wordArray(uint32 valueWidth, uint64 segmentSizeInBits, bool useLocks) {

  if (valueWidth == 0)
    fprintf(stderr, "wordArray::wordArray()-- valueWidth=%u too small; must greater than zero.\n", valueWidth), exit(1);
  assert(valueWidth > 0);

  if (valueWidth > 128)
    fprintf(stderr, "wordArray::wordArray()-- valueWidth=%u too large; must be at most 128.\n", valueWidth), exit(1);
  assert(valueWidth <= 128);

  _valueWidth       = valueWidth;          //  In bits.
  _valueMask        = buildLowBitMask<uint128>(_valueWidth);
  _segmentSize      = segmentSizeInBits;   //  In bits.

  _valuesPerSegment = _segmentSize / _valueWidth;

  _wordsPerSegment  = _segmentSize / 128;
  _wordsPerLock     = (useLocks == false) ? (0) : (64);
  _locksPerSegment  = (useLocks == false) ? (0) : (_segmentSize / 128 / _wordsPerLock + 1);

  _numValuesAlloc   = 0;
  _validData        = 0;

  _lock.clear();

  _segmentsLen      = 0;
  _segmentsMax      = 16;
  _segments         = new uint128 *          [_segmentsMax];
  _segLocks         = new std::atomic_flag * [_segmentsMax];

  for (uint32 ss=0; ss<_segmentsMax; ss++) {
    _segments[ss] = nullptr;
    _segLocks[ss] = nullptr;
  }
}



wordArray::~wordArray() {
  for (uint32 i=0; i<_segmentsLen; i++) {
    delete [] _segments[i];
    delete [] _segLocks[i];
  }

  delete [] _segments;
  delete [] _segLocks;
}



//  Erase ALL allocated space to the given constant and set the array size
//  to be maxElt.
//
void
wordArray::erase(uint8 c, uint64 maxElt) {
  size_t   l =  sizeof(uint128) * _wordsPerSegment;

  allocate(maxElt);                             //  Allocate space for maxElt elements.
  _validData = maxElt;                          //  Claim there are that many elements.

  for (uint32 seg=0; seg<_segmentsLen; seg++)   //  Set all bytes to 'c'.
    memset(_segments[seg], c, l);
}



//  Allocate space for at least nElements.  Space will be allocated for an
//  integer number of blocks, each block having _valuesPerSegment elements.
//
//  Does not ever shrink space or change the number of elements in the array.
//
void
wordArray::allocate(uint64 nElements) {
  uint64 segmentsNeeded = nElements / _valuesPerSegment + 1;

  //  Allocate more space for segment pointers.  Does nothing
  //  if segmentsNeeded <= _segmentsMax.

  resizeArrayPair(_segments,
                  _segLocks,
                  _segmentsLen, _segmentsMax, segmentsNeeded,
                  _raAct::copyData | _raAct::clearNew);

  //  Allocate segments and locks, then open the locks.

  for (uint32 seg=_segmentsLen; seg<segmentsNeeded; seg++) {
    if (_segments[seg] != nullptr)
      continue;

    _segments[seg] = new uint128 [ _wordsPerSegment ];

    if (_locksPerSegment > 0) {
      _segLocks[seg] = new std::atomic_flag [ _locksPerSegment ];

      for (uint32 ll=0; ll<_locksPerSegment; ll++)
        _segLocks[seg][ll].clear();
    }
  }

  //  Update the number of values/segments we have allocated.

  if (segmentsNeeded > _segmentsLen) {
    _numValuesAlloc = segmentsNeeded * _valuesPerSegment;
    _segmentsLen    = segmentsNeeded;
  }
}



uint128
wordArray::get(uint64 eIdx) {
  uint64  seg =                eIdx / _valuesPerSegment;    //  Which segment are we in?
  uint64  pos = _valueWidth * (eIdx % _valuesPerSegment);   //  Bit position of the start of the value.

  uint64  wrd = pos / 128;   //  The word we start in.
  uint64  bit = pos % 128;   //  Starting at this bit.

  uint128 val = 0;

  if (eIdx >= _validData)
    fprintf(stderr, "wordArray::get()-- eIdx %lu >= _validData %lu\n", eIdx, _validData);
  assert(eIdx < _validData);

  //  If the value is all in one word, just shift that word to the right to
  //  put the proper bits in the proper position.
  //
  //  Otherwise, the value spans two words.
  //   - Shift the first word left to place the right-most bits at the left end of the return value.
  //   - Shift the second word right so the left-most bits are at the right end of the return value.
  //
  //                       ssssssssssss <- second shift amount
  //  [--first-word--][--second-word--]
  //             [--value--]
  //                   fffff <- first shift amount

  if (bit + _valueWidth <= 128) {
    val = _segments[seg][wrd] >> (128 - _valueWidth - bit);
  }
  else {
    uint32  fShift = _valueWidth - (128 - bit);
    uint32  sShift = 128 - fShift;

    val  = _segments[seg][wrd+0] << fShift;
    val |= _segments[seg][wrd+1] >> sShift;
  }

  //  Finally, mask off the stuff we don't care about.

  val &= _valueMask;

  return(val);
}



void
wordArray::set(uint64 eIdx, uint128 value) {
  uint64 seg =                eIdx / _valuesPerSegment;     //  Which segment are we in?
  uint64 pos = _valueWidth * (eIdx % _valuesPerSegment);    //  Which word in the segment?

  uint64 wrd = pos / 128;         //  The word we start in.
  uint64 bit = pos % 128;         //  Starting at this bit.

  uint64 lockW1 = 0;   //  Address of locks, computed inline with the
  uint64 lockW2 = 0;   //  setLock() function call below.

  //  Allocate more segment pointers and any missing segments.

  if (eIdx >= _numValuesAlloc) {
    setLock();
    allocate(eIdx);
    relLock();
  }

  //  Remember the largest element we've set.

  if (eIdx >= _validData) {
    setLock();
    _validData = std::max(_validData, eIdx+1);
    relLock();
  }

  //  Grab the locks for the two words we're going to be accessing.

  if (_wordsPerLock > 0)
    setLock(seg,
            lockW1 = (wrd + 0) / _wordsPerLock,
            lockW2 = (wrd + 1) / _wordsPerLock);

  //  Set the value in one word....
  //
  //          [--------------------]
  //                 [value]
  //           lSave           rSave
  //
  //  Or split the value across two words.
  //
  //            --lSave--               --rSave--
  //  [--word--][--first-word--][--second-word--][--word--]
  //                     [----value---=]
  //                      lSize  rSize

  value &= _valueMask;  //  Mask to width we can store, just in case.

  if (bit + _valueWidth <= 128) {
    uint32   lSave = bit;
    uint32   rSave = 128 - _valueWidth - bit;

    _segments[seg][wrd] = (saveLeftBits(_segments[seg][wrd], lSave) |
                           (value << rSave)                         |
                           saveRightBits(_segments[seg][wrd], rSave));
  }

  else {
    uint32   lSave =       bit,   rSave = 128 - _valueWidth - bit;
    uint32   lSize = 128 - bit,   rSize = _valueWidth - (128 - bit);

    _segments[seg][wrd+0] = saveLeftBits(_segments[seg][wrd+0], lSave) | (value >> rSize);
    _segments[seg][wrd+1] = (value << rSave) | saveRightBits(_segments[seg][wrd+1], rSave);
  }

  //  Release the locks.

  if (_wordsPerLock > 0)
    relLock(seg, lockW1, lockW2);
}



void
wordArray::show(void) {
  uint64  lastBit = _validData * _valueWidth;

  fprintf(stderr, "wordArray:\n");
  fprintf(stderr, "  validData        %10lu values\n", _validData);
  fprintf(stderr, "  valueWidth       %10lu bits\n",   _valueWidth);
  fprintf(stderr, "  segmentSize      %10lu bits\n",   _segmentSize);
  fprintf(stderr, "  valuesPerSegment %10lu values\n", _valuesPerSegment);
  fprintf(stderr, "\n");

  //  For each segment, dump full words, until we hit the end of data.

  for (uint64 ss=0; ss<_segmentsLen; ss++) {
    fprintf(stderr, "Segment %lu:\n", ss);

    uint64 bitPos = ss * _valuesPerSegment * _valueWidth;

    for (uint64 ww=0; (ww < _wordsPerSegment) && (bitPos < lastBit); ww += 4) {
      fprintf(stderr, "%5lu: %s %s %s %s\n",
              ww,
              (bitPos + 128 * 0 < lastBit) ? toHex(_segments[ss][ww+0]) : "",
              (bitPos + 128 * 1 < lastBit) ? toHex(_segments[ss][ww+1]) : "",
              (bitPos + 128 * 2 < lastBit) ? toHex(_segments[ss][ww+2]) : "",
              (bitPos + 128 * 3 < lastBit) ? toHex(_segments[ss][ww+3]) : "");

      bitPos += 128 * 4;
    }
  }

  fprintf(stderr, "\n");
  fprintf(stderr, "\n");
}

}  //  namespace merylutil::bits::v1
