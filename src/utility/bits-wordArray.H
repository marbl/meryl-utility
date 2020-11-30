
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

//  To be included only by bits.H
#ifndef BITS_IMPLEMENTATIONS
#error Include bits.H instead of bits-wordArray.H
#endif


inline
uint128
wordArray::get(uint64 eIdx) {
  uint64  seg =                eIdx / _valuesPerSegment;    //  Which segment are we in?
  uint64  pos = _valueWidth * (eIdx % _valuesPerSegment);   //  Bit position of the start of the value.

  uint64  wrd = pos / 128;   //  The word we start in.
  uint64  bit = pos % 128;   //  Starting at this bit.

  uint128 val = 0;

  assert(eIdx < _nextElement);

  //  If the value is all in one word, just shift that word to the right to
  //  put the proper bits in the proper position.  We'll clean up the extra
  //  bits in just a moment.

  if (bit + _valueWidth <= 128) {
    val   = _segments[seg][wrd] >> (128 - _valueWidth - bit);
  }

  //  Otherwise, the value spans two words.  First, shift the first word so
  //  the end of it is at the start of the value.  Then shift the second
  //  word to the start of it is at the end of the value.
  //
  //                                 ssssssssssssssssssssss <- second shift
  //  [--word--][--first-word--][--second-word--][--word--]
  //                      [--value--]
  //                            fffff <- first shift
  //
  else {
    uint32  fShift = _valueWidth - (128 - bit);
    uint32  sShift = 128 - fShift;

    val  = _segments[seg][wrd+0] << fShift;
    val |= _segments[seg][wrd+1] >> sShift;
  }

  //  Finally, mask off the stuff we don't care about.

  val &= buildLowBitMask<uint128>(_valueWidth);

  return(val);
}



inline
void
wordArray::set(uint64 eIdx, uint128 value) {
  uint64 seg =                eIdx / _valuesPerSegment;     //  Which segment are we in?
  uint64 pos = _valueWidth * (eIdx % _valuesPerSegment);    //  Which word in the segment?

  uint64 wrd = pos / 128;   //  The word we start in.
  uint64 bit = pos % 128;   //  Starting at this bit.

  if (eIdx >= _nextElement)
    _nextElement = eIdx+1;

  if (seg >= _segmentsMax)
    resizeArray(_segments, _segmentsLen, _segmentsMax, seg + 16, resizeArray_copyData | resizeArray_clearNew);

  while (_segmentsLen <= seg) {
    _segments[_segmentsLen] = new uint128 [_segmentSize / 128];

    memset(_segments[_segmentsLen], 0xff, sizeof(uint64) * _segmentSize / 128);

    _segmentsLen++;
  }

  //  Mask the value, just in case.

  value &= buildLowBitMask<uint128>(_valueWidth);

  //  Set the value in the segment.

  //          [--------------------]
  //                 [value]
  //           lSave           rSave
  //
  if (bit + _valueWidth <= 128) {
    uint32   lSave = bit;
    uint32   rSave = 128 - _valueWidth - bit;

    _segments[seg][wrd] = (saveLeftBits(_segments[seg][wrd], lSave) |
                           (value << rSave)                         |
                           saveRightBits(_segments[seg][wrd], rSave));
  }

  //            --lSave--               --rSave--
  //  [--word--][--first-word--][--second-word--][--word--]
  //                     [----value---=]
  //                      lSize  rSize
  //
  else {
    uint32   lSave =       bit,   rSave = 128 - _valueWidth - bit;
    uint32   lSize = 128 - bit,   rSize = _valueWidth - (128 - bit);

    _segments[seg][wrd+0] = saveLeftBits(_segments[seg][wrd+0], lSave) | (value >> rSize);
    _segments[seg][wrd+1] = (value << rSave) | saveRightBits(_segments[seg][wrd+1], rSave);
  }
}

